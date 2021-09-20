#include "token/tokenize.h"
#include "util/util.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HashMap macros = {};

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

    arg->conv = strndup(ptr - len, len);
    arg->conv_tkn = tokenize_file(new_file("builtin", arg->conv), true);
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

  while (tkn != NULL) {
    if (tkn->kind == TK_KEYWORD) {
      fputc(' ', fp);
    }

    char *str = strndup(tkn->loc, tkn->len);
    fwrite(str, sizeof(char), tkn->len, fp);
    free(str);

    if (tkn->kind == TK_KEYWORD) {
      fputc(' ', fp);
    }

    if (tkn->kind == TK_IDENT && (tkn->next != NULL && tkn->next->kind == TK_IDENT)) {
      fputc(' ', fp);
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

  // Extract macro arguments
  Token *pass_sharp = NULL;
  for (Token *tkn = head; tkn->next != NULL; tkn = tkn->next) {
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
      Token *macro_tkn = find_macro_arg(macro, tkn->next)->conv_tkn;
      macro_tkn = copy_conv_tkn(macro_tkn);
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
