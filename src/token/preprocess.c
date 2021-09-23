#include "token/tokenize.h"
#include "util/util.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct IncludePath IncludePath;
struct IncludePath {
  char *path;
  IncludePath *next;
};

typedef struct MacroArg MacroArg;
struct MacroArg {
  char *name;
  MacroArg *next;

  // The expand_tkn variable stores the token to be replaced and
  // is used when expanding the macro.
  Token *expand_tkn;
};

typedef Token *macro_handler_fn(Token *tkn);

typedef struct {
  char *name;
  bool is_objlike;

  Token *expand_tkn;
  MacroArg *args;

  macro_handler_fn *handler;
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
  return hashmap_get(&macros, get_ident(tkn));
}

static bool add_macro(char *name, Macro *macro) {
  if (hashmap_get(&macros, name) != NULL) {
    return false;
  }
  hashmap_insert(&macros, name, macro);
  return true;
}

static bool define_macro(char *name, bool is_objlike, Token *expand_tkn, MacroArg *args) {
  Macro *macro = calloc(1, sizeof(Macro));
  macro->name = name;
  macro->is_objlike = is_objlike;
  macro->expand_tkn = expand_tkn;
  macro->args = args;
  return add_macro(name, macro);
}

static void predefine_macro(char *name, char *conv) {
  Token *tkn = tokenize_file(new_file("builtin", conv));
  define_macro(name, true, tkn, NULL);
}

static void predefine_handler_macro(char *name, macro_handler_fn *handler) {
  Macro *macro = calloc(1, sizeof(Macro));
  macro->name = name;
  macro->is_objlike = true;
  macro->handler = handler;
  add_macro(name, macro);
}

static void undefine_macro(char *name) {
  hashmap_delete(&macros, name);
}

static Token *counter_macro(Token *tkn) {
  static int count = 0;

  char *str = calloc(16, sizeof(char));
  sprintf(str, "%d", count++);
  return tokenize_file(new_file("builtin", str));
}

static Token *file_macro(Token *tkn) {
  while (tkn->ref_tkn != NULL) {
    tkn = tkn->ref_tkn;
  }

  char *str = calloc(256, sizeof(char));
  sprintf(str, "\"%s\"", tkn->file->name);
  return tokenize_file(new_file("builtin", str));
}

static Token *line_macro(Token *tkn) {
  while (tkn->ref_tkn != NULL) {
    tkn = tkn->ref_tkn;
  }

  int line_no = 1;
  for (char *loc = tkn->file->contents; loc != tkn->loc; loc++) {
    if (*loc == '\n') {
      line_no++;
    }
  }

  char *str = calloc(16, sizeof(char));
  sprintf(str, "%d", line_no);
  return tokenize_file(new_file("builtin", str));
}

// __DATE__ needs to be expanded to the current date and time,
// such as "Sep 22 2021".
static char *to_format_date(struct tm *tm) {
  static char *month[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Out", "Nov", "Dec",
  };

  char *str = calloc(16, sizeof(char));
  sprintf(str, "\"%s %2d %d\"", month[tm->tm_mon], tm->tm_mday, tm->tm_year + 1900);
  return str;
}

static char *to_format_time(struct tm *tm) {
  char *str = calloc(16, sizeof(char));
  sprintf(str, "\"%02d:%02d:%02d\"", tm->tm_hour, tm->tm_min, tm->tm_sec);
  return str;
}

void init_macro() {
  macros = (HashMap){0};

  // Predefine macros
  predefine_macro("__STDC__", "1");
  predefine_macro("__STDC_VERSION__", "201112L");
  predefine_macro("__STDC_HOSTED__", "1");

  time_t now = time(NULL);
  struct tm *tm = localtime(&now);
  predefine_macro("__DATE__", to_format_date(tm));
  predefine_macro("__TIME__", to_format_time(tm));

  predefine_handler_macro("__COUNTER__", counter_macro);
  predefine_handler_macro("__LINE__", line_macro);
  predefine_handler_macro("__FILE__", file_macro);
}

