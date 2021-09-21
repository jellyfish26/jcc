#include "token/tokenize.h"
#include "util/util.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct IncludePath IncludePath;
struct IncludePath {
  char *path;
  IncludePath *next;
};

static HashMap macros;
static IncludePath *include_paths;

static bool add_macro(char *name, Macro *macro) {
  if (hashmap_get(&macros, name) != NULL) {
    return false;
  }
  hashmap_insert(&macros, name, macro);
  return true;
}

Macro *find_macro(Token *tkn) {
  return hashmap_nget(&macros, tkn->loc, tkn->len);
}

bool define_macro(char *name, bool is_objlike, Token *conv_tkn, MacroArg *args) {
  Macro *macro = calloc(1, sizeof(Macro));
  macro->name = name;
  macro->is_objlike = is_objlike;
  macro->conv_tkn = conv_tkn;
  macro->args = args;
  return add_macro(name, macro);
}

static Token *copy_conv_tkn(Token *tkn) {
  Token head = {};
  Token *cur = &head;
  
  int cnt = 0;
  for (Token *conv_tkn = tkn; conv_tkn != NULL; conv_tkn = conv_tkn->next) {
    cur->next = calloc(1, sizeof(Token));
    memcpy(cur->next, conv_tkn, sizeof(Token));
    cur = cur->next;
  }

  return head.next;
}

static Token *extract_tkn(Macro *macro) {
  return copy_conv_tkn(macro->conv_tkn);
}

static void concat_token(Token *ltkn, Token *rtkn) {
  if (rtkn == NULL) {
    return;
  }

  Token *tail = ltkn->next;
  ltkn->next = rtkn;

  while (rtkn->next != NULL) {
    rtkn = rtkn->next;
  }
  rtkn->next = tail;
}

Token *delete_pp_token(Token *tkn) {
  Token *before = calloc(1, sizeof(Token));
  Token *now = before->next = tkn;
  tkn = before;

  while (now != NULL) {
    if (now->kind == TK_PP_SPACE) {
      before->next = now = now->next;
      continue;
    }

    before = now;
    now = now->next;
  }

  return tkn->next;
}

static Token *delete_enclose_pp_token(Token *tkn) {
  while (tkn != NULL && tkn->kind == TK_PP_SPACE) {
    tkn = tkn->next;
  }
  Token *head = tkn;

  Token *tail = tkn;
  while (tkn != NULL) {
    if (tkn->kind != TK_PP_SPACE) {
      tail = tkn;
    }
    tkn = tkn->next;
  }
  tail->next = NULL;

  return head;
}


// allow only (ptr, &ptr)
void define_objlike_macro(char *ident, char *ptr, char **endptr) {
  int strlen = 0;
  while (*ptr != '\n' && *ptr != '\0') {
    ptr++;
    strlen++;
  }
  File *builtin = new_file("builtin", strndup(ptr - strlen, strlen));
  define_macro(ident, true, tokenize_file(builtin, false), NULL);

  *endptr = ptr;
}

// allow onl (ptr, &ptr)
void define_funclike_macro(char *ident, char *ptr, char **endptr) {
  Token *ident_tkn = tokenize_file(new_file("builtin", ident), false);
  ident_tkn = delete_pp_token(ident_tkn);

  ident = strndup(ident_tkn->loc, ident_tkn->len);
  ident_tkn = skip(ident_tkn->next, "(");

  MacroArg head = {};
  MacroArg *cur = &head;

  while (!equal(ident_tkn, ")")) {
    if (cur != &head) {
      ident_tkn = skip(ident_tkn, ",");
    }

    cur->next = calloc(1, sizeof(MacroArg));
    cur = cur->next;

    if (equal(ident_tkn, ".")) {
      for (int i = 0; i < 3; i++) {
        ident_tkn = skip(ident_tkn, ".");
      }
      cur->name = "__VA_ARGS__";
      ident_tkn = skip(ident_tkn, ")");
      break;
    }

    cur->name = strndup(ident_tkn->loc, ident_tkn->len);
    ident_tkn = ident_tkn->next;
  }

  int strlen = 0;
  while (*ptr != '\n' && *ptr != '\0') {
    ptr++;
    strlen++;
  }

  File *builtin = new_file("builtin", strndup(ptr - strlen, strlen));
  define_macro(ident, false, tokenize_file(builtin, false), head.next);

  *endptr = ptr;
}

