#include "token/tokenize.h"
#include "util/util.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct IncludePath IncludePath;
struct IncludePath {
  char *path;
  IncludePath *next;
};

typedef struct {
  char *name;
  bool is_objlike;
  Token *expand_tkn;
} Macro;

static IncludePath *include_paths;
static HashMap macros;

void add_include_path(char *path) {
  IncludePath *include_path = calloc(1, sizeof(IncludePath));
  include_path->path = path;
  include_path->next = include_paths;
  include_paths = include_path;
}

static Macro *find_macro(Token *tkn) {
  return hashmap_nget(&macros, tkn->loc, tkn->len);
}

static bool add_macro(char *name, Macro *macro) {
  if (hashmap_get(&macros, name) != NULL) {
    return false;
  }
  hashmap_insert(&macros, name, macro);
  return true;
}

static bool define_macro(char *name, bool is_objlike, Token *expand_tkn) {
  Macro *macro = calloc(1, sizeof(Macro));
  macro->name = name;
  macro->is_objlike = is_objlike;
  macro->expand_tkn = expand_tkn;
  return add_macro(name, macro);
}

// mask: 0 is all space, 1 is only ' ', 2 is only '\n'
static bool consume_pp_space(Token *tkn, Token **end_tkn, int mask) {
  bool chk = false;
  while (!is_eof(tkn)) {
    char c = *(tkn->loc);
    if (tkn->kind != TK_PP) {
      break;
    }

    if (mask == 0 && !isspace(c)) {
      break;
    }

    if (mask == 1 && c != ' ') {
      break;
    }

    if (mask == 2 && c != '\n') {
      break;
    }

    chk = true;
    tkn = tkn->next;
  }

  *end_tkn = tkn;
  return chk;
}

static void ignore_to_newline(Token *tkn, Token **endtkn) {
  while (!is_eof(tkn)) {
    if (tkn->kind == TK_PP && *(tkn->loc) == '\n') {
      break;
    }

    tkn = tkn->next;
  }

  *endtkn = tkn;
}


static Token *concat_separate_ident_token(Token *head) {
  Token *tkn = calloc(1, sizeof(Token));
  tkn->next = head;
  head = tkn;

  while (tkn->next != NULL && tkn->next->next != NULL) {
    Token *first = tkn->next;
    Token *second = tkn->next->next;

    if (first->kind != second->kind || first->kind != TK_IDENT) {
      tkn = tkn->next;
      continue;
    }

    char *ptr = first->loc;
    char *endptr = second->loc + second->len;

    first = new_token(TK_IDENT, ptr, endptr - ptr);
    first->next = second->next;
    tkn->next = first;
  }

  return head->next;
}

static Token *delete_pp_token(Token *tkn) {
  Token *before = calloc(1, sizeof(Token));
  Token *now = before->next = tkn;
  tkn = before;

  while (now != NULL) {
    if (now->kind == TK_PP) {
      before->next = now = now->next;
      continue;
    }

    before = now;
    now = now->next;
  }

  return tkn->next;
}

static Token *copy_token(Token *tkn) {
  Token *cpy = calloc(1, sizeof(Token));
  memcpy(cpy, tkn, sizeof(Token));
  cpy->next = NULL;
  return cpy;
}

static Token *copy_expand_tkn(Macro *macro, Token *ref_tkn) {
  Token head = {};
  Token *tkn = &head;

  for (Token *expand_tkn = macro->expand_tkn; expand_tkn != NULL; expand_tkn = expand_tkn->next) {
    tkn->next = calloc(1, sizeof(Token));
    memcpy(tkn->next, expand_tkn, sizeof(Token));

    tkn = tkn->next;
    tkn->ref_tkn = copy_token(ref_tkn);
  }

  return head.next;
}


static Token *expand_macro(Token *head) {
  Token *tkn = calloc(1, sizeof(Token));
  tkn->next = head;
  head = tkn;

  while (!is_eof(tkn->next)) {
    if (tkn->next->kind == TK_IDENT && find_macro(tkn->next)) {
      Macro *macro = find_macro(tkn->next);
      Token *ref_tkn = tkn->next;

      if (!macro->is_objlike && !equal(tkn->next->next, "(")) {
        tkn = tkn->next;
        continue;
      }
      tkn->next = tkn->next->next;

      if (!macro->is_objlike) {
        tkn->next = skip(tkn->next, "(");
        tkn->next = skip(tkn->next, ")");
      }

      Token *expand_tkn = copy_expand_tkn(macro, ref_tkn);
      get_tail_token(expand_tkn)->next = tkn->next;
      tkn->next = expand_tkn;
      continue;
    }

    if (is_eof(tkn->next->next)) {
      tkn = tkn->next;
      continue;
    }

    if (equal(tkn->next, "#") && equal(tkn->next->next, "define")) {
      Token *expand_tkn = tkn->next->next->next, *tail;
      ignore_to_newline(expand_tkn, &tail);

      tkn->next = tail->next;
      tail->next = NULL;
      expand_tkn = delete_pp_token(expand_tkn);

      char *name = get_ident(expand_tkn);
      bool is_objlike = true;
      expand_tkn = expand_tkn->next;

      if (consume(expand_tkn, &expand_tkn, "(")) {
        expand_tkn = skip(expand_tkn, ")");
        is_objlike = false;
      }

      define_macro(name, is_objlike, expand_tkn);
      continue;
    }

    tkn = tkn->next;
  }

  return head->next;
}

static Token *read_include(char *name, bool allow_curdir) {
  char path[1024] = {};

  // Find current directory
  if (allow_curdir) {
    add_include_path(getcwd(path, 1024));
  }

  // Find absolute path
  FILE *fp = NULL;
  for (IncludePath *ipath = include_paths; ipath != NULL; ipath = ipath->next) {
    sprintf(path, "%s/%s", ipath->path, name);

    if ((fp = fopen(path, "r")) != NULL) {
      break;
    }
  }

  if (allow_curdir) {
    include_paths = include_paths->next;
  }

  if (fp == NULL) {
    return NULL;
  }
  fclose(fp);

  return tokenize_file(read_file(path));
}

static Token *expand_include(Token *head) {
  Token *tkn = calloc(1, sizeof(Token));
  tkn->next = head;
  head = tkn;

  while (!is_eof(tkn->next) && !is_eof(tkn->next->next)) {
    if (!(equal(tkn->next, "#") && equal(tkn->next->next, "include"))) {
      tkn = tkn->next;
      continue;
    }

    Token *inc_tkn = tkn->next->next->next;
    consume_pp_space(inc_tkn, &inc_tkn, 0);

    char *name = inc_tkn->loc;
    bool allow_curdir = true;

    if (equal(inc_tkn, "<")) {
      allow_curdir = false;
      inc_tkn = inc_tkn->next;

      while (!equal(inc_tkn, ">")) {
        inc_tkn = inc_tkn->next;
      }
      name = erase_bslash_str(name, inc_tkn->next->loc - name - 1);
    }

    if (inc_tkn->kind == TK_STR) {
      name = inc_tkn->strlit;
    }
    inc_tkn = inc_tkn->next;

    tkn->next = read_include(name, allow_curdir);
    get_tail_token(tkn->next)->next = inc_tkn->next;
  }

  return head->next;
}

Token *preprocess(Token *tkn) {
  tkn = concat_separate_ident_token(tkn);
  tkn = expand_include(tkn);
  tkn = expand_macro(tkn);
  tkn = delete_pp_token(tkn);
  return tkn;
}