static MacroArg *find_macro_arg(Macro *macro, char *name) {
  for (MacroArg *arg = macro->args; arg != NULL; arg = arg->next) {
    if (strcmp(arg->name, name) == 0) {
      return arg;
    }
  }
  return NULL;
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
    printf("%s\n", get_ident(first));
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

static Token *delete_enclose_pp_token(Token *tkn) {
  consume_pp_space(tkn, &tkn, 0);
  Token *head = tkn;

  Token *tail = tkn;
  while (tkn != NULL) {
    if (tkn->kind != TK_PP) {
      tail = tkn;
    }
    tkn = tkn->next;
  }
  tail->next = NULL;

  return head;
}

static Token *copy_token(Token *tkn) {
  Token *cpy = calloc(1, sizeof(Token));
  memcpy(cpy, tkn, sizeof(Token));
  cpy->next = NULL;
  return cpy;
}

static Token *expand_macro(Token *head);

static Token *copy_arg_expand_tkn(MacroArg *arg, Token *ref_tkn) {
  Token head = {};
  Token *cur = &head;

  for (Token *expand_tkn = arg->expand_tkn; expand_tkn != NULL; expand_tkn = expand_tkn->next) {
    cur->next = calloc(1, sizeof(Token));
    memcpy(cur->next, expand_tkn, sizeof(Token));

    cur = cur->next;
    cur->ref_tkn = copy_token(ref_tkn);
  }

  return head.next;
}

static char *stringizing(Token *tkn, bool has_dquote) {
  tkn = delete_enclose_pp_token(tkn);

  char *buf;
  size_t buflen;
  FILE *fp = open_memstream(&buf, &buflen);

  if (has_dquote) {
    putc('"', fp);
  }

  while (tkn != NULL) {
    if (consume_pp_space(tkn, &tkn, 0)) {
      putc(' ', fp);
      continue;
    }

    char *str = erase_bslash_str(tkn->loc, tkn->len);
    fwrite(str, sizeof(char), strlen(str), fp);
    free(str);
    tkn = tkn->next;
  }

  if (has_dquote) {
    putc('"', fp);
  }
  putc('\0', fp);

  fflush(fp);
  fclose(fp);

  return buf;
}

static Token *copy_expand_tkn(Macro *macro, Token *ref_tkn) {
  Token *head = calloc(1, sizeof(Token));
  Token *cur = head;

  if (macro->handler != NULL) {
    cur->next = macro->handler(ref_tkn);
    for (Token *expand_tkn = cur->next; cur != NULL; cur = cur->next) {
      expand_tkn->ref_tkn = copy_token(ref_tkn);
    }
    return head->next;
  }

  for (Token *expand_tkn = macro->expand_tkn; expand_tkn != NULL; expand_tkn = expand_tkn->next) {
    cur->next = calloc(1, sizeof(Token));
    memcpy(cur->next, expand_tkn, sizeof(Token));

    cur = cur->next;
    cur->ref_tkn = copy_token(ref_tkn);
  }

  // Expand macro arguments
  cur = head;
  while (cur->next != NULL) {
    // Stringizing
    if (equal(cur->next, "#") && cur->next->next != NULL) {
      MacroArg *arg = find_macro_arg(macro, get_ident(cur->next->next));
      if (arg == NULL) {
        errorf_tkn(ER_COMPILE, cur->next, "'#' is not follwed by a macro parameter");
      }

      char *loc = cur->next->loc;
      int len = cur->next->next->loc - loc + cur->next->next->len;
      Token *ref_tkn = copy_token(cur->next);
      ref_tkn->len = len;

      Token *tail = cur->next->next->next;

      char *str = stringizing(copy_arg_expand_tkn(arg, ref_tkn), true);
      cur->next = tokenize_file(new_file("builtin", str));
      cur->next->ref_tkn = ref_tkn;
      cur->next->next = tail;
      continue;
    }

    // Concatenate
    if (cur->next->next != NULL && equal(cur->next->next, "##")) {
      Token *lhs = cur->next;
      Token *rhs = cur->next->next->next;
      Token *tail = rhs->next;

      if (rhs == NULL) {
        errorf_tkn(ER_COMPILE, lhs->next, "'##' cannot appear at end of macro expansion");
      }

      Token *ref_tkn = copy_token(lhs);
      ref_tkn->len = rhs->loc - lhs->loc + rhs->len;
      char *lstr;
      char *rstr;

      MacroArg *arg = NULL;
      if ((arg = find_macro_arg(macro, get_ident(lhs))) != NULL) {
        lhs = copy_arg_expand_tkn(arg, ref_tkn);
        lhs = delete_enclose_pp_token(lhs);
      }
      lstr = stringizing(lhs, false);

      if ((arg = find_macro_arg(macro, get_ident(rhs))) != NULL) {
        rhs = copy_arg_expand_tkn(arg, ref_tkn);
        rhs = delete_enclose_pp_token(rhs);
      }
      rstr = stringizing(rhs, false);

      char *str = calloc(strlen(lstr) + strlen(rstr) + 8, sizeof(char));
      strcat(str, lstr), strcat(str, rstr);
      free(lstr), free(rstr);

      Token *con_tkn = tokenize_file(new_file("builtin", str));

      // Set reference token
      for (Token *tkn = con_tkn; tkn != NULL; tkn = tkn->next) {
        Token *ref = tkn;
        for (;ref->ref_tkn != NULL; ref = ref->ref_tkn) {
          ref = ref->ref_tkn;
        }
        ref->ref_tkn = ref_tkn;
      }

      get_tail_token(con_tkn)->next = tail;
      cur->next = con_tkn;
      cur = get_tail_token(con_tkn);
      continue;
    }

    if (cur->next->kind != TK_IDENT) {
      cur = cur->next;
      continue;
    }

    MacroArg *arg = find_macro_arg(macro, get_ident(cur->next));
    if (arg == NULL) {
      cur = cur->next;
      continue;
    }

    Token *expand_tkn = copy_arg_expand_tkn(arg, cur->next);
    expand_tkn = expand_macro(expand_tkn);

    get_tail_token(expand_tkn)->next = cur->next->next;
    cur->next = expand_tkn;
  }

  return head->next;
}

static void set_macro_args(Macro *macro, Token *tkn, Token **end_tkn) {
  if (!consume(tkn, &tkn, "(")) {
    errorf_tkn(ER_COMPILE, tkn, "Unexpected macro");
  }

  MacroArg *arg = macro->args;

  bool need_commma = false;
  while (!equal(tkn, ")")) {
    if (arg == NULL) {
      errorf_tkn(ER_COMPILE, tkn, "The number of arguments does not match");
    }

    if (need_commma) {
      tkn = skip(tkn, ",");
    }
    need_commma = true;

    Token *expand_tkn = tkn;
    bool is_va_args = (strcmp(arg->name, "__VA_ARGS__") == 0);
    int pass_lparenthese = 0;
    while (true) {
      if (pass_lparenthese != 0 && equal(tkn, ")")) {
        pass_lparenthese--;
      }

      if (equal(tkn, "(")) {
        pass_lparenthese++;
      }

      if (pass_lparenthese == 0 && ((!is_va_args && equal(tkn->next, ",")) || equal(tkn->next, ")"))) {
        Token *tail = tkn->next;
        tkn->next = NULL;
        tkn = tail;
        break;
      }

      tkn = tkn->next;
    }

    arg->expand_tkn = expand_tkn;
    arg = arg->next;
  }

  if (arg != NULL && strcmp(arg->name, "__VA_ARGS__") != 0) {
    errorf_tkn(ER_COMPILE, tkn, "The number of arguments does not match");
  }

  *end_tkn = tkn->next;
}

static Token *expand_macro(Token *head) {
  Token *tkn = calloc(1, sizeof(Token));
  tkn->next = head;
  head = tkn;

  while (tkn->next != NULL && !is_eof(tkn->next)) {
    if (tkn->next->kind == TK_IDENT && find_macro(tkn->next) != NULL) {
      Macro *macro = find_macro(tkn->next);
      Token *ref_tkn = tkn->next;

      if (!macro->is_objlike && !equal(tkn->next->next, "(")) {
        tkn = tkn->next;
        continue;
      }
      tkn->next = tkn->next->next;

      if (!macro->is_objlike) {
        set_macro_args(macro, tkn->next, &(tkn->next));
      }

      Token *expand_tkn = copy_expand_tkn(macro, ref_tkn);
      get_tail_token(expand_tkn)->next = tkn->next;
      tkn->next = expand_tkn;
      continue;
    }

    if (tkn->next->next == NULL || is_eof(tkn->next->next)) {
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

      MacroArg head = {};
      MacroArg *cur = &head;

      if (consume(expand_tkn, &expand_tkn, "(")) {
        is_objlike = false;

        while (!consume(expand_tkn, &expand_tkn, ")")) {
          if (cur != &head) {
            expand_tkn = skip(expand_tkn, ",");
          }

          cur->next = calloc(1, sizeof(MacroArg));
          cur = cur->next;

          if (equal(expand_tkn, ".")) {
            for (int i = 0; i < 3; i++) {
              expand_tkn = skip(expand_tkn, ".");
            }
            cur->name = "__VA_ARGS__";
            expand_tkn = skip(expand_tkn, ")");
            break;
          }

          cur->name = get_ident(expand_tkn);
          expand_tkn = expand_tkn->next;
        }
      }

      define_macro(name, is_objlike, expand_tkn, head.next);
      continue;
    }

    if (equal(tkn->next, "#") && equal(tkn->next->next, "undef")) {
      Token *expand_tkn = tkn->next->next->next, *tail;
      ignore_to_newline(expand_tkn, &tail);
      expand_tkn = delete_pp_token(expand_tkn);

      undefine_macro(get_ident(expand_tkn));
      tkn->next = tail->next;
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

