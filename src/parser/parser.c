#include "parser/parser.h"
#include "token/tokenize.h"
#include "util/util.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// This structure is used to initialize variable.
// In particular, Array initializer can be nested, so need tree data structure.
typedef struct Initializer Initializer;
struct Initializer {
  Type *ty;
  Token *tkn;

  // The size of the first array can be omitted.
  bool is_flexible;
  Initializer **children;

  Node *node;
};

typedef struct {
  bool is_const;
} VarAttr;

static char *break_label;
static char *conti_label;

static HashMap *label_map;
static Node *label_node;
static Node *goto_node;

static Node *literal_node;

// Prototype
static Type *declspec(Token *tkn, Token **end_tkn);
static Type *type_suffix(Token *tkn, Token **end_tkn, Type *ty);
static Type *declarator(Token *tkn, Token **end_tkn, Type *ty);
static Type *abstract_declarator(Token *tkn, Token **end_tkn, Type *ty);
static Initializer *new_initializer(Type *ty, bool is_flexible);
static void initializer_only(Token *tkn, Token **end_tkn, Initializer *init);
static void array_initializer(Token *tkn, Token **end_tkn, Initializer *init);
static void string_initializer(Token *tkn, Token **end_tkn, Initializer *init);
static void create_init_node(Initializer *init, Node **connect, bool only_const, Type *ty);
static int64_t eval_expr(Node *node);
static int64_t eval_expr2(Node *node, char **label);
static int64_t eval_addr(Node *node, char **label);
static long double eval_double(Node *node);
static Node *funcdef(Token *tkn, Token **end_tkn, Type *base_ty);
static Node *comp_stmt(Token *tkn, Token **end_tkn);
static Initializer *initializer(Token *tkn, Token **end_tkn, Type *ty);
static Node *initdecl(Token *tkn, Token **end_tkn, Type *ty, bool is_global);
static Node *topmost(Token *tkn, Token **end_tkn);
static Node *statement(Token *tkn, Token **end_tkn);
static Node *expr(Token *tkn, Token **end_tkn);
static Node *assign(Token *tkn, Token **end_tkn);
static Node *conditional(Token *tkn, Token **end_tkn);
static Node *logical_or(Token *tkn, Token **end_tkn);
static Node *logical_and(Token *tkn, Token **end_tkn);
static Node *bitor(Token *tkn, Token **end_tkn);
static Node *bitxor(Token *tkn, Token **end_tkn);
static Node *bitand(Token *tkn, Token **end_tkn);
static Node *equality(Token *tkn, Token **end_tkn);
static Node *relational(Token *tkn, Token **end_tkn);
static Node *bitshift(Token *tkn, Token **end_tkn);
static Node *add(Token *tkn, Token **end_tkn);
static Node *mul(Token *tkn, Token **end_tkn);
static Node *cast(Token *tkn, Token **end_tkn);
static Node *unary(Token *tkn, Token **end_tkn);
static Node *postfix(Token *tkn, Token **end_tkn);
static Node *primary(Token *tkn, Token **end_tkn);
static Node *constant(Token *tkn, Token **end_tkn);

static char *new_unique_label() {
  static int cnt = 0;
  char *ptr = calloc(10, sizeof(char));
  sprintf(ptr, ".Luni%d", cnt++);
  return ptr;
}

Node *new_node(NodeKind kind, Token *tkn) {
  Node *node = calloc(1, sizeof(Node));
  node->tkn = tkn;
  node->kind = kind;
  return node;
}

Node *new_num(Token *tkn, int64_t val) {
  Node *node = new_node(ND_NUM, tkn);
  node->ty = ty_i32;
  node->val = val;
  return node;
}

Node *new_floating(Token *tkn, Type *ty, long double fval) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->ty = ty;
  node->fval = fval;
  return node;
}

Node *new_cast(Node *expr, Type *ty) {
  add_type(expr);

  Node *node = new_node(ND_CAST, expr->tkn);
  node->lhs = expr;
  node->ty = ty;
  return node;
}

Node *new_var(Token *tkn, Obj *obj) {
  Node *node = new_node(ND_VAR, tkn);
  node->var = obj;
  add_type(node);
  return node;
}

Node *new_strlit(Token *tkn) {
  Initializer *init = new_initializer(tkn->ty, false);
  string_initializer(tkn, NULL, init);

  Node *node = new_node(ND_INIT, tkn);

  Node head = {};
  Node *dummy = &head;
  create_init_node(init, &dummy, true, tkn->ty);

  node->lhs = new_var(tkn, new_obj(tkn->ty, new_unique_label()));
  node->lhs->var->is_global = true;
  node->rhs = head.lhs;

  node->next = literal_node;
  literal_node = node;

  Node *addr = new_node(ND_ADDR, tkn);
  addr->lhs = node->lhs;

  return addr;
}

static bool is_addr_node(Node *node) {
  switch (node->ty->kind) {
    case TY_PTR:
    case TY_ARRAY:
      return true;
    default:
      return false;
  }
}

static Node *new_binary(NodeKind kind, Token *tkn, Node *lhs) {
  Node *node = new_node(kind, tkn);
  node->lhs = lhs;
  return node;
}

Node *new_calc(NodeKind kind, Token *tkn, Node *lhs, Node *rhs) {
  Node *node = new_node(kind, tkn);
  node->lhs = lhs;
  node->rhs = rhs;
  add_type(node);

  if (is_addr_node(node->lhs) || is_addr_node(node->rhs)) {
    errorf_tkn(ER_COMPILE, tkn, "Invalid operand.");
  }
  return node;
}

Node *new_add(Token *tkn, Node *lhs, Node *rhs) {
  Node *node = new_node(ND_ADD, tkn);
  node->lhs = lhs;
  node->rhs = rhs;
  add_type(node);

  if (is_addr_node(node->lhs) && is_addr_node(node->rhs)) {
    errorf_tkn(ER_COMPILE, tkn, "Invalid operand.");
  }

  if (is_addr_node(node->lhs)) {
    node->rhs = new_calc(ND_MUL, tkn, node->rhs, new_num(tkn, node->lhs->ty->base->var_size));
  }

  if (is_addr_node(node->rhs)) {
    node->lhs = new_calc(ND_MUL, tkn, node->lhs, new_num(tkn, node->rhs->ty->base->var_size));
  }

  return node;
}

Node *new_sub(Token *tkn, Node *lhs, Node *rhs) {
  Node *node = new_node(ND_SUB, tkn);
  node->lhs = lhs;
  node->rhs = rhs;
  add_type(node);

  if (is_addr_node(node->lhs) && is_addr_node(node->rhs)) {
    if (!is_same_type(node->lhs->ty, node->rhs->ty)) {
      errorf_tkn(ER_COMPILE, tkn, "Invalid operand");
    }

    node->ty = ty_i64;
    node = new_calc(ND_DIV, tkn, node, new_num(tkn, node->lhs->ty->base->var_size));
    return node;
  }

  if (is_addr_node(node->lhs)) {
    node->rhs = new_calc(ND_MUL, tkn, node->rhs, new_num(tkn, node->lhs->ty->base->var_size));
  }

  if (is_addr_node(node->rhs)) {
    node->lhs = new_calc(ND_MUL, tkn, node->lhs, new_num(tkn, node->rhs->ty->base->var_size));
  }

  return node;
}

Node *new_assign(Token *tkn, Node *lhs, Node *rhs) {
  // Remove implicit cast 
  if (lhs->kind == ND_CAST) {
    lhs = lhs->lhs;
  }

  Node *node = new_node(ND_ASSIGN, tkn);
  node->lhs = lhs;
  node->rhs = rhs;
  add_type(node);

  if (lhs != NULL && lhs->ty->is_const) {
    errorf_tkn(ER_COMPILE, tkn, "Cannot assign to const variable.");
  }

  return node;
}

Node *to_assign(Token *tkn, Node *rhs) {
  return new_assign(tkn, rhs->lhs, rhs);
}

static bool is_typename(Token *tkn) {
  char *keywords[] = {
    "void", "_Bool", "char", "short", "int", "long", "float", "double", "signed", "unsigned",
    "const", "enum", "struct", "union"
  };

  for (int i = 0; i < sizeof(keywords) / sizeof(*keywords); i++) {
    if (equal(tkn, keywords[i])) {
      return true;
    }
  }

  return false;
}