void set_macro_args(Macro *macro, char *ptr, char **endptr) {
  if (*ptr != '(') {
    errorf_loc(ER_COMPILE, ptr, 1, "Unexpected macro");
  }
  ptr++;

  MacroArg *arg = macro->args;
  char variadic[2048] = {};

  bool need_comma = false;
  while (*ptr != ')') {
    if (arg == NULL) {
      errorf_loc(ER_COMPILE, ptr, 1, "The number of arguments does not match");
    }

    if (need_comma && *ptr != ',') {
      errorf_loc(ER_COMPILE, ptr, 1, "Unexpected macro");
    } else if (need_comma) {
      ptr++;
    }
    need_comma = true;

    int len = 0;
    int pass_lparenthese = 0;
    while ((*ptr != ',' && *ptr != ')') || pass_lparenthese != 0) {
      if (pass_lparenthese != 0 && *ptr == ')') {
        pass_lparenthese--;
      }

      if (*ptr == '(') {
        pass_lparenthese++;
      }

      ptr++;
      len++;
    }

    if (strcmp(arg->name, "__VA_ARGS__") == 0) {
      bool chk = strlen(variadic) != 0;  // Check include commma
      char *str = strndup(ptr - len - chk, len + chk);
      strcat(variadic, str);
      free(str);
      continue;
    }

    arg->conv = strndup(ptr - len, len);
    arg = arg->next;
  }

  if (arg != NULL && strcmp(arg->name, "__VA_ARGS__") == 0) {
    arg->conv = strdup(variadic);
    arg = arg->next;
  }

  if (arg != NULL) {
    errorf_loc(ER_COMPILE, ptr, 1, "The number of arguments does not match");
  }

  *endptr = ptr + 1;
}

static MacroArg *find_macro_arg(Macro *macro, Token *tkn) {
  for (MacroArg *arg = macro->args; arg != NULL; arg = arg->next) {
    if (strlen(arg->name) == tkn->len && strncmp(arg->name, tkn->loc, tkn->len) == 0) {
      return arg;
    }
  }
  return NULL;
}

static char *stringizing(Token *tkn, bool add_dquote) {
  char *buf;
  size_t buflen;
  FILE *fp = open_memstream(&buf, &buflen);

  if (add_dquote) {
    fwrite("\"", sizeof(char), 1, fp);
  }

  tkn = delete_enclose_pp_token(tkn);
  while (tkn != NULL) {
    if (tkn->kind == TK_PP_SPACE) {
      putc(' ', fp);
    } else {
      char *str = strndup(tkn->loc, tkn->len);
      fwrite(str, sizeof(char), tkn->len, fp);
      free(str);
    }

    tkn = tkn->next;
  }

  if (add_dquote) {
    fwrite("\"\0", sizeof(char), 2, fp);
  } else {
    fputc('\0', fp);
  }

  fflush(fp);
  fclose(fp);

  return buf;
}

