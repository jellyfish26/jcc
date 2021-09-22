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
  char *name = get_ident(tkn);
  Macro *macro = hashmap_get(&macros, name);
  free(name);
  return macro;
}

static bool define_macro(char *name, bool is_objlike, Token *conv_tkn, MacroArg *args) {
  Macro *macro = calloc(1, sizeof(Macro));
  macro->name = name;
  macro->is_objlike = is_objlike;
  macro->conv_tkn = conv_tkn;
  macro->args = args;

  return add_macro(name, macro);
}

static void predefine_macro(char *name, char *conv) {
  Token *tkn = tokenize_file(new_file("builtin", conv), false);
  define_macro(name, true, tkn, NULL);
}

static void predefine_handler_macro(char *name, Token *(*handler)(Token *tkn)) {
  Macro *macro = calloc(1, sizeof(Macro));
  macro->name = name;
  macro->is_objlike = true;
  macro->macro_handler_fn = handler;
  add_macro(name, macro);
}

void undefine_macro(char *name) {
  hashmap_delete(&macros, name);
}

static Token *counter_macro(Token *tkn) {
  static int count = 0;

  char *str = calloc(16, sizeof(char));
  sprintf(str, "%d", count++);
  return tokenize_file(new_file("builtin", str), false);
}

static Token *file_macro(Token *tkn) {
  while (tkn->ref_tkn != NULL) {
    tkn = tkn->ref_tkn;
  }

  char *str = calloc(256, sizeof(char));
  sprintf(str, "\"%s\"", tkn->file->name);
  return tokenize_file(new_file("builtin", str), false);
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
  return tokenize_file(new_file("builtin", str), false);
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
  // Predefine macros
  predefine_macro("__STDC__", "1");
  predefine_macro("__STDC_VERSION__", "201112L");
  predefine_macro("__STDC_HOSTED__", "1");

  time_t now = time(NULL);
  struct tm *tm = localtime(&now);
  predefine_macro("__DATE__", to_format_date(tm));
  predefine_macro("__TIME__", to_format_time(tm));

  predefine_handler_macro("__LINE__", line_macro);
  predefine_handler_macro("__FILE__", file_macro);
  predefine_handler_macro("__COUNTER__", counter_macro);
}

