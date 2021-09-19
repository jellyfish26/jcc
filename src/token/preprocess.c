#include "token/tokenize.h"
#include "util/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HashMap macros;

Macro *find_macro(Token *tkn) {
  return hashmap_nget(&macros, tkn->loc, tkn->len);
}

static bool add_macro(char *name, Macro *macro) {
  if (hashmap_get(&macros, name) != NULL) {
    return false;
  }
  hashmap_insert(&macros, name, macro);
  return true;
}

bool define_macro(char *name, bool is_objlike, Token *conv_tkn) {
  Macro *macro = calloc(1, sizeof(Macro));
  macro->name = name;
  macro->is_objlike = is_objlike;
  macro->conv_tkn = conv_tkn;
  return add_macro(name, macro);
}

static Token *copy_conv_tkn(Macro *macro) {
  Token head = {};
  Token *tkn = &head;

  for (Token *conv_tkn = macro->conv_tkn; conv_tkn != NULL; conv_tkn = conv_tkn->next) {
    tkn->next = calloc(1, sizeof(Token));
    memcpy(tkn->next, conv_tkn, sizeof(Token));
    tkn = tkn->next;
  }

  return head.next;
}

static void concat_token(Token *ltkn, Token *rtkn) {
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
  define_macro(ident, true, tokenize_file(builtin));

  *endptr = ptr;
}

// allow onl (ptr, &ptr)
void define_funclike_macro(char *ident, char *ptr, char **endptr) {
  Token *ident_tkn = tokenize_file(new_file("builtin", ident));
  ident = strndup(ident_tkn->loc, ident_tkn->len);

  int strlen = 0;
  while (*ptr != '\n' && *ptr != '\0') {
    ptr++;
    strlen++;
  }
  File *builtin = new_file("builtin", strndup(ptr - strlen, strlen));
  define_macro(ident, false, tokenize_file(builtin));

  *endptr = ptr;
}

Token *expand_macro(Token *tkn) {
  Macro *macro = find_macro(tkn);
  if (macro == NULL) {
    return NULL;
  }

  Token *macro_tkn = copy_conv_tkn(macro);
  Token *head = calloc(1, sizeof(Token));  // dummy

  // Check recursive macro
  for (Token *tkn = head; tkn->next != NULL; tkn = tkn->next) {
    if (tkn->next->kind == TK_IDENT && find_macro(tkn->next)) {
      tkn->next = tkn->next->next;
      concat_token(tkn, expand_macro(tkn->next));
    }
  }

  return head->next;
}