Token *expand_macro(Token *tkn) {
  Macro *macro = find_macro(tkn);
  if (macro == NULL) {
    return NULL;
  }

  Token *head = calloc(1, sizeof(Token));
  head->next = extract_tkn(macro);
  head->next = delete_pp_token(head->next);

  // Extract macro arguments
  Token *pass_sharp = NULL;
  for (Token *tkn = head; tkn != NULL && tkn->next != NULL; tkn = tkn->next) {
    // Concatenation
    if (tkn->next->next != NULL && equal(tkn->next->next, "##")) {
      Token *ltkn = tkn->next;
      Token *rtkn = tkn->next->next->next;

      if (rtkn == NULL) {
        errorf_tkn(ER_COMPILE, tkn->next->next, "'##' cannot appear at end of macro expansion");
      }

      char *lstr;
      char *rstr;

      if (find_macro_arg(macro, ltkn)) {
        ltkn = tokenize_file(new_file("builtin", find_macro_arg(macro, ltkn)->conv), false);
        ltkn = delete_enclose_pp_token(ltkn);
        lstr = stringizing(ltkn, false);
      }
      lstr = stringizing(ltkn, false);

      if (find_macro_arg(macro, rtkn)) {
        rtkn = tokenize_file(new_file("builtin", find_macro_arg(macro, rtkn)->conv), false);
        rtkn = delete_enclose_pp_token(rtkn);
      }
      rstr = stringizing(rtkn, false);

      char *str = calloc(strlen(lstr) + strlen(rstr) + 10, sizeof(char));
      strcat(str, lstr), strcat(str, rstr);
      free(lstr), free(rstr);

      Token *con_tkn = tokenize_file(new_file("builtin", str), true);
      get_tail_token(con_tkn)->next = tkn->next->next->next->next;  // lol
      tkn->next = con_tkn;
      continue;
    }

    if (equal(tkn->next, "#")) {
      pass_sharp = tkn;
      continue;
    }

    if (pass_sharp != NULL) {
      if (find_macro_arg(macro, tkn->next) == NULL) {
        errorf_tkn(ER_COMPILE, tkn->next, "Not found this argument");
      }

      Token *macro_tkn = tokenize_file(new_file("builtin", find_macro_arg(macro, tkn->next)->conv), false);
      macro_tkn = tokenize_file(new_file("builtin", stringizing(macro_tkn, true)), false);

      macro_tkn->next = tkn->next = tkn->next->next;
      pass_sharp->next = macro_tkn;
      pass_sharp = NULL;
      continue;
    }

    if (tkn->next->kind == TK_IDENT && find_macro_arg(macro, tkn->next) != NULL) {
      Token *macro_tkn = tokenize_file(new_file("builtin", find_macro_arg(macro, tkn->next)->conv), true);
      tkn->next = tkn->next->next;
      concat_token(tkn, macro_tkn);
      continue;
    }
  }

  // Extract macro
  for (Token *tkn = head; tkn->next != NULL; tkn = tkn->next) {
    Macro *macro = NULL;

    if (tkn->next->kind != TK_IDENT || (macro = find_macro(tkn->next)) == NULL) {
      continue;
    }

    if (macro->is_objlike) {
      Token *macro_tkn = expand_macro(tkn->next);
      tkn->next = tkn->next->next;
      concat_token(tkn, macro_tkn);
      continue;
    }

    if (!equal(tkn->next->next, "(")) {
      continue;
    }

    Token *ident_tkn = tkn->next;
    Token *arg_head = tkn->next->next;
    tkn->next = skip(tkn->next->next, "(");

    int pass_lparenthese = 0;
    while (pass_lparenthese != 0 || !equal(tkn->next, ")")) {
      if (equal(tkn->next, "(")) {
        pass_lparenthese++;
      }

      if (equal(tkn->next, ")")) {
        pass_lparenthese--;
      }

      tkn->next = tkn->next->next;
    }

    Token *arg_tail = tkn->next;
    tkn->next = tkn->next->next;
    arg_tail->next = NULL;

    char *ptr = stringizing(arg_head, false);
    set_macro_args(macro, ptr, &ptr);
    concat_token(tkn, expand_macro(ident_tkn));
  }

  return head->next;
}

void add_include_path(char *path) {
  IncludePath *include_path = calloc(1, sizeof(IncludePath));
  include_path->path = path;
  include_path->next = include_paths;
  include_paths = include_path;
}

static Token *expand_include(Token *tkn, bool allow_relative) {
  char path[1024] = {};

  // Find relative path
  if (allow_relative) {
    add_include_path(getcwd(path, 1024));
  }

  // Find absolute path
  FILE *fp = NULL;
  for (IncludePath *ipath = include_paths; ipath != NULL; ipath = ipath->next) {
    sprintf(path, "%s/%s", ipath->path, tkn->strlit);

    if ((fp = fopen(path, "r")) != NULL) {
      break;
    }
  }

  if (allow_relative) {
    include_paths = include_paths->next;
  }

  if (fp == NULL) {
    errorf_tkn(ER_COMPILE, tkn, "Failed open this file");
  }
  fclose(fp);

  return tokenize_file(read_file(path), true);
}


Token *read_include(char *ptr, char **endptr) {
  while (isspace(*ptr)) {
    ptr++;
  }

  bool allow_relative = (*ptr == '"');
  Token *tkn = new_token(TK_IDENT, ptr + 1, 0);
  ptr++;

  int len = 0;
  while (allow_relative ? (*ptr != '"') : (*ptr == '>')) {
    ptr++;
    len++;
  }
  tkn->strlit = strndup(ptr - len, len);
  tkn->len = len;

  *endptr = ptr + 1;
  return expand_include(tkn, allow_relative);
}
