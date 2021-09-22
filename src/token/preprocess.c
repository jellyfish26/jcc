#include "token/tokenize.h"
#include "util/util.h"

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

static IncludePath *include_paths;

static bool consume_pp_space(Token *tkn, Token **end_tkn) {
  bool chk = (tkn->kind == TK_PP && *(tkn->loc) == ' ');
  *end_tkn = (chk ? tkn->next : tkn);
  return chk;
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

void add_include_path(char *path) {
  IncludePath *include_path = calloc(1, sizeof(IncludePath));
  include_path->path = path;
  include_path->next = include_paths;
  include_paths = include_path;
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
    if (!equal(tkn->next, "#") && !equal(tkn->next->next, "include")) {
      tkn = tkn->next;
      continue;
    }

    Token *inc_tkn = tkn->next->next->next;
    consume_pp_space(inc_tkn, &inc_tkn);

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
  tkn = delete_pp_token(tkn);
  return tkn;
}