static char *get_ident(Token *tkn) {
  if (tkn->kind != TK_IDENT) {
    errorf_tkn(ER_COMPILE, tkn, "Expected an identifier");
  }
  return strndup(tkn->loc, tkn->len);
}

// type-qualifier = "const"
static bool typequal(Token *tkn, Token **end_tkn, VarAttr *attr) {
  bool is_qual = false;

  if (equal(tkn, "const")) {
    if (attr->is_const) {
      errorf_tkn(ER_COMPILE, tkn, "Duplicate const");
    }

    attr->is_const = true;
    tkn = tkn->next;
    is_qual = true;
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return is_qual;
};

// The type of array, structure, enum and a initializer end with '}' or ',' and '}'.
static bool consume_close_brace(Token *tkn, Token **end_tkn) {
  if (equal(tkn, "}")) {
    *end_tkn = tkn->next;
    return true;
  }

  if (equal(tkn, ",") && equal(tkn->next, "}")) {
    *end_tkn = tkn->next->next;
    return true;
  }

  return false;
}

// enum-specifier = "enum" idenfitier |
//                  "enum" identifier? "{" enumerator ("," enumerator)* ","? "}"
//
// enumerator = identifier |
//              identifier "=" constant-expression
static Type *enumspec(Token *tkn, Token **end_tkn) {
  tkn = skip(tkn, "enum");

  char *tag = NULL;
  if (tkn->kind == TK_IDENT) {
    tag = get_ident(tkn);
    tkn = tkn->next;
  }

  if (tkn != NULL && !equal(tkn, "{")) {
    Type *ty = find_tag(tag);

    if (ty == NULL) {
      errorf_tkn(ER_COMPILE, tkn, "This enum tag is not declared");
    } else if (ty->kind != TY_ENUM) {
      errorf_tkn(ER_COMPILE, tkn, "This tag is not an enum tag");
    }

    if (end_tkn != NULL) *end_tkn = tkn;
    return ty;
  }

  Type *enumerator_ty = copy_ty(ty_i32);

  Obj head = {};
  Obj *cur = &head;
  int64_t val = -1;

  tkn = skip(tkn, "{");
  while (!consume_close_brace(tkn, &tkn)) {
    if (cur != &head) {
      tkn = skip(tkn, ",");
    }

    char *ident = get_ident(tkn);

    if (consume(tkn->next, &tkn, "=")) {
      val = eval_expr(expr(tkn, &tkn));
    } else {
      val++;
    }

    if (val >> 31 && enumerator_ty->kind == TY_INT) {
      enumerator_ty = copy_ty(ty_i64);
    }

    Obj *obj = new_obj(NULL, ident);
    obj->val = val;

    cur->next = obj;
    cur = obj;
  }

  Type *ty = copy_ty(enumerator_ty);
  ty->kind = TY_ENUM;
  ty->base = enumerator_ty;
  ty->is_const = true;

  for (Obj *obj = head.next; obj != NULL; obj = obj->next) {
    obj->ty = ty;
    add_var(obj, false);
  }

  if (tag != NULL) {
    add_tag(ty, tag);
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return ty;
}

// struct-or-union-specifier = struct-or-union identifier |
//                             struct-or-union identifier? "{" struct-declaration-list "}"
//
// struct-or-union = "struct" | "union"
//
// struct-declaration-list = struct-declaration*
// struct-declaration      = specifier-qualifier-list (declspec) struct-declarator ("," struct -declarator)* ";"
//
// struct-declarator = declarator
static Type *stunspec(Token *tkn, Token **end_tkn) {
  TypeKind kind;
  if (consume(tkn, &tkn, "struct")) {
    kind = TY_STRUCT;
  } else if (consume(tkn, &tkn, "union")) {
    kind = TY_UNION;
  } else {
    return NULL;
  }

  char *tag = NULL;
  if (tkn->kind == TK_IDENT) {
    tag = get_ident(tkn);
    tkn = tkn->next;
  }

  if (tag != NULL && !equal(tkn, "{")) {
    Type *ty = find_tag(tag);
    if (ty == NULL) {
      errorf_tkn(ER_COMPILE, tkn, "This struct tag is not declared");
    } else if (ty->kind != kind) {
      errorf_tkn(ER_COMPILE, tkn, "This tag is not an %s tag", kind == TY_STRUCT ? "struct" : "union");
    }

    if (end_tkn != NULL) *end_tkn = tkn;
    return ty;
  }

  Member head = {};
  Member *cur = &head;
  Type *ty = calloc(1, sizeof(Type));

  tkn = skip(tkn, "{");
  while (!consume_close_brace(tkn, &tkn)) {
    if (!is_typename(tkn)) {
      errorf_tkn(ER_COMPILE, tkn, "Need type");
    }

    Type *base_ty = declspec(tkn, &tkn);
    bool is_first = true;

    while (!consume(tkn, &tkn, ";")) {
      if (!is_first) {
        tkn = skip(tkn, ",");
      }
      is_first = false;

      Type *member_ty = declarator(tkn, &tkn, copy_ty(base_ty));
      if (member_ty != NULL) {
        if (find_member(head.next, member_ty->name) != NULL) {
          errorf_tkn(ER_COMPILE, tkn, "Duplicate member");
        }

        Member *member = calloc(1, sizeof(Member));
        member->ty = member_ty;
        member->tkn = member_ty->tkn;
        member->name = member_ty->name;

        if (kind == TY_STRUCT) {
          member->offset = ty->var_size;
          ty->var_size += member_ty->var_size;
        } else {
          if (ty->var_size < member_ty->var_size) {
            ty->var_size = member_ty->var_size;
          }
        }

        cur->next = member;
        cur = member;
      }
    }
  }
  ty->member = head.next;
  ty->kind = kind;

  if (tag != NULL) {
    add_tag(ty, tag);
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return ty;
}

// declaration-specifiers = type-specifier declaration-specifiers?
//                          type-qualifier declaration-specifiers?
//
// type-specifier = "void" | "_Bool | "char" | "short" | "int" | "long" | "double" | "signed" | "unsigned" |
//                  enum-specifier |
//                  struct-or-union-specifier
// type-qualifier = "const"
static Type *declspec(Token *tkn, Token **end_tkn) {
  // We replace the type with a number and count it,
  // which makes it easier to detect duplicates and types.
  // If typename is 'long', we have a duplicate when the long bit
  // and the high bit are 1.
  // Otherwise, we have a duplicate when the high is 1.
  enum {
    VOID     = 1 << 0,
    BOOL     = 1 << 2,
    CHAR     = 1 << 4,
    SHORT    = 1 << 6,
    INT      = 1 << 8,
    LONG     = 1 << 10,
    FLOAT    = 1 << 12,
    DOUBLE   = 1 << 14,
    SIGNED   = 1 << 16,
    UNSIGNED = 1 << 18,
    OTHER    = 1 << 20,
  };

  VarAttr *attr = calloc(1, sizeof(VarAttr));

  int ty_cnt = 0;
  Type *ty = NULL;
  while (is_typename(tkn)) {
    if (typequal(tkn, &tkn, attr)) {
      continue;
    }

    // Counting Types
    if (equal(tkn,"void")) {
      ty_cnt += VOID;
    } else if (equal(tkn, "_Bool")) {
      ty_cnt += BOOL;
    } else if (equal(tkn, "char")) {
      ty_cnt += CHAR;
    } else if (equal(tkn, "short")) {
      ty_cnt += SHORT;
    } else if (equal(tkn, "int")) {
      ty_cnt += INT;
    } else if (equal(tkn, "long")) {
      ty_cnt += LONG;
    } else if (equal(tkn, "float")) {
      ty_cnt += FLOAT;
    } else if (equal(tkn, "double")) {
      ty_cnt += DOUBLE;
    } else if (equal(tkn, "signed")) {
      ty_cnt += SIGNED;
    } else if (equal(tkn, "unsigned")) {
      ty_cnt += UNSIGNED;
    }

    if (equal(tkn, "enum")) {
      ty = enumspec(tkn, &tkn)->base;
      ty_cnt += OTHER;
    }

    if (equal(tkn, "struct") || equal(tkn, "union")) {
      ty = stunspec(tkn, &tkn);
      ty_cnt += OTHER;
    }

    switch (ty_cnt) {
      case VOID:
        ty = ty_void;
        break;
      case BOOL:
      case CHAR:
      case SIGNED + CHAR:
        ty = ty_i8;
        break;
      case UNSIGNED + CHAR:
        ty = ty_u8;
        break;
      case SHORT:
      case SHORT + INT:
      case SIGNED + SHORT:
      case SIGNED + SHORT + INT:
        ty = ty_i16;
        break;
      case UNSIGNED + SHORT:
      case UNSIGNED + SHORT + INT:
        ty = ty_u16;
        break;
      case INT:
      case SIGNED:
      case SIGNED + INT:
        ty = ty_i32;
        break;
      case UNSIGNED:
      case UNSIGNED + INT:
        ty = ty_u32;
        break;
      case LONG:
      case LONG + INT:
      case LONG + LONG:
      case LONG + LONG + INT:
      case SIGNED + LONG:
      case SIGNED + LONG + INT:
      case SIGNED + LONG + LONG:
      case SIGNED + LONG + LONG + INT:
        ty = ty_i64;
        break;
      case UNSIGNED + LONG:
      case UNSIGNED + LONG + INT:
      case UNSIGNED + LONG + LONG:
      case UNSIGNED + LONG + LONG + INT:
        ty = ty_u64;
        break;
      case FLOAT:
        ty = ty_f32;
        break;
      case DOUBLE:
        ty = ty_f64;
        break;
      case LONG + DOUBLE:
        ty = ty_f80;
        break;
      case OTHER:
        continue;
      default:
        errorf_tkn(ER_COMPILE, tkn, "Invalid type");
    }
    tkn = tkn->next;
  }

  if (end_tkn != NULL) *end_tkn = tkn;

  ty = copy_ty(ty);

  if (attr->is_const) {
    ty->is_const = true;
  }

  return ty;
}

// pointer = "*" type-qualifier-list? |
//           "*" type-qualifier-list? pointer |
//
// type-qualifier-list = type-qualifier |
//                       type-qualifier-list type-qualifier
//
// Implement:
// pointer = ("*" typequal*)*
static Type *pointer(Token *tkn, Token **end_tkn, Type *ty) {
  VarAttr *attr = calloc(1, sizeof(VarAttr));

  while (consume(tkn, &tkn, "*")) {
    ty = pointer_to(ty);

    while (typequal(tkn, &tkn, attr));
    if (attr->is_const) {
      ty->is_const = true;
      attr->is_const = false;
    }
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return ty;
}

// array-dimension = constant "]" type-suffix
static Type *array_dimension(Token *tkn, Token **end_tkn, Type *ty) {
  int64_t val = 0;

  if (!consume(tkn, &tkn, "]")) {
    val = constant(tkn, &tkn)->val;
    tkn = skip(tkn, "]");
  }

  return array_to(type_suffix(tkn, end_tkn, ty), val);
}

// param-list = ("void" | param ("," param)*)? ")"
//
// param = declspec declarator
static Type *param_list(Token *tkn, Token **end_tkn, Type *ty) {
  Type *ret_ty = ty;
  ty = calloc(1, sizeof(Type));
  ty->kind = TY_FUNC;
  ty->ret_ty = ret_ty;

  if (equal(tkn, "void") && equal(tkn->next, ")")) {
    if (end_tkn != NULL) *end_tkn = tkn->next->next;
    return ty;
  }

  Type head = {};
  Type *cur = &head;

  while (!consume(tkn, &tkn, ")")) {
    if (cur != &head) {
      tkn = skip(tkn, ",");
    }

    Type *param_ty = declspec(tkn, &tkn);
    param_ty = declarator(tkn, &tkn, param_ty);

    cur->next = param_ty;
    cur = param_ty;
    ty->param_cnt++;
  }

  ty->params = head.next;
  if (end_tkn != NULL) *end_tkn = tkn;
  return ty;
}

// type-suffix = "[" array-dimension |
//               "(" param-list |
//               None
static Type *type_suffix(Token *tkn, Token **end_tkn, Type *ty) {
  if (equal(tkn, "[")) {
    return array_dimension(tkn->next, end_tkn, ty);
  }

  if (equal(tkn, "(")) {
    return param_list(tkn->next, end_tkn, ty);
  }

  if (*end_tkn != NULL) *end_tkn = tkn;
  return ty;
}

// declarator = pointer? direct-declarator
//
// direct-declarator = type-suffix
//
// Implement:
// declarator = pointer? ident type-suffix | None
static Type *declarator(Token *tkn, Token **end_tkn, Type *ty) {
  ty = pointer(tkn, &tkn, ty);

  if (tkn->kind != TK_IDENT) {
    return NULL;
  }

  char *ident = get_ident(tkn);
  ty = type_suffix(tkn->next, end_tkn, ty);
  ty->name = ident;
  return ty;
}

// abstract-declarator = pointer | pointer? direct-abstract-declarator
//
// direct-abstract-declarator = type-suffix
//
// Implement:
// abstract-declarator = pointer? type-suffix
static Type *abstract_declarator(Token *tkn, Token **end_tkn, Type *ty) {
  ty = pointer(tkn, &tkn, ty);
  return type_suffix(tkn, end_tkn, ty);
}

static Initializer *new_initializer(Type *ty, bool is_flexible) {
  Initializer *init = calloc(1, sizeof(Initializer));
  init->ty = ty;

  if (ty->kind == TY_ARRAY) {
    if (is_flexible && ty->var_size == 0) {
      init->is_flexible = true;
    } else {
      init->children = calloc(ty->array_len, sizeof(Initializer *));
      for (int i = 0; i < ty->array_len; i++) {
        init->children[i] = new_initializer(ty->base, false);
      }
    }
  } else if (ty->kind == TY_STRUCT) {
    init->children = calloc(ty->member_cnt, sizeof(Initializer *));

    int i = 0;
    for (Member *member = ty->member; member != NULL; member = member->next) {
      init->children[i] = new_initializer(member->ty, false);
      i++;
    }
  } else if (ty->kind == TY_UNION) {
    init->children = calloc(1, sizeof(Initializer *));
    init->children[0] = new_initializer(ty->member->ty, false);
  }

  return init;
}

static void fill_flexible_initializer(Token *tkn, Type *ty, Initializer *init) {
  Initializer *dummy = new_initializer(ty->base, false);
  tkn = skip(tkn, "{");
  int len = 0;

  while (true) {
    if (len > 0 && !consume(tkn, &tkn, ",")) break;
    if (equal(tkn, "}")) break;
    initializer_only(tkn, &tkn, dummy);
    len++;
  }

  *init = *new_initializer(array_to(init->ty->base, len), false);
}

// string_initializer = string literal
static void string_initializer(Token *tkn, Token **end_tkn, Initializer *init) {
  // If type is not Array, initializer return address of string literal.
  // Otherwise, char number store in each elements of Array.
  if (init->ty->kind == TY_PTR) {
    init->node = new_strlit(tkn);
    if (end_tkn != NULL) *end_tkn = tkn->next;
    return;
  }

  int len = ((Type*)tkn->ty)->array_len;

  if (init->is_flexible) {
    Initializer *tmp = new_initializer(array_to(init->ty->base, len), false);
    *init = *tmp;
  }

  for (int idx = 0; idx < len; idx++) {
    init->children[idx]->node = new_num(tkn, tkn->strlit[idx]);
  }

  tkn = tkn->next;
  if (end_tkn != NULL) *end_tkn = tkn;
}

// array_initializer = "{" initializer_only ("," initializer_only)* ","? "}" | "{" "}"
static void array_initializer(Token *tkn, Token **end_tkn, Initializer *init) {
  if (init->is_flexible) {
    fill_flexible_initializer(tkn, init->ty, init);
  }

  tkn = skip(tkn, "{");
  int idx = 0;

  while (!consume_close_brace(tkn, &tkn)) {
    if (idx != 0) {
      tkn = skip(tkn, ",");
    }
    initializer_only(tkn, &tkn, init->children[idx]);
    idx++;
  }
  if (end_tkn != NULL) *end_tkn = tkn;
}

// initializer = array_initializer | string_initializer | assign
static void initializer_only(Token *tkn, Token **end_tkn, Initializer *init) {
  if (tkn->kind == TK_STR) {
    string_initializer(tkn, &tkn, init);
  } else if (init->ty->kind == TY_ARRAY || init->ty->kind == TY_STRUCT || init->ty->kind == TY_UNION) {
    array_initializer(tkn, &tkn, init);
  } else {
    init->node = assign(tkn, &tkn);
  }

  if (end_tkn != NULL) *end_tkn = tkn;
}

static Initializer *initializer(Token *tkn, Token **end_tkn, Type *ty) {
  Initializer *init = new_initializer(ty, true);
  initializer_only(tkn, end_tkn, init);
  return init;
}

static void create_init_node(Initializer *init, Node **connect, bool only_const, Type *ty) {
  if (init->children == NULL) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_INIT;
    node->init = init->node;
    node->ty = ty;

    if (init->node != NULL && !is_same_type(ty, init->node->ty)) {
      node->init = new_cast(init->node, ty);
    }

    if (only_const && init->node != NULL) {
      char *label = NULL;

      if (is_float_ty(node->init->ty)) {
        node->init = new_floating(node->init->tkn, node->init->ty, eval_double(node->init));
      } else {
        node->init = new_num(init->node->tkn, eval_expr2(node->init, &label));
      }

      node->init->var = calloc(1, sizeof(Obj));
      node->init->var->name = label;
    }

    (*connect)->lhs = node;
    *connect = node;
    return;
  }

  if (ty->kind == TY_ARRAY) {
    for (int i = 0; i < init->ty->array_len; i++) {
      create_init_node(init->children[i], connect, only_const, ty->base);
    }
  } else if (ty->kind == TY_STRUCT) {
    int i = 0;
    for (Member *member = ty->member; member != NULL; member = member->next) {
      create_init_node(init->children[i], connect, only_const, member->ty);
      i++;
    }
  } else if (ty->kind == TY_UNION) {
    create_init_node(init->children[0], connect, only_const, ty->member->ty);
  }

  return;
}

static int64_t eval_expr(Node *node) {
  return eval_expr2(node, NULL);
}

// Evaluate a given node as a constant expression;
static int64_t eval_expr2(Node *node, char **label) {
  add_type(node);

  switch (node->kind) {
    case ND_ADD:
      return eval_expr2(node->lhs, label) + eval_expr2(node->rhs, label);
    case ND_SUB: {
      char *dummyl = NULL, *dummyr = NULL;
      int64_t val = eval_expr2(node->lhs, &dummyl) - eval_expr2(node->rhs, &dummyr);

      if (dummyl != NULL && dummyr != NULL) {
        if (strcmp(dummyl, dummyr) != 0) {
          errorf_tkn(ER_COMPILE, node->tkn, "Initializer element is not computable at load time");
        }
        dummyl = NULL;
        dummyr = NULL;
      }

      if (dummyl == NULL) {
        dummyr = dummyl;
      }

      if (label == NULL && dummyl != NULL) {
        errorf_tkn(ER_COMPILE, node->tkn, "Pointer is not allowed");
      }

      if (label != NULL) {
        *label = dummyl;
      }
      return val;
    }
    case ND_MUL:
      return eval_expr(node->lhs) * eval_expr(node->rhs);
    case ND_DIV:
      if (node->ty->is_unsigned) {
        return (uint64_t)eval_expr(node->lhs) / eval_expr(node->rhs);
      }
      return eval_expr(node->lhs) / eval_expr(node->rhs);
    case ND_REMAINDER:
      if (node->ty->is_unsigned) {
        return (uint64_t)eval_expr(node->lhs) % eval_expr(node->rhs);
      }
      return eval_expr(node->lhs) % eval_expr(node->rhs);
    case ND_EQ:
      if (is_float_ty(node->lhs->ty)) {
        return eval_double(node->lhs) == eval_double(node->rhs);
      }
      return eval_expr(node->lhs) == eval_expr(node->rhs);
    case ND_NEQ:
      if (is_float_ty(node->lhs->ty)) {
        return eval_double(node->lhs) != eval_double(node->rhs);
      }
      return eval_expr(node->lhs) != eval_expr(node->rhs);
    case ND_LC:
      if (is_float_ty(node->lhs->ty)) {
        return eval_double(node->lhs) < eval_double(node->rhs);
      }

      if (node->lhs->ty->is_unsigned) {
        return (uint64_t)eval_expr(node->lhs) < eval_expr(node->rhs);
      }
      return eval_expr(node->lhs) < eval_expr(node->rhs);
    case ND_LEC:
      if (is_float_ty(node->lhs->ty)) {
        return eval_double(node->lhs) <= eval_double(node->rhs);
      }

      if (node->lhs->ty->is_unsigned) {
        return (uint64_t)eval_expr(node->lhs) <= eval_expr(node->rhs);
      }
      return eval_expr(node->lhs) <= eval_expr(node->rhs);
    case ND_LEFTSHIFT:
      return eval_expr(node->lhs) << eval_expr(node->rhs);
    case ND_RIGHTSHIFT:
      if (node->lhs->ty->is_unsigned && node->lhs->ty->var_size == 8) {
        return (uint64_t)eval_expr(node->lhs) >> eval_expr(node->rhs);
      }
      return eval_expr(node->lhs) >> eval_expr(node->rhs);
    case ND_BITWISEAND:
      return eval_expr(node->lhs) & eval_expr(node->rhs);
    case ND_BITWISEOR:
      return eval_expr(node->lhs) | eval_expr(node->rhs);
    case ND_BITWISEXOR:
      return eval_expr(node->lhs) ^ eval_expr(node->rhs);
    case ND_BITWISENOT:
      return ~eval_expr(node->lhs);
    case ND_LOGICALAND:
      if (is_float_ty(node->lhs->ty)) {
        return eval_double(node->lhs) && eval_double(node->rhs);
      }

      return eval_expr(node->lhs) && eval_expr(node->rhs);
    case ND_LOGICALOR:
      if (is_float_ty(node->lhs->ty)) {
        return eval_double(node->lhs) || eval_double(node->rhs);
      }

      return eval_expr(node->lhs) || eval_expr(node->rhs);
    case ND_COND:
      return eval_expr(node->cond) ? eval_expr2(node->lhs, label) : eval_expr2(node->rhs, label);
    case ND_VAR:
      if (label != NULL && node->ty->kind == TY_ARRAY) {
        *label = node->var->name;
        return 0;
      }

      if (is_addr_node(node)) {
        break;
      }

      return node->var->val;
    case ND_CAST: {
      if (is_float_ty(node->lhs->ty)) {
        return (int64_t)eval_double(node->lhs);
      }

      int64_t val = eval_expr2(node->lhs, label);
      switch (node->ty->kind) {
        case TY_CHAR:
          return node->ty->is_unsigned ? (uint8_t)val : (int8_t)val;
        case TY_SHORT:
          return node->ty->is_unsigned ? (uint16_t)val : (int16_t)val;
        case TY_INT:
          return node->ty->is_unsigned ? (uint32_t)val : (int32_t)val;
      }
      return val;
    }
    case ND_ADDR:
      return eval_addr(node->lhs, label);
    case ND_CONTENT:
      return eval_expr2(node->lhs, label);
    case ND_NUM:
      return node->val;
  }
  errorf_tkn(ER_COMPILE, node->tkn, "Not a compiler-time constant");
}

static int64_t eval_addr(Node *node, char **label) {
  switch (node->kind) {
    case ND_VAR:
      if (!node->var->is_global) {
        break;
      }
      if (label == NULL) {
        errorf_tkn(ER_COMPILE, node->tkn, "Pointer is not allowed");
      }

      *label = node->var->name;
      return 0;
    case ND_CONTENT:
      return eval_expr2(node->lhs, label);
  }

  errorf_tkn(ER_COMPILE, node->tkn, "Not a compiler-time constant");
}

static long double eval_double(Node *node) {
  add_type(node);

  switch (node->kind) {
    case ND_ADD:
      return eval_double(node->lhs) + eval_double(node->rhs);
    case ND_SUB:
      return eval_double(node->lhs) - eval_double(node->rhs);
    case ND_MUL:
      return eval_double(node->lhs) * eval_double(node->rhs);
    case ND_DIV:
      return eval_double(node->lhs) / eval_double(node->rhs);
    case ND_VAR:
      if (!node->var->is_global) {
        break;
      }

      return node->var->fval;
    case ND_CAST:
      if (is_float_ty(node->lhs->ty)) {
        return eval_double(node->lhs);
      }
      return (long double)eval_expr(node->lhs);
    case ND_NUM:
      return node->fval;
  }

  errorf_tkn(ER_COMPILE, node->tkn, "Not a compiler-time constant");
}

// declaration = declaration-specifiers init-declarator-list?
//
// init-declarator-list = init-declarator | 
//                        init-declarator-list "," init-declarator
//
// declaration -> init-declarator ("," init-declarator)* ";"?
// declspec is replaced as base_ty argument.
static Node *declaration(Token *tkn, Token **end_tkn, Type *base_ty, bool is_global) {
  Node head = {};
  Node *cur = &head;

  while (!consume(tkn, &tkn, ";")) {
    if (cur != &head) {
      tkn = skip(tkn, ",");
    }
    cur->next = initdecl(tkn, &tkn, base_ty, is_global);
    cur = cur->next;
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return head.next;
}

// init-declarator = declarator ("=" initializer)?
static Node *initdecl(Token *tkn, Token **end_tkn, Type *ty, bool is_global) {
  ty = declarator(tkn, &tkn, ty);
  Obj *obj = new_obj(ty, ty->name);

  if (ty->kind == TY_FUNC) {
    if (!declare_func(ty)) {
      errorf_tkn(ER_COMPILE, tkn, "Conflict declaration");
    }
  } else {
    obj->is_global = is_global;
    add_var(obj, !is_global);
  }

  if (ty->kind == TY_FUNC) {
    Node *node = new_node(ND_FUNC, tkn);
    node->func = obj;

    if (end_tkn != NULL) *end_tkn = tkn;
    return node;
  }

  if (equal(tkn, "=")) {
    Initializer *init = initializer(tkn->next, &tkn, obj->ty);
    obj->ty = init->ty;

    Node head = {};
    Node *cur = &head;

    create_init_node(init, &cur, is_global, obj->ty);
    Node *node = new_node(ND_INIT, tkn);
    node->lhs = new_var(tkn, obj);
    node->rhs = head.lhs;

    // If the lengh of the array is empty, Type will be updated,
    // so it needs to be passed to var as well.
    node->ty = init->ty;

    if (is_global) {
      node->lhs->var->val = node->rhs->init->val;
      node->lhs->var->fval = node->rhs->init->fval;
    }

    if (end_tkn != NULL) *end_tkn = tkn;
    return node;
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return new_var(tkn, obj);
}

Node *last_stmt(Node *now) {
  while (now->next != NULL) {
    now = now->next;
  }
  return now;
}

// translation-unit     = external-declaration | translation-unit external-declaration
// external-declaration = function-definition | declaration
//
// program -> (declspec (funcdef | declaration))*
Node *program(Token *tkn) {
  Node head = {};
  Node *cur = &head;

  while (!is_eof(tkn)) {
    if (!is_typename(tkn)) {
      errorf_tkn(ER_COMPILE, tkn, "Need type name");
    }

    Type *ty = declspec(tkn, &tkn);
    Node *node = funcdef(tkn, &tkn, ty);
    if (node == NULL) {
      node = declaration(tkn, &tkn, ty, true);
    }

    cur->next = literal_node;
    cur = last_stmt(cur);
    cur->next = node;
    cur = last_stmt(cur);

    literal_node = NULL;
  }
  return head.next;
}

// function-definition = declaration-specifiers declarator declaration-list? compound-statement
//
// declaration-list = declaration |
//
// funcdef -> declarator comp-stmt | None
//
static Node *funcdef(Token *tkn, Token **end_tkn, Type *base_ty) {
  label_node = NULL;
  goto_node = NULL;
  label_map = calloc(1, sizeof(HashMap));

  Type *ty = declarator(tkn, &tkn, base_ty);
  if (!equal(tkn, "{")) {
    return NULL;
  }

  if (!define_func(ty)) {
    errorf_tkn(ER_COMPILE, tkn, "Conflict define");
  }

  Node *node = new_node(ND_FUNC, tkn);
  enter_scope();
  ty->is_prototype = false;

  Obj *func = find_var(ty->name);
  node->func = func;

  Obj head = {};
  Obj *cur = &head;

  for (Type *param = ty->params; param != NULL; param = param->next) {
    cur->next = new_obj(param, param->name);
    cur = cur->next;
    add_var(cur, true);
  }
  func->params = head.next;

  node->deep = comp_stmt(tkn, &tkn);
  leave_scope();
  node->func->vars_size = init_offset();

  // Relocate label
  for (Node *stmt = label_node; stmt != NULL; stmt = stmt->deep) {
    stmt->label = hashmap_get(label_map, stmt->label);
  }

  for (Node *stmt = goto_node; stmt != NULL; stmt = stmt->deep) {
    char *label = hashmap_get(label_map, stmt->label);
    if (label == NULL) {
      errorf_tkn(ER_COMPILE, stmt->tkn, "Label '%s' is not defined", stmt->label);
    }
    stmt->label = label;
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return node;
}

// statement = labeled-statement |
//             compound-statement |
//             selection-statement |
//             iteratoin-statement |
//             jump-statement |
//             expression-statement
// labeled-statement    = identifier ":" statement
//                        "case" constant-expression ":" statement
//                        "default" ":" statement
// selection-statement  = "if" "(" expression ")" statement ("else" statement)?
//                        "switch" "(" expression ")" statement
// iteration-statement  = "while" "(" expression ")" statement |
//                        "do" statement "while" "(" expression ")" ";" |
//                        "for" "(" declaration expression? ";" expression? ")" statement |
//                        "for" "(" expression? ";" expression? ";" expression? ")" statement
// jump-statement       = "goto" identifier ";" |
//                        "continue;" |
//                        "break" |
//                        "return" expr? ";"
// expression-statement = expression? ";"
static Node *statement(Token *tkn, Token **end_tkn) {
  if (tkn->kind == TK_IDENT && equal(tkn->next, ":")) {
    Node *node = new_node(ND_LABEL, tkn);
    node->label = get_ident(tkn);

    if (hashmap_get(label_map, node->label) != NULL) {
      errorf_tkn(ER_COMPILE, tkn, "Duplicate label");
    }
    hashmap_insert(label_map, node->label, new_unique_label());
    tkn = skip(tkn->next, ":");

    node->deep = label_node;
    label_node = node;

    node->next = statement(tkn, end_tkn);
    return node;
  }

  // labeled-statement
  if (equal(tkn, "case")) {
    Node *node = new_node(ND_CASE, tkn);
    node->val = eval_expr(expr(tkn->next, &tkn));
    tkn = skip(tkn, ":");

    enter_scope();
    node->deep = statement(tkn, end_tkn);
    leave_scope();

    return node;
  }

  if (equal(tkn, "default")) {
    Node *node = new_node(ND_DEFAULT, tkn);
    tkn = skip(tkn->next, ":");

    enter_scope();
    node->deep = statement(tkn, end_tkn);
    leave_scope();

    return node;
  }

  // compound-statement
  Node *node = comp_stmt(tkn, &tkn);
  if (node != NULL) {
    if (end_tkn != NULL) *end_tkn = tkn;
    return node;
  }

  // selection-statement
  if (equal(tkn, "if")) {
    tkn = skip(tkn->next, "(");
    Node *ret = new_node(ND_IF, tkn);
    ret->cond = assign(tkn, &tkn);
    tkn = skip(tkn, ")");

    enter_scope();
    ret->then = statement(tkn, &tkn);
    leave_scope();

    if (equal(tkn, "else")) {
      enter_scope();
      ret->other = statement(tkn->next, &tkn);
      leave_scope();
    }

    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  // selection-statement
  if (equal(tkn, "switch")) {
    char *break_store = break_label;

    Node *node = new_node(ND_SWITCH, tkn);
    break_label = node->break_label = new_unique_label();

    tkn = skip(tkn->next, "(");
    node->cond = expr(tkn, &tkn);
    add_type(node->cond);
    if (!is_integer_ty(node->cond->ty)) {
      errorf_tkn(ER_COMPILE, tkn, "Statemnet require expression of integer type");
    }
    tkn = skip(tkn, ")");
    
    enter_scope();
    Node *stmt = statement(tkn, end_tkn);
    leave_scope();

    if (stmt->kind == ND_BLOCK) {
      Node head = {};
      Node *cur = &head;

      for (Node *expr = stmt->deep; expr != NULL; expr = expr->next) {
        if (expr->kind == ND_CASE) {
          cur->case_stmt = expr;
          cur = expr;

          // Duplicate check
          for (Node *before = head.case_stmt; before != expr; before = before->case_stmt) {
            if (expr->val == before->val) {
              errorf_tkn(ER_COMPILE, expr->tkn, "Duplicate case value");
            }
          }
        }

        if (expr->kind == ND_DEFAULT) {
          if (node->default_stmt != NULL) {
            errorf_tkn(ER_COMPILE, expr->tkn, "Duplicate default label");
          }
          node->default_stmt = expr;
        }
      }
      node->case_stmt = head.case_stmt;
      node->lhs = stmt->deep;
    }

    break_label = break_store;
    return node;
  }

  // iteration-statement
  if (equal(tkn, "while")) {
    tkn = skip(tkn->next, "(");
    enter_scope();

    char *break_store = break_label;
    char *conti_store = conti_label;
    
    Node *ret = new_node(ND_FOR, tkn);
    break_label = ret->break_label = new_unique_label();
    conti_label = ret->conti_label = new_unique_label();

    ret->cond = expr(tkn, &tkn);

    tkn = skip(tkn, ")");

    ret->then = statement(tkn, &tkn);
    leave_scope();

    break_label = break_store;
    conti_label = conti_store;
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  // iteration-statement
  if (equal(tkn, "do")) {
    char *break_store = break_label;
    char *conti_store = conti_label;

    Node *node = new_node(ND_DO, tkn);
    break_label = node->break_label = new_unique_label();
    conti_label = node->conti_label = new_unique_label();

    enter_scope();
    node->then = statement(tkn->next, &tkn);
    leave_scope();

    tkn = skip(skip(tkn, "while"), "(");
    node->cond = expr(tkn, &tkn);
    tkn = skip(skip(tkn, ")"), ";");

    break_label = break_store;
    conti_label = conti_store;
    if (end_tkn != NULL) *end_tkn = tkn;
    return node;
  }

  // iteration-statement
  if (equal(tkn, "for")) {
    tkn = skip(tkn->next, "(");
    enter_scope();

    char *break_store = break_label;
    char *conti_store = conti_label;
    
    Node *ret = new_node(ND_FOR, tkn);
    break_label = ret->break_label = new_unique_label();
    conti_label = ret->conti_label = new_unique_label();

    if (is_typename(tkn)) {
      Type *ty = declspec(tkn, &tkn);
      ret->init = declaration(tkn, &tkn, ty, false);
    } else if (!consume(tkn, &tkn, ";")) {
      ret->init = expr(tkn, &tkn);
      tkn = skip(tkn, ";");
    }

    if (!consume(tkn, &tkn, ";")) {
      ret->cond = expr(tkn, &tkn);
      tkn = skip(tkn, ";");
    }

    if (!consume(tkn, &tkn, ")")) {
      ret->loop = assign(tkn, &tkn);
      tkn = skip(tkn, ")");
    }

    ret->then = statement(tkn, &tkn);
    leave_scope();

    break_label = break_store;
    conti_label = conti_store;
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  // jump-statement
  if (equal(tkn, "goto")) {
    Node *node = new_node(ND_GOTO, tkn);
    node->label = get_ident(tkn->next);
    node->deep = goto_node;
    goto_node = node;

    tkn = skip(tkn->next->next, ";");
    if (end_tkn != NULL) *end_tkn = tkn;
    return node;
  }

  // jump-statement
  if (equal(tkn, "continue")) {
    if (conti_label == NULL) {
      errorf_tkn(ER_COMPILE, tkn, "There is no jump destination");
    }

    Node *ret = new_node(ND_CONTINUE, tkn);
    ret->conti_label = conti_label;
    tkn = skip(tkn->next, ";");

    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  // jump-statement
  if (equal(tkn, "break")) {
    if (break_label == NULL) {
      errorf_tkn(ER_COMPILE, tkn, "There is no jump destination");
    }

    Node *ret = new_node(ND_BREAK, tkn);
    ret->break_label = break_label;
    tkn = skip(tkn->next, ";");

    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  // jump-statement
  if (equal(tkn, "return")) {
    Node *ret = new_node(ND_RETURN, tkn);
    ret->lhs = assign(tkn->next, &tkn);
    tkn = skip(tkn, ";");

    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  // expression-statement
  while (consume(tkn, &tkn, ";"));
  node = expr(tkn, &tkn);
  tkn = skip(tkn, ";");

  if (end_tkn != NULL) *end_tkn = tkn;
  return node;
}

// compound-statement = "{" ( declaration | statement )* "}"
//                    -> "{" (declspec declaration | statement )* "}"
static Node *comp_stmt(Token *tkn, Token **end_tkn) {
  if (consume(tkn, &tkn, "{")) {
    Node *ret = new_node(ND_BLOCK, tkn);

    Node head = {};
    Node *cur = &head;

    while (!consume(tkn, &tkn, "}")) {
      if (is_typename(tkn)) {
        Type *ty = declspec(tkn, &tkn);
        cur->next = declaration(tkn, &tkn, ty, false);
      } else {
        enter_scope();
        cur->next = statement(tkn, &tkn);
        leave_scope();
      }
      cur = last_stmt(cur);
    }

    ret->deep = head.next;
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  return NULL;
}

// expression = assignment-expression
static Node *expr(Token *tkn, Token **end_tkn) {
  return assign(tkn, end_tkn);
}

// assignment-expression = conditional-expression
//                         unary-expression assignment-operator assignment-expression
//
//                         implement:
//                         conditional-expression (assignment-operator assignment-expression)?
//
// assignment-operator   = "=" | "*=" | "/=" | "%=" | "+=" | "-=" | "<<=" | ">>=" | "&=" | "^=" | "|="
//
// Since conditional-expression encompassess unary-expression, for simplicity of implementation,
// unary-expression is implemented as conditional-expression.
static Node *assign(Token *tkn, Token **end_tkn) {
  Node *node = conditional(tkn, &tkn);

  if (equal(tkn, "=")) {
    return new_assign(tkn, node, assign(tkn->next, end_tkn));
  }

  if (equal(tkn, "+=")) {
    return to_assign(tkn, new_add(tkn, node, assign(tkn->next, end_tkn)));
  }

  if (equal(tkn, "-=")) {
    return to_assign(tkn, new_sub(tkn, node, assign(tkn->next, end_tkn)));
  }

  if (equal(tkn, "*=")) {
    return to_assign(tkn, new_calc(ND_MUL, tkn, node, assign(tkn->next, end_tkn)));
  }

  if (equal(tkn, "/=")) {
    return to_assign(tkn, new_calc(ND_DIV, tkn, node, assign(tkn->next, end_tkn)));
  }

  if (equal(tkn, "%=")) {
    return to_assign(tkn, new_calc(ND_REMAINDER, tkn, node, assign(tkn->next, end_tkn)));
  }

  if (equal(tkn, "<<=")) {
    return to_assign(tkn, new_calc(ND_LEFTSHIFT, tkn, node, assign(tkn->next, end_tkn)));
  }

  if (equal(tkn, ">>=")) {
    return to_assign(tkn, new_calc(ND_RIGHTSHIFT, tkn, node, assign(tkn->next, end_tkn)));
  }

  if (equal(tkn, "&=")) {
    return to_assign(tkn, new_calc(ND_BITWISEAND, tkn, node, assign(tkn->next, end_tkn)));
  }

  if (equal(tkn, "^=")) {
    return to_assign(tkn, new_calc(ND_BITWISEXOR, tkn, node, assign(tkn->next, end_tkn)));
  }

  if (equal(tkn, "|=")) {
    return to_assign(tkn, new_calc(ND_BITWISEOR, tkn, node, assign(tkn->next, end_tkn)));
  }

  add_type(node);
  if (end_tkn != NULL) *end_tkn = tkn;
  return node;
}
// conditional-expression = logical-OR-expression |
//                          logical-OR-expression "?" expression ":" conditional-expression
static Node *conditional(Token *tkn, Token **end_tkn) {
  Node *node = logical_or(tkn, &tkn);

  if (equal(tkn, "?")) {
    Node *cond_expr = new_node(ND_COND, tkn);
    cond_expr->cond = node;
    cond_expr->lhs = expr(tkn->next, &tkn);
    
    tkn = skip(tkn, ":");

    cond_expr->rhs = conditional(tkn, &tkn);
    node = cond_expr;
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return node;
}

// logical-OR-expression = logical-AND-expression |
//                         logical-OR-expression "||" logical-AND-expression
//
//                         implement:
//                         logical-AND-expression ( "||" logical-ANd-expression)
static Node *logical_or(Token *tkn, Token **end_tkn) {
  Node *ret = logical_and(tkn, &tkn);

  while (equal(tkn, "||")) {
    Token *operand = tkn;
    ret = new_calc(ND_LOGICALOR, operand, ret, logical_and(tkn->next, &tkn));
    ret->lhs = new_calc(ND_NEQ, operand, ret->lhs, new_num(operand, 0));
    ret->rhs = new_calc(ND_NEQ, operand, ret->rhs, new_num(operand, 0));
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// logical-AND-expression = inclusive-OR-expression | 
//                          logical-AND-expression "&&" inclusive-OR-expression
//
//                          implement:
//                          inclusive-OR-expression ( "&&" inclusive-OR-expression)
static Node *logical_and(Token *tkn, Token **end_tkn) {
  Node *ret = bitor(tkn, &tkn);

  while (equal(tkn, "&&")) {
    Token *operand = tkn;
    ret = new_calc(ND_LOGICALAND, operand, ret, bitor(tkn->next, &tkn));
    ret->lhs = new_calc(ND_NEQ, operand, ret->lhs, new_num(operand, 0));
    ret->rhs = new_calc(ND_NEQ, operand, ret->rhs, new_num(operand, 0));
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// inclusive-OR-expression = exclusive-OR-expression |
//                           inclusive-OR-expression "|" exclusive-OR-expression
//
//                           implement:
//                           exclusive-OR-expression ( "|" exclusive-OR-epxression )*
static Node *bitor(Token *tkn, Token **end_tkn) {
  Node *ret = bitxor(tkn, &tkn);

  while (equal(tkn, "|")) {
    Token *operand = tkn;
    ret = new_calc(ND_BITWISEOR, operand, ret, bitxor(tkn->next, &tkn));
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// exclusive-OR-expression = AND-expression |
//                           exclusive-OR-expression "^" AND-expression
//
//                           implement:
//                           AND-expression ( "^" AND-expression )*
static Node *bitxor(Token *tkn, Token **end_tkn) {
  Node *ret = bitand(tkn, &tkn);

  while (equal(tkn, "^")) {
    Token *operand = tkn;
    ret = new_calc(ND_BITWISEXOR, operand, ret, bitand(tkn->next, &tkn));
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// AND-expression = equality-expression |
//                  AND-expression "&" equality-expression
//
//                  implement:
//                  equality-expression ( "&" equality-expression )*
static Node *bitand(Token *tkn, Token **end_tkn) {
  Node *ret = equality(tkn, &tkn);

  while (equal(tkn, "&")) {
    Token *operand = tkn;
    ret = new_calc(ND_BITWISEAND, operand, ret, equality(tkn->next, &tkn));
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// equality-expression = relational-expression |
//                       equality-expression "==" relational-expression |
//                       equality-expression "!=" relational-expression |
//                       
//                       implement:
//                       relational-expression ( ( "==" | "!=") relational-expression )*
static Node *equality(Token *tkn, Token **end_tkn) {
  Node *node = relational(tkn, &tkn);

  while (equal(tkn, "==") || equal(tkn, "!=")) {
    NodeKind kind = equal(tkn, "==") ? ND_EQ : ND_NEQ;

    Node *eq_expr = new_node(kind, tkn);
    eq_expr->lhs = node;
    eq_expr->rhs = relational(tkn->next, &tkn);
    node = eq_expr;
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return node;
}

// relational-expression = shift-expression |
//                         relational-expression "<" shift-expression
//                         relational-expression ">" shift-expression
//                         relational-expression "<=" shift-expression
//                         relational-expression ">=" shift-expression
//
//                         implement:
//                         shift-expression ( ( "<" | ">" | "<=" | ">=" ) shift-expression )*
static Node *relational(Token *tkn, Token **end_tkn) {
  Node *node = bitshift(tkn, &tkn);

  while (equal(tkn, "<") || equal(tkn, ">") ||
         equal(tkn, "<=") || equal(tkn, ">=")) {
    NodeKind kind = equal(tkn, "<") || equal(tkn, ">") ? ND_LC : ND_LEC;

    Node *rel_expr = new_node(kind, tkn);

    if (equal(tkn, ">") || equal(tkn, ">=")) {
      rel_expr->lhs = bitshift(tkn->next, &tkn);
      rel_expr->rhs = node;
    } else {
      rel_expr->lhs = node;
      rel_expr->rhs = bitshift(tkn->next, &tkn);
    }
    node = rel_expr;
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return node;
}

// shift-expression = additive-expression | 
//                    shift-expression "<<" additive-expression
//                    shift-expression ">>" additive-expression
//
//                    implement:
//                    additive-expression ( ( "<<" | ">>" ) additive-expression )*
static Node *bitshift(Token *tkn, Token **end_tkn) {
  Node *ret = add(tkn, &tkn);

  while (equal(tkn, "<<") || equal(tkn, ">>")) {
    NodeKind kind = equal(tkn, "<<") ? ND_LEFTSHIFT : ND_RIGHTSHIFT;
    Token *operand = tkn;
    ret = new_calc(kind, operand, ret, add(tkn->next, &tkn));
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// additive-expression = multiplicative-expression |
//                       additive-expression "+" multiplicative-expression |
//                       additive-expression "-" multiplicative-expression
//
//                       implement:
//                       multiplicative-expression ( ( "+" | "-" ) multiplicative-expression )*
// add = mul ("+" mul | "-" mul)*
static Node *add(Token *tkn, Token **end_tkn) {
  Node *ret = mul(tkn, &tkn);

  while (equal(tkn, "+") || equal(tkn, "-")) {
    Token *operand = tkn;
    if (equal(tkn, "+")) {
      ret = new_add(operand, ret, mul(tkn->next, &tkn));
    }

    if (equal(tkn, "-")) {
      ret = new_sub(operand, ret, mul(tkn->next, &tkn));
    }
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return ret;
}

// multiplicative-expression = cast-expression |
//                             multiplicative-expression "*" cast-expression
//                             multiplicative-expression "/" cast-expression
//                             multiplicative-expresioon "%" cast-expression
//
//                             implement:
//                             cast-expression ( ( "*" | "/" | "%") cast-expression )*
static Node *mul(Token *tkn, Token **end_tkn) {
  Node *node = cast(tkn, &tkn);

  while (equal(tkn, "*") || equal(tkn, "/") || equal(tkn, "%")) {
    NodeKind kind = ND_VOID;

    if (equal(tkn, "*")) {
      kind = ND_MUL;
    } else if (equal(tkn, "/")) {
      kind = ND_DIV;
    } else {
      kind = ND_REMAINDER;
    }

    Node *mul_node = new_node(kind, tkn);
    mul_node->lhs = node;
    mul_node->rhs = cast(tkn->next, &tkn);
    
    node = mul_node;
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return node;
}

// cast-expression = unary-expression | 
//                   "(" type-name ")" cast-expression
//
// The definition of typename is shown in the comments of the function below.
static Node *cast(Token *tkn, Token **end_tkn) {
  if (equal(tkn, "(") && is_typename(tkn->next)) {
    Type *ty = declspec(tkn->next, &tkn);
    ty = abstract_declarator(tkn, &tkn, ty);

    tkn = skip(tkn, ")");
    return new_cast(cast(tkn, end_tkn), ty);
  }
  return unary(tkn, end_tkn);
}


// unary-expression = postfix-expression |
//                    "++" unary-expression |
//                    "--" unary-expression | 
//                    unary-operator cast-expression |
//                    "sizeof" unary-expression |
//                    "sizeof" "(" type-name ")"
//
// unary-operator   = "&" | "*" | "+" | "-" | "~" | "!"
// typename =  specifier-qualifier-list abstruct-declarator?
//          -> declspec abstract-declarator
static Node *unary(Token *tkn, Token **end_tkn) {
  if (equal(tkn, "++")) {
    Node *node = to_assign(tkn, new_add(tkn, unary(tkn->next, end_tkn), new_num(tkn, 1)));
    node->deep = node->lhs;
    return node;
  }

  if (equal(tkn, "--")) {
    Node *node = to_assign(tkn, new_sub(tkn, unary(tkn->next, end_tkn), new_num(tkn, 1)));
    node->deep = node->lhs;
    return node;
  }

  // unary-operator
  if (equal(tkn, "&")) {
    Node *node = new_node(ND_ADDR, tkn);
    node->lhs = cast(tkn->next, end_tkn);
    return node;
  }

  if (equal(tkn, "*")) {
    Node *node = new_node(ND_CONTENT, tkn);
    node->lhs = cast(tkn->next, end_tkn);
    return node;
  }

  if (equal(tkn, "+") || equal(tkn, "-")) {
    NodeKind kind = equal(tkn, "+") ? ND_ADD : ND_SUB;
    Node *node = new_node(kind, tkn);
    node->lhs = new_num(tkn, 0);
    node->rhs = cast(tkn->next, end_tkn);
    return node;
  }

  if (equal(tkn, "~")) {
    Node *node = new_node(ND_BITWISENOT, tkn);
    node->lhs = cast(tkn->next, end_tkn);
    return node;
  }

  if (equal(tkn, "!")) {
    return new_calc(ND_EQ, tkn, cast(tkn->next, end_tkn), new_num(tkn, 0));
  }

  if (equal(tkn, "sizeof")) {
    tkn = tkn->next;

    // type-name
    if (equal(tkn, "(") && is_typename(tkn->next)) {
      Type *ty = declspec(tkn->next, &tkn);
      ty = abstract_declarator(tkn, &tkn, ty);

      tkn = skip(tkn, ")");
      if (end_tkn != NULL) *end_tkn = tkn;
      return new_num(tkn, ty->var_size);
    }

    Node *node = unary(tkn, end_tkn);
    add_type(node);
    return new_num(tkn, node->ty->var_size);
  }

  return postfix(tkn, end_tkn);
}

// postfix-expression       = primary-expression |
//                            postfix-expression "[" expression "]" |
//                            postfix-expression "(" argument-expression-list? ")" |
//                            postfix-expression "++" |
//                            postfix-expression "--"
//                            postfix-expression "."  identifier
//                            postfix-expression "->" identifier
//
// argument-expression-list = assignment-expression |
//                            argument-expression-list "," assignment-expression
//
//                            implement:
//                            assignment-expression ( "," assignment-expression )*
static Node *postfix(Token *tkn, Token **end_tkn) {
  Node *node = primary(tkn, &tkn);

  while (equal(tkn, "[") || equal(tkn, "(") || equal(tkn, "++") ||
         equal(tkn, "--") || equal(tkn, ".") || equal(tkn, "->")) {
    if (equal(tkn, "[")) {
      node = new_binary(ND_CONTENT, tkn, new_add(tkn, node, assign(tkn->next, &tkn)));
      tkn = skip(tkn, "]");
      continue;
    }

    if (equal(tkn, "(")) {
      node = new_binary(ND_FUNCCALL, tkn, node);
      node->func = node->lhs->var;
      node->ty = node->lhs->var->ty;
      node->lhs = NULL;
      tkn = tkn->next;

      int argc = 0;
      Node head = {};
      Node *cur = &head;

      while (!consume(tkn, &tkn, ")")) {
        if (cur != &head) {
          tkn = skip(tkn, ",");
        }

        cur->next = new_binary(ND_VOID, tkn, assign(tkn, &tkn));
        cur = cur->next;
        argc++;
      }

      if (node->ty->param_cnt != 0 && node->ty->param_cnt != argc) {
        errorf_tkn(ER_COMPILE, tkn, "Do not match arguments to function call, expected %d, have %d", node->ty->param_cnt, argc);
      }

      cur = head.next;
      for (Type *arg_ty = node->ty->params; arg_ty != NULL; arg_ty = arg_ty->next) {
        cur->lhs = new_cast(cur->lhs, arg_ty);
        cur = cur->next;
      }

      node->args = head.next;
      continue;
    }

    if (equal(tkn, "++")) {
      node = to_assign(tkn, new_add(tkn, node, new_num(tkn, 1)));
      node->deep = new_sub(tkn, node->lhs, new_num(tkn, 1));
      tkn = tkn->next;
      continue;
    }

    if (equal(tkn, "--")) {
      node = to_assign(tkn, new_sub(tkn, node, new_num(tkn, 1)));
      node->deep = new_add(tkn, node->lhs, new_num(tkn, 1));
      tkn = tkn->next;
      continue;
    }

    // "." or "->" operator
    add_type(node);

    if (equal(tkn, "->") && node->ty->kind != TY_PTR) {
      errorf_tkn(ER_COMPILE, tkn, "Need pointer type");
    }

    Type *ty = equal(tkn, ".") ? node->ty : node->ty->base;
    if (ty->kind != TY_STRUCT && ty->kind != TY_UNION) {
      errorf_tkn(ER_COMPILE, tkn, "Need struct or union type");
    }

    Member *member = find_member(ty->member, get_ident(tkn->next));
    if (member == NULL) {
      errorf_tkn(ER_COMPILE, tkn, "This member is not found");
    }

    if (equal(tkn, ".")) node = new_binary(ND_ADDR, tkn, node);
    node = new_binary(ND_ADD, tkn, node);
    node->rhs = new_num(tkn, member->offset);
    node = new_binary(ND_CONTENT, tkn, node);

    add_type(node);
    node->ty = member->ty;
    tkn = tkn->next->next;
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return node;
}


// primary-expression = gnu-statement-expr |
//                      identifier
//                      constant
//                      string-literal
//                      "(" expr ")"
// 
// gnu-statement-expr = "({" statement statement* "})"
static Node *primary(Token *tkn, Token **end_tkn) {
  // GNU Statements
  if (equal(tkn, "(") && equal(tkn->next, "{")) {
    enter_scope();
    Node *ret = statement(tkn->next, &tkn);
    leave_scope();
 
    tkn = skip(tkn, ")");
 
    if (end_tkn != NULL) *end_tkn = tkn;
    return ret;
  }

  if (equal(tkn, "(")) {
    Node *node = expr(tkn->next, &tkn);

    tkn = skip(tkn, ")");
    if (end_tkn != NULL) *end_tkn = tkn;
    return node;
  }

  // identifier
  if (tkn->kind == TK_IDENT) {
    Obj *obj = find_var(get_ident(tkn));

    if (obj == NULL) {
      errorf_tkn(ER_COMPILE, tkn, "This object is not declaration.");
    }

    Node *node = new_var(tkn, obj);
    add_type(node);

    if (end_tkn != NULL) *end_tkn = tkn->next;
    return node;
  }

  if (tkn->kind == TK_STR) {
    if (end_tkn != NULL) *end_tkn = tkn->next;
    return new_strlit(tkn);
  }

  return constant(tkn, end_tkn);
}

// constant = interger-constant |
//            character-constant
static Node *constant(Token *tkn, Token **end_tkn) {
  if (tkn->kind != TK_NUM) {
    errorf_tkn(ER_COMPILE, tkn, "Grammatical Error");
  }

  Type *ty = tkn->ty;
  Node *node = new_node(ND_NUM, tkn);
  node->ty = ty;

  switch (ty->kind) {
    case TY_FLOAT:
    case TY_DOUBLE:
    case TY_LDOUBLE:
      node = new_floating(tkn, ty, tkn->fval);
      break;
    default:
      node->val = tkn->val;
  }

  if (end_tkn != NULL) *end_tkn = tkn->next;
  return node;
}