static Token *copy_conv_tkn(Token *tkn) {
  Token head = {};
  Token *cur = &head;
  
  int cnt = 0;
  for (Token *conv_tkn = tkn; conv_tkn != NULL; conv_tkn = conv_tkn->next) {
    cur = cur->next = copy_token(conv_tkn);
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
void define_objlike_macro(char *name, char *ptr, char **endptr) {
  name = erase_bslash_str(name, ptr - name - 1);

  int len = 0;
  while (*ptr != '\n' && *ptr != '\0') {
    if (*ptr == '\\') {
      len += ignore_to_newline(ptr, &ptr);
      continue;
    }

    ptr++;
    len++;
  }

  define_macro(name, true, tokenize_str(ptr - len, ptr, false), NULL);
  *endptr = ptr;
}

// allow onl (ptr, &ptr)
void define_funclike_macro(char *name, char *ptr, char **endptr) {
  Token *ident_tkn = tokenize_str(name, ptr, false);
  ident_tkn = delete_pp_token(ident_tkn);

  name = get_ident(ident_tkn);
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

  define_macro(name, false, tokenize_str(ptr - strlen, ptr, false), head.next);
  *endptr = ptr;
}

void set_macro_args(Macro *macro, File *file, char *ptr, char **endptr) {
  if (*ptr != '(') {
    errorf_at(ER_COMPILE, file, ptr, 1, "Unexpected macro");
  }
  ptr++;

  MacroArg *arg = macro->args;
  int variadic = 0;

  bool need_comma = false;
  while (*ptr != ')') {
    if (arg == NULL) {
      errorf_at(ER_COMPILE, file, ptr, 1, "The number of arguments does not match");
    }

    if (need_comma && *ptr != ',') {
      errorf_at(ER_COMPILE, file, ptr, 1, "Unexpected macro");
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
      bool chk = (variadic != 0);  // Check include commma
      variadic += len + chk;
      continue;
    }
    
    arg->conv = ptr - len;
    arg->convlen = len;
    arg = arg->next;
  }

  if (arg != NULL && strcmp(arg->name, "__VA_ARGS__") == 0) {
    arg->conv = ptr - variadic;
    arg->convlen = variadic;
    arg = arg->next;
  }

  if (arg != NULL) {
    errorf_at(ER_COMPILE, file, ptr, 1, "The number of arguments does not match");
  }

  *endptr = ptr + 1;
}

static MacroArg *find_macro_arg(Macro *macro, Token *tkn) {
  char *name = get_ident(tkn);

  for (MacroArg *arg = macro->args; arg != NULL; arg = arg->next) {
    if (strcmp(arg->name, name) == 0) {
      free(name);
      return arg;
    }
  }

  free(name);
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

    char *str = NULL;
    switch (tkn->kind) {
      case TK_IDENT:
        str = erase_bslash_str(tkn->loc, tkn->len);
        break;
      case TK_PP_SPACE:
        str = strdup(" ");
        break;
      default:
        str = strndup(tkn->loc, tkn->len);
    }

    fwrite(str, sizeof(char), strlen(str), fp);
    free(str);

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
  macro->ref_tkn = tkn;

  Token *head = calloc(1, sizeof(Token));

  if (macro->macro_handler_fn != NULL) {
    head->next = macro->macro_handler_fn(macro->ref_tkn);
  } else {
    head->next = extract_tkn(macro);
  }
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

      MacroArg *arg = find_macro_arg(macro, ltkn);
      if (arg != NULL) {
        ltkn = tokenize_str(arg->conv, arg->conv + arg->convlen, false);
        ltkn = delete_enclose_pp_token(ltkn);
      }
      lstr = stringizing(ltkn, false);

      arg = find_macro_arg(macro, rtkn);
      if (arg != NULL) {
        rtkn = tokenize_str(arg->conv, arg->conv + arg->convlen, false);
        rtkn = delete_enclose_pp_token(rtkn);
      }
      rstr = stringizing(rtkn, false);

      char *str = calloc(strlen(lstr) + strlen(rstr) + 10, sizeof(char));
      strcat(str, lstr), strcat(str, rstr);
      free(lstr), free(rstr);

      Token *con_tkn = tokenize_file(new_file("builtin", str), false);
      get_tail_token(con_tkn)->next = tkn->next->next->next->next;
      tkn->next = con_tkn;
      continue;
    }

    if (equal(tkn->next, "#")) {
      pass_sharp = tkn;
      continue;
    }

    if (pass_sharp != NULL) {
      MacroArg *arg = NULL;
      if ((arg = find_macro_arg(macro, tkn->next)) == NULL) {
        errorf_tkn(ER_COMPILE, tkn->next, "Not found this argument");
      }

      Token *macro_tkn = tokenize_str(arg->conv, arg->conv + arg->convlen, false);
      macro_tkn = tokenize_file(new_file("builtin", stringizing(macro_tkn, true)), false);

      macro_tkn->next = tkn->next = tkn->next->next;
      pass_sharp->next = macro_tkn;
      pass_sharp = NULL;
      continue;
    }

    if (tkn->next->kind == TK_IDENT && find_macro_arg(macro, tkn->next) != NULL) {
      MacroArg *arg = find_macro_arg(macro, tkn->next);
      Token *macro_tkn = tokenize_str(arg->conv, arg->conv + arg->convlen, true);

      tkn->next = tkn->next->next;
      concat_token(tkn, macro_tkn);
      continue;
    }
  }

  // Set ref_tkn
  for (Token *tkn = head; tkn != NULL; tkn = tkn->next) {
    Token *ref_tkn = copy_token(macro->ref_tkn);
    Token *tail = ref_tkn;
    while (tail->ref_tkn != NULL) {
      tail = tail->ref_tkn;
    }

    tail->ref_tkn = tkn->ref_tkn;
    tkn->ref_tkn = ref_tkn;
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
    set_macro_args(macro, tkn->file, ptr, &ptr);
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

// Return NULL if include failed.
static Token *expand_include(char *inc_path, bool allow_relative) {
  char path[1024] = {};

  // Find relative path
  if (allow_relative) {
    add_include_path(getcwd(path, 1024));
  }

  // Find absolute path
  FILE *fp = NULL;
  for (IncludePath *ipath = include_paths; ipath != NULL; ipath = ipath->next) {
    sprintf(path, "%s/%s", ipath->path, inc_path);

    if ((fp = fopen(path, "r")) != NULL) {
      break;
    }
  }

  if (allow_relative) {
    include_paths = include_paths->next;
  }

  if (fp == NULL) {
    return NULL;
  }
  fclose(fp);

  return tokenize_file(read_file(path), true);
}


// Return NULL if include failed.
Token *read_include(char *ptr, char **endptr) {
  while (isspace(*ptr)) {
    ptr++;
  }

  bool allow_relative = (*ptr == '"');
  ptr++;

  int len = 0;
  while (allow_relative ? (*ptr != '"') : (*ptr != '>')) {
    ptr++;
    len++;
  }

  char *inc_path = strndup(ptr - len, len);

  *endptr = ptr + 1;
  return expand_include(inc_path, allow_relative);
}
