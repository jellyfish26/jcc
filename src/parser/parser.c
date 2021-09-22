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
  int size;
  Initializer **children;

  Node *node;
};

typedef struct {
  bool is_type_def;
  bool is_static;
} VarAttr;

static char *break_label;
static char *conti_label;

static HashMap *label_map;
static Node *label_node;
static Node *goto_node;

// Since variables with static attribute and string literals
// need to be treated as global variables,
// they are all stored in the tmp_node variable.
static Node *tmp_node;

static Type *func_ty;  // Type of function being explore.

// Prototype
static Type *declspec(Token *tkn, Token **end_tkn, VarAttr *attr);
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
static bool is_const_expr(Node *node);
static long double eval_double(Node *node);
static Node *funcdef(Token *tkn, Token **end_tkn, Type *base_ty, VarAttr *attr);
static Node *comp_stmt(Token *tkn, Token **end_tkn);
static Initializer *initializer(Token *tkn, Token **end_tkn, Type *ty);
static Node *initdecl(Token *tkn, Token **end_tkn, Type *ty, bool is_global, VarAttr *attr);
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

char *new_unique_label() {
  static int cnt = 0;
  char *ptr = calloc(10, sizeof(char));
  sprintf(ptr, ".Luni%d", cnt++);
  return ptr;
}

int align_to(int bytes, int align) {
  if (align == 0) {
    errorf(ER_COMPILE, "Align zero is not allow");
  }

  return bytes + (align - bytes % align) % align;
}

static Node *new_node(NodeKind kind, Token *tkn) {
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

static Node *new_floating(Token *tkn, Type *ty, long double fval) {
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

static void add_tmp_node(Node *node) {
  node->next = tmp_node;
  tmp_node = node;
}

static Node *new_strlit(Token *tkn) {
  Initializer *init = new_initializer(tkn->ty, false);
  string_initializer(tkn, NULL, init);

  Node *node = new_node(ND_INIT, tkn);

  Node head = {};
  Node *dummy = &head;
  create_init_node(init, &dummy, true, tkn->ty);

  node->lhs = new_var(tkn, new_obj(tkn->ty, new_unique_label()));
  node->lhs->var->is_global = true;
  node->rhs = head.lhs;

  add_tmp_node(node);

  Node *addr = new_node(ND_ADDR, tkn);
  addr->lhs = node->lhs;

  return addr;
}

static Node *new_static_var(Node *var_node) {
  var_node->next = tmp_node;
  tmp_node = var_node;

  Node *node = new_node(ND_VAR, var_node->tkn);
  if (var_node->kind == ND_INIT) {
    node->var = var_node->lhs->var;
  } else {
    node->var = var_node->var;
  }

  node->var->is_global = true;
  node->var->name = new_unique_label();
  node->ty = node->var->ty;

  return node;
}

static Node *new_unary(NodeKind kind, Token *tkn, Node *lhs) {
  Node *node = new_node(kind, tkn);
  node->lhs = lhs;
  return node;
}

static Node *new_binary(NodeKind kind, Token *tkn, Node *lhs, Node *rhs) {
  Node *node = new_node(kind, tkn);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_commma(Token *tkn, Node *head) {
  Node *node = new_node(ND_COMMA, tkn);
  node->deep = head;
  return node;
}

static Node *new_calc(NodeKind kind, Token *tkn, Node *lhs, Node *rhs) {
  Node *node = new_binary(kind, tkn, lhs, rhs);
  add_type(node);

  if (is_ptr_type(lhs->ty) || is_ptr_type(rhs->ty)) {
    errorf_tkn(ER_COMPILE, tkn, "Invalid operand.");
  }

  return node;
}

static Node *new_add(Token *tkn, Node *lhs, Node *rhs) {
  add_type(lhs);
  add_type(rhs);

  // num + num
  if (!is_ptr_type(lhs->ty) && !is_ptr_type(rhs->ty)) {
    return new_calc(ND_ADD, tkn, lhs, rhs);
  }

  if (is_ptr_type(lhs->ty) && is_ptr_type(rhs->ty)) {
    errorf_tkn(ER_COMPILE, tkn, "Invalid operand");
  }

  if (is_ptr_type(lhs->ty)) {
    if (lhs->ty->base->kind == TY_VLA) {
      rhs = new_calc(ND_MUL, tkn, rhs, lhs->ty->base->vla_size);
    } else {
      rhs = new_calc(ND_MUL, tkn, rhs, new_num(tkn, lhs->ty->base->var_size));
    }
  }

  if (is_ptr_type(rhs->ty)) {
    if (rhs->ty->base->kind == TY_VLA) {
      lhs = new_calc(ND_MUL, tkn, lhs, rhs->ty->base->vla_size);
    } else {
      lhs = new_calc(ND_MUL, tkn, lhs, new_num(tkn, rhs->ty->base->var_size));
    }
  }

  return new_binary(ND_ADD, tkn, lhs, rhs);
}

static Node *new_sub(Token *tkn, Node *lhs, Node *rhs) {
  add_type(lhs);
  add_type(rhs);

  if (is_ptr_type(lhs->ty) && is_ptr_type(rhs->ty)) {
    if (!is_same_type(lhs->ty, rhs->ty)) {
      errorf_tkn(ER_COMPILE, tkn, "Invalid operand");
    }

    Node *node = new_binary(ND_SUB, tkn, lhs, rhs);
    node->ty = ty_i64;

    if (lhs->ty->base->kind == TY_VLA) {
      node = new_binary(ND_DIV, tkn, node, lhs->ty->base->vla_size);
    } else {
      node = new_binary(ND_DIV, tkn, node, new_num(tkn, lhs->ty->base->var_size));
    }
    return node;
  }

  Node *node = new_add(tkn, lhs, rhs);
  node->kind = ND_SUB;
  return node;
}

static Node *new_assign(Token *tkn, Node *lhs, Node *rhs) {
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

// Return nodes to fix the length of VLA,
// and vla_size is replaced with a temporary variable that
// contains the length of VLA.
static Node *compute_vla_len(Type *ty) {
  if (ty->kind != TY_VLA) {
    return NULL;
  }

  Obj *tmp_var = new_obj(copy_type(ty_i64), new_unique_label());
  add_var(tmp_var, true);

  Node *node = ty->vla_size;
  ty->vla_size = new_var(ty->tkn, tmp_var);
  node = new_assign(ty->tkn, ty->vla_size, node);
  node->next = compute_vla_len(ty->base);
  return node;
}

static Node *to_assign(Token *tkn, Node *rhs) {
  return new_assign(tkn, rhs->lhs, rhs);
}

static bool is_typename(Token *tkn) {
  char *keywords[] = {
    "void", "_Bool", "char", "short", "int", "long", "float", "double", "signed", "unsigned",
    "const", "enum", "struct", "union", "auto", "static", "typedef"
  };

  for (int i = 0; i < sizeof(keywords) / sizeof(*keywords); i++) {
    if (equal(tkn, keywords[i])) {
      return true;
    }
  }

  return find_type_def(strndup(tkn->loc, tkn->len));
}

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
// constant-expression = conditional-expression
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

  Type *enumerator_ty = copy_type(ty_i32);

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
      val = eval_expr(conditional(tkn, &tkn));
    } else {
      val++;
    }

    if (val >> 31 && enumerator_ty->kind == TY_INT) {
      enumerator_ty = copy_type(ty_i64);
    }

    Obj *obj = new_obj(NULL, ident);
    obj->val = val;

    cur->next = obj;
    cur = obj;
  }

  Type *ty = copy_type(enumerator_ty);
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

// construct-members -> "{" struct-declaration-list* "}"
// struct-declaration-list = struct-declaration*
// struct-declaration      = specifier-qualifier-list (declspec) struct-declarator ("," struct-declarator)* ";"
// struct-declarator       = declarator (":" constant-expression)?
static void construct_members(Token *tkn, Token **end_tkn, Type *ty) {
  Member head = {};
  Member *cur = &head;
  int num_members = 0;

  tkn = skip(tkn, "{");
  while (!consume_close_brace(tkn, &tkn)) {
    if (!is_typename(tkn)) {
      errorf_tkn(ER_COMPILE, tkn, "Need type");
    }

    Type *base_ty = declspec(tkn, &tkn, NULL);

    bool need_comma = false;
    while (!consume(tkn, &tkn, ";")) {
      if (need_comma) {
        tkn = skip(tkn, ",");
      }
      need_comma = true;

      Type *mem_ty = copy_type(declarator(tkn, &tkn, base_ty));

      if (consume(tkn, &tkn, ":")) {
        if (mem_ty == NULL) {
          mem_ty = copy_type(base_ty);
        }

        mem_ty->bit_field = eval_expr(conditional(tkn, &tkn));
      } else {
        // If the bit field is 0, the behavior is to fill the extra field,
        // which distinguishes it from members with no specified bit field.
        mem_ty->bit_field = -1;
      }

      if (mem_ty == NULL) {
        continue;
      }

      check_member(head.next, mem_ty->name, mem_ty->tkn);
      Member *member = calloc(1, sizeof(Member));
      member->ty = mem_ty;
      member->tkn = mem_ty->tkn;
      member->name = mem_ty->name;

      cur->next = member;
      cur = member;
      num_members++;
    }
  }

  ty->num_members = num_members;
  ty->members = head.next;
  if (end_tkn != NULL) *end_tkn = tkn;
}

static void struct_specifier(Token *tkn, Token **end_tkn, Type *ty) {
  construct_members(tkn, &tkn, ty);

  // Initialize size, offset, and align
  int bytes = 0, align = 0, bit_offset = 0;
  for (Member *member = ty->members; member != NULL; member = member->next) {
    if (align < member->ty->align) {
      align = member->ty->align;
    }

    int bit_field = member->ty->bit_field;
    if (bit_field != -1) {
      if (bit_field == 0 || bit_offset + bit_field > member->ty->var_size * 8) {
        bytes += align_to(bit_offset, 8) / 8;
        bit_offset = 0;
      }

      member->ty->bit_offset = bit_offset;
      member->offset = bytes;
      bit_offset += bit_field;
      continue;
    }

    bytes = align_to(bytes + align_to(bit_offset, 8) / 8, member->ty->align);
    bit_offset = 0;

    member->offset = bytes;
    bytes += member->ty->var_size;
  }
  ty->var_size = bytes + align_to(bit_offset, 8) / 8;
  ty->align = align;

  if (end_tkn != NULL) *end_tkn = tkn;
}

static void union_specifier(Token *tkn, Token **end_tkn, Type *ty) {
  construct_members(tkn, &tkn, ty);

  int bytes = 0, align = 0;
  for (Member *member = ty->members; member != NULL; member = member->next) {
    if (align < member->ty->align) {
      align = member->ty->align;
    }

    if (bytes < member->ty->var_size) {
      bytes = member->ty->var_size;
    }
  }
  ty->var_size = bytes;
  ty->align = align;

  if (end_tkn != NULL) *end_tkn = tkn; 
}

// struct-or-union-specifier = struct-or-union identifier |
//                             struct-or-union identifier? "{" struct-declaration-list "}"
//
// struct-or-union = "struct" | "union"
//
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
      ty = calloc(1, sizeof(Type));
      ty->tkn = tkn;
      ty->kind = kind;
      add_tag(ty, tag);
    } else if (ty->kind != kind) {
      errorf_tkn(ER_COMPILE, tkn, "This tag is not an %s tag", kind == TY_STRUCT ? "struct" : "union");
    }

    if (end_tkn != NULL) *end_tkn = tkn;
    return ty;
  }

  if (tag == NULL) {
    tag = new_unique_label();
  }

  Type *ty = calloc(1, sizeof(Type));
  ty->kind = kind;
  enforce_add_tag(ty, tag);
  ty = find_tag(tag);

  if (kind == TY_STRUCT) {
    struct_specifier(tkn, end_tkn, ty);
  } else {
    union_specifier(tkn, end_tkn, ty);
  }

  return ty;
}

// declaration-specifiers = type-specifier declaration-specifiers?
//                          type-qualifier declaration-specifiers?
//                          storage-class-specifier declaration-specifiers?
//
// type-specifier = "void" | "_Bool | "char" | "short" | "int" | "long" | "double" | "signed" | "unsigned" |
//                  enum-specifier |
//                  struct-or-union-specifier
// type-qualifier = "const"
// storage-class-specifier = "typedef" | "static" | "auto"
static Type *declspec(Token *tkn, Token **end_tkn, VarAttr *attr) {
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

  int ty_cnt = 0;
  bool is_const = false;
  Type *ty = NULL;
  while (is_typename(tkn)) {
    if (equal(tkn, "const")) {
      if (is_const) {
        errorf_tkn(ER_COMPILE, tkn, "Duplicate const");
      }
      is_const = true;
      tkn = tkn->next;
      continue;
    }

    // Check storage class specifier
    if (equal(tkn, "static") || equal(tkn, "typedef")) {
      if (attr == NULL) {
        errorf_tkn(ER_COMPILE, tkn, "Storage class specifier is not allowd in this context");
      }

      if (equal(tkn, "static")) {
        attr->is_static = true;
      } else if (equal(tkn, "typedef")) {
        attr->is_type_def = true;
      }

      tkn = tkn->next;
      continue;
    }

    // Ignore these keywords
    if (consume(tkn, &tkn, "auto")) {
      continue;
    }

    Type *type_def = find_type_def(strndup(tkn->loc, tkn->len));
    if (type_def != NULL) {
      ty = type_def;
      tkn = tkn->next;
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
  ty->is_const = is_const;

  if (end_tkn != NULL) *end_tkn = tkn;
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
  while (consume(tkn, &tkn, "*")) {
    ty = pointer_to(ty);

    if (consume(tkn, &tkn, "const")) {
      ty->is_const = true;
    }
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return ty;
}

static Type *new_vla(Token *tkn, Type *base, Node *node) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_VLA;

  if (base->vla_size == NULL) {
    ty->vla_size = new_binary(ND_MUL, tkn, node, new_num(tkn, base->var_size));
  } else {
    ty->vla_size = new_binary(ND_MUL, tkn, node, base->vla_size);
  }

  ty->base = base;
  ty->var_size = 8;
  ty->align = base->align;
  return ty;
}

// If all the array length specifications are constant expressions,
// the type is changing from vla to normal array.
static Type *vla_to_arr(Type *ty) {
  if (ty->kind != TY_VLA || !is_const_expr(ty->vla_size)) {
    return ty;
  }

  ty->kind = TY_ARRAY;
  ty->base = vla_to_arr(ty->base);
  ty->align = ty->base->align;
  ty->var_size = eval_expr(ty->vla_size);
  ty->array_len = ty->var_size / ty->base->var_size;
  ty->vla_size = NULL;
  return ty;
}

// array-dimension = constant "]" type-suffix
static Type *array_dimension(Token *tkn, Token **end_tkn, Type *ty) {
  int64_t val = 0;
  Node *node = NULL;

  if (!consume(tkn, &tkn, "]")) {
    node = assign(tkn, &tkn);
    tkn = skip(tkn, "]");
  }

  ty = type_suffix(tkn, end_tkn, ty);

  // When declare an array of unspecific length, we cannot declare a VLA.
  if (node == NULL || ty->kind == TY_ARRAY) {
    ty = vla_to_arr(ty);
    return array_to(ty, node == NULL ? 0 : eval_expr(node));
  }

  return new_vla(node->tkn, ty, node);
}

// param-list = ("void" | param ("," param)*)? ")"
//
// param = declspec declarator
static Type *param_list(Token *tkn, Token **end_tkn, Type *ty) {
  Type *ret_ty = ty;
  ty = calloc(1, sizeof(Type));
  ty->kind = TY_FUNC;
  ty->ret_ty = ret_ty;

  Type head = {};
  Type *cur = &head;

  consume(tkn, &tkn, "void");
  while (!consume(tkn, &tkn, ")")) {
    if (cur != &head) {
      tkn = skip(tkn, ",");
    }

    Type *param_ty = declspec(tkn, &tkn, NULL);
    param_ty = declarator(tkn, &tkn, copy_type(param_ty));

    // In the function parameters, the array is treated as a pointer variable.
    if (param_ty->kind == TY_ARRAY || param_ty->kind == TY_VLA) {
      param_ty->kind = TY_PTR;
      param_ty->var_size = 8;
    }
    add_var(new_obj(param_ty, param_ty->name), true);

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
    enter_scope();
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

  return vla_to_arr(ty);
}

// abstract-declarator = pointer | pointer? direct-abstract-declarator
//
// direct-abstract-declarator = type-suffix
//
// Implement:
// abstract-declarator = pointer? type-suffix
static Type *abstract_declarator(Token *tkn, Token **end_tkn, Type *ty) {
  ty = pointer(tkn, &tkn, ty);
  ty = type_suffix(tkn, end_tkn, ty);
  
  return vla_to_arr(ty);
}

static Initializer *new_initializer(Type *ty, bool is_flexible) {
  Initializer *init = calloc(1, sizeof(Initializer));
  init->ty = ty;

  if (ty->kind == TY_ARRAY) {
    if (is_flexible && ty->array_len == 0) {
      init->is_flexible = true;
      return init;
    }

    init->size = ty->array_len;
    init->children = calloc(ty->array_len, sizeof(Initializer *));
    for (int i = 0; i < ty->array_len; i++) {
      init->children[i] = new_initializer(ty->base, false);
    }
  }

  if (ty->kind == TY_STRUCT) {
    init->size = ty->num_members;
    init->children = calloc(ty->num_members, sizeof(Initializer *));

    int i = 0;
    for (Member *member = ty->members; member != NULL; member = member->next) {
      init->children[i++] = new_initializer(member->ty, false);
    }
  }

  if (ty->kind == TY_UNION) {
    init->size = 1;
    init->children = calloc(1, sizeof(Initializer *));
    init->children[0] = new_initializer(ty->members->ty, false);
  }
  return init;
}

static int count_array_init_elements(Token *tkn, Type *ty) {
  tkn = skip(tkn, "{");

  int cnt = 0;
  Initializer *dummy = new_initializer(ty->base, false);

  while (!consume_close_brace(tkn, &tkn)) {
    if (cnt != 0) {
      tkn = skip(tkn, ",");
    }

    initializer_only(tkn, &tkn, dummy);
    cnt++;
  }
  
  return cnt;
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

// initializer-list = "{" designation? initializer_only ("," designation? initializer_only)* ","? "}" | "{" "}"
// designation = "[" constant-expression "]" "=" |
//                "." identifier "="
static void initializer_list(Token *tkn, Token **end_tkn, Initializer *init) {
  if (init->is_flexible) {
    int size = count_array_init_elements(tkn, init->ty);
    *init = *new_initializer(array_to(init->ty->base, size), false);
  }

  tkn = skip(tkn, "{");
  Type *ty = init->ty;
  int idx = 0;

  while (!consume_close_brace(tkn, &tkn)) {
    if (idx != 0) {
      tkn = skip(tkn, ",");
    }

    if (ty->kind == TY_ARRAY && consume(tkn, &tkn, "[")) {
      int val = eval_expr(conditional(tkn, &tkn));
      tkn = skip(tkn, "]");
      tkn = skip(tkn, "=");
      idx = val;
    }

    if (is_struct_type(ty) && consume(tkn, &tkn, ".")) {
      char *ident = get_ident(tkn);
      int i = 0;
      Type *member_ty;
      for (Member *member = ty->members; member != NULL; member = member->next) {
        member_ty = member->ty;
        if (strcmp(ident, member->name) == 0) {
          break;
        }
        i++;
      }
      tkn = skip(tkn->next, "=");
      if (ty->kind == TY_UNION) {
        idx = 0;
        init->children[0]->ty = member_ty;
      } else {
        idx = i;
      }
    }

    if (idx < init->size) {
      initializer_only(tkn, &tkn, init->children[idx]);
    } else {
      Initializer *dummy = calloc(1, sizeof(Initializer));
      initializer_only(tkn, &tkn, dummy);
      free(dummy);
    }
    idx++;
  }
  if (end_tkn != NULL) *end_tkn = tkn;
}

// initializer = array_initializer | string_initializer | assign
static void initializer_only(Token *tkn, Token **end_tkn, Initializer *init) {
  if (tkn->kind == TK_STR) {
    string_initializer(tkn, end_tkn, init);
    return;
  }

  if (equal(tkn, "{")) {
    initializer_list(tkn, end_tkn, init);
    return;
  }

  init->node = assign(tkn, end_tkn);
}

static Initializer *initializer(Token *tkn, Token **end_tkn, Type *ty) {
  Initializer *init = new_initializer(ty, true);
  initializer_only(tkn, end_tkn, init);
  return init;
}

static void create_init_node(Initializer *init, Node **connect, bool only_const, Type *ty) {
  if (init->node != NULL) {
    Node *node = new_node(ND_INIT, init->node->tkn);
    node->init = init->node;
    node->ty = ty;

    if (!is_same_type(ty, node->init->ty)) {
      node->init = new_cast(node->init, ty);
    }

    if (only_const) {
      char *label = NULL;
      if (is_float_type(node->init->ty)) {
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

  if (init->children == NULL) {
    Node *node = new_node(ND_INIT, init->tkn);
    node->ty = init->ty;
    (*connect)->lhs = node;
    *connect = node;
  }

  if (ty->kind == TY_ARRAY) {
    for (int i = 0; i < init->ty->array_len; i++) {
      create_init_node(init->children[i], connect, only_const, ty->base);
    }
  }

  if (ty->kind == TY_STRUCT) {
    int i = 0;
    for (Member *member = ty->members; member != NULL; member = member->next) {
      create_init_node(init->children[i++], connect, only_const, member->ty);
    }
  }

  if (ty->kind == TY_UNION) {
    create_init_node(init->children[0], connect, only_const, init->children[0]->ty);
  }
}

static int64_t eval_expr(Node *node) {
  return eval_expr2(node, NULL);
}

// Evaluate a given node as a constant expression;
static int64_t eval_expr2(Node *node, char **label) {
  add_type(node);

  if (node->lhs != NULL && is_float_type(node->lhs->ty)) {
    return eval_double(node);
  }

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
      return eval_expr(node->lhs) == eval_expr(node->rhs);
    case ND_NEQ:
      return eval_expr(node->lhs) != eval_expr(node->rhs);
    case ND_LC:
      if (node->lhs->ty->is_unsigned) {
        return (uint64_t)eval_expr(node->lhs) < eval_expr(node->rhs);
      }
      return eval_expr(node->lhs) < eval_expr(node->rhs);
    case ND_LEC:
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
      return eval_expr(node->lhs) && eval_expr(node->rhs);
    case ND_LOGICALOR:
      return eval_expr(node->lhs) || eval_expr(node->rhs);
    case ND_COND:
      return eval_expr(node->cond) ? eval_expr2(node->lhs, label) : eval_expr2(node->rhs, label);
    case ND_VAR:
      if (label != NULL && node->ty->kind == TY_ARRAY) {
        *label = node->var->name;
        return 0;
      }

      if (is_ptr_type(node->ty)) {
        break;
      }

      return node->var->val;
    case ND_CAST: {
      int64_t val = eval_expr2(node->lhs, label);
      switch (node->ty->kind) {
        case TY_CHAR:
          return node->ty->is_unsigned ? (uint8_t)val : (int8_t)val;
        case TY_SHORT:
          return node->ty->is_unsigned ? (uint16_t)val : (int16_t)val;
        case TY_INT:
          return node->ty->is_unsigned ? (uint32_t)val : (int32_t)val;
        default:
          return val;
      }
    }
    case ND_ADDR:
      return eval_addr(node->lhs, label);
    case ND_CONTENT:
      return eval_expr2(node->lhs, label);
    case ND_NUM:
      return node->val;
    default:
      errorf_tkn(ER_COMPILE, node->tkn, "Not a compiler-time constant");
  }

  return 0;  // unreachable
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
    default:
      errorf_tkn(ER_COMPILE, node->tkn, "Not a compiler-time constant");
  }

  return 0; // unreachable
}

static long double eval_double(Node *node) {
  add_type(node);

  if (node->lhs != NULL && is_integer_type(node->lhs->ty)) {
    if (node->lhs->ty->is_unsigned) {
      return (uint64_t)eval_expr(node);
    }
    return eval_expr(node);
  }

  switch (node->kind) {
    case ND_ADD:
      return eval_double(node->lhs) + eval_double(node->rhs);
    case ND_SUB:
      return eval_double(node->lhs) - eval_double(node->rhs);
    case ND_MUL:
      return eval_double(node->lhs) * eval_double(node->rhs);
    case ND_DIV:
      return eval_double(node->lhs) / eval_double(node->rhs);
    case ND_EQ:
      return eval_double(node->lhs) == eval_double(node->rhs);
    case ND_NEQ:
      return eval_double(node->lhs) != eval_double(node->rhs);
    case ND_LC:
      return eval_double(node->lhs) < eval_double(node->rhs);
    case ND_LEC:
      return eval_double(node->lhs) <= eval_double(node->rhs);
    case ND_VAR:
      if (!node->var->is_global) {
        break;
      }

      return node->var->fval;
    case ND_CAST:
      if (is_float_type(node->lhs->ty)) {
        return eval_double(node->lhs);
      }
      return eval_expr(node->lhs);
    case ND_NUM:
      return node->fval;
    default:
      errorf_tkn(ER_COMPILE, node->tkn, "Not a compiler-time constant");
  }

  return 0; // unreachable
}

static bool is_const_expr(Node *node) {
  add_type(node);

  switch (node->kind) {
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_REMAINDER:
    case ND_EQ:
    case ND_NEQ:
    case ND_LC:
    case ND_LEC:
    case ND_LEFTSHIFT:
    case ND_RIGHTSHIFT:
    case ND_BITWISEAND:
    case ND_BITWISEOR:
    case ND_BITWISEXOR:
    case ND_LOGICALAND:
    case ND_LOGICALOR:
      return is_const_expr(node->lhs) && is_const_expr(node->rhs);
    case ND_COND:
      if (!is_const_expr(node->cond)) {
        return false;
      }
      return is_const_expr(eval_expr(node->cond) ? node->lhs : node->rhs);
    case ND_BITWISENOT:
    case ND_CAST:
      return is_const_expr(node->lhs);
    case ND_NUM:
      return true;
    default:
      return false;
  }
}

// declaration = declaration-specifiers init-declarator-list?
//
// init-declarator-list = init-declarator | 
//                        init-declarator-list "," init-declarator
//
// declaration -> init-declarator ("," init-declarator)* ";"?
// declspec is replaced as base_ty argument.
static Node *declaration(Token *tkn, Token **end_tkn, Type *base_ty, bool is_global, VarAttr *attr) {
  Node head = {};
  Node *cur = &head;

  while (!consume(tkn, &tkn, ";")) {
    if (cur != &head) {
      tkn = skip(tkn, ",");
    }
    cur->next = initdecl(tkn, &tkn, base_ty, is_global, attr);
    cur = cur->next;
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return head.next;
}

// init-declarator = declarator ("=" initializer)?
static Node *initdecl(Token *tkn, Token **end_tkn, Type *ty, bool is_global, VarAttr *attr) {
  ty = declarator(tkn, &tkn, ty);

  if (attr->is_type_def) {
    char *name = ty->name;
    ty->name = NULL;
    add_type_def(ty, name);

    if (end_tkn != NULL) *end_tkn = tkn;
    return new_node(ND_VOID, tkn);
  }

  ty = copy_type(ty);
  Obj *obj = new_obj(ty, ty->name);

  if (ty->kind == TY_FUNC) {
    Node *node = new_node(ND_FUNC, tkn);
    node->func = obj;
    leave_scope();
    init_offset();

    if (!declare_func(ty, attr->is_static)) {
      errorf_tkn(ER_COMPILE, tkn, "Conflict declaration");
    }

    if (end_tkn != NULL) *end_tkn = tkn;
    return node;
  }

  if (ty->kind == TY_VLA) {
    if (is_global) {
      errorf_tkn(ER_COMPILE, tkn, "VLA cannot declare globally");
    }

    // Fix array len
    Node *node = compute_vla_len(ty);
    node = new_commma(ty->tkn, node);
    node->next = new_var(tkn, obj);
    add_var(obj, true);

    if (end_tkn != NULL) *end_tkn = tkn;
    return node;
  }

  obj->is_global = is_global;

  if (attr->is_static) {
    add_var(obj, false);
    is_global = true;
  } else {
    add_var(obj, !is_global);
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

    if (attr->is_static) {
      return new_static_var(node);
    } else {
      return node;
    }
  }

  if (end_tkn != NULL) *end_tkn = tkn;

  if (attr->is_static) {
    return new_static_var(new_var(tkn, obj));
  } else {
    return new_var(tkn, obj);
  }
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

    VarAttr *attr = calloc(1, sizeof(VarAttr));
    Type *ty = declspec(tkn, &tkn, attr);
    Node *node = funcdef(tkn, &tkn, copy_type(ty), attr);
    if (node == NULL) {
      node = declaration(tkn, &tkn, ty, true, attr);
    }

    cur->next = tmp_node;
    cur = last_stmt(cur);
    cur->next = node;
    cur = last_stmt(cur);

    tmp_node = NULL;
  }
  return head.next;
}

// function-definition = declaration-specifiers declarator declaration-list? compound-statement
//
// declaration-list = declaration |
//
// funcdef -> declarator comp-stmt | None
//
static Node *funcdef(Token *tkn, Token **end_tkn, Type *base_ty, VarAttr *attr) {
  label_node = NULL;
  goto_node = NULL;
  label_map = calloc(1, sizeof(HashMap));

  Type *ty = declarator(tkn, &tkn, base_ty);
  if (!equal(tkn, "{")) {
    if (ty != NULL && ty->kind == TY_FUNC) {
      leave_scope();
      init_offset();
    }
    return NULL;
  }

  func_ty = ty;
  ty->is_prototype = false;

  Obj head = {};
  Obj *cur = &head;

  Node vla_init_head = {};
  Node *vla_init_node = &vla_init_head;

  for (Type *param = ty->params; param != NULL; param = param->next) {
    // Fix array len 
    if (param->base != NULL && param->base->kind == TY_VLA) {
      vla_init_node->next = compute_vla_len(param->base);
      vla_init_node->next = new_commma(param->tkn, vla_init_node->next);
      vla_init_node = vla_init_node->next;
    }

    cur->next = find_var(param->name);
    cur = cur->next;
  }

  if (!define_func(ty, attr->is_static)) {
    errorf_tkn(ER_COMPILE, tkn, "Conflict define");
  }

  Node *node = new_node(ND_FUNC, tkn);
  node->deep = comp_stmt(tkn, &tkn);
  leave_scope();

  // Add nodes of fix array len
  vla_init_node->next = node->deep->deep;
  node->deep->deep = vla_init_head.next;

  Obj *func = find_var(ty->name);
  node->func = func;
  node->func->vars_size = init_offset();
  node->func->ty->var_size = node->func->vars_size;
  func->params = head.next;

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
    node->val = eval_expr(conditional(tkn->next, &tkn));
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
    if (!is_integer_type(node->cond->ty)) {
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
      VarAttr *attr = calloc(1, sizeof(VarAttr));
      Type *ty = declspec(tkn, &tkn, attr);
      ret->init = declaration(tkn, &tkn, ty, false, attr);
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
    Node *node = new_node(ND_RETURN, tkn);
    node->lhs = assign(tkn->next, &tkn);
    add_type(node);
    node->ty = func_ty;

    if (!is_same_type(func_ty->ret_ty, node->lhs->ty)) {
      node->lhs = new_cast(node->lhs, func_ty->ret_ty);
    }
    tkn = skip(tkn, ";");

    if (end_tkn != NULL) *end_tkn = tkn;
    return node;
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
        VarAttr *attr = calloc(1, sizeof(VarAttr));
        Type *ty = declspec(tkn, &tkn, attr);
        cur->next = declaration(tkn, &tkn, ty, false, attr);
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
//              expression "," assignment-expression
//
// expr -> assign ("," assign)*
static Node *expr(Token *tkn, Token **end_tkn) {
  Node head = {};
  Node *cur = &head;

  while (true) {
    if (cur != &head && !consume(tkn, &tkn, ",")) {
      break;
    }

    cur->next = assign(tkn, &tkn);
    cur = cur->next;
  }

  Node *node = head.next;
  if (node->next != NULL) {
    node = new_commma(node->tkn, node);
  }

  if (end_tkn != NULL) *end_tkn = tkn;
  return node;
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
    Type *ty = declspec(tkn->next, &tkn, NULL);
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
//                    "_Alignof" unary-expression (GNU-extension) |
//                    "_Alignof" "(" type-name ")"
//
// unary-operator   = "&" | "*" | "+" | "-" | "~" | "!"
// typename =  specifier-qualifier-list abstruct-declarator?
//          -> declspec abstract-declarator
static Node *unary(Token *tkn, Token **end_tkn) {
  if (equal(tkn, "++")) {
    Node *node = to_assign(tkn, new_add(tkn, unary(tkn->next, end_tkn), new_num(tkn, 1)));
    node->next = node->lhs;
    return new_commma(tkn, node);
  }

  if (equal(tkn, "--")) {
    Node *node = to_assign(tkn, new_sub(tkn, unary(tkn->next, end_tkn), new_num(tkn, 1)));
    node->next = node->lhs;
    return new_commma(tkn, node);
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

  if (equal(tkn, "sizeof") || equal(tkn, "_Alignof")) {
    bool is_sizeof = equal(tkn, "sizeof");
    tkn = tkn->next;

    // type-name
    if (equal(tkn, "(") && is_typename(tkn->next)) {
      Type *ty = declspec(tkn->next, &tkn, NULL);
      ty = abstract_declarator(tkn, &tkn, ty);

      tkn = skip(tkn, ")");
      if (end_tkn != NULL) *end_tkn = tkn;

      if (is_sizeof && ty->kind == TY_VLA) {
        return ty->vla_size;
      }
      return new_num(tkn, is_sizeof ? ty->var_size : ty->align);
    }

    Node *node = unary(tkn, end_tkn);
    add_type(node);

    if (is_sizeof && node->ty->kind == TY_VLA) {
      return node->ty->vla_size;
    }
    return new_num(tkn, is_sizeof ? node->ty->var_size : node->ty->align);
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
      node = new_unary(ND_CONTENT, tkn, new_add(tkn, node, assign(tkn->next, &tkn)));
      tkn = skip(tkn, "]");
      continue;
    }

    if (equal(tkn, "(")) {
      node = new_unary(ND_FUNCCALL, tkn, node);
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

        cur->next = new_unary(ND_VOID, tkn, assign(tkn, &tkn));
        cur = cur->next;
        argc++;
      }

      if (node->ty->param_cnt != 0 && node->ty->param_cnt != argc) {
        errorf_tkn(ER_COMPILE, tkn, "Do not match arguments to function call, expected %d, have %d", node->ty->param_cnt, argc);
      }

      cur = head.next;
      for (Type *arg_ty = node->ty->params; arg_ty != NULL; arg_ty = arg_ty->next) {
        if (!is_struct_type(arg_ty)) {
          cur->lhs = new_cast(cur->lhs, arg_ty);
        }
        cur = cur->next;
      }

      node->args = head.next;
      continue;
    }

    if (equal(tkn, "++")) {
      node = to_assign(tkn, new_add(tkn, node, new_num(tkn, 1)));
      node->next = new_sub(tkn, node->lhs, new_num(tkn, 1));
      node = new_commma(tkn, node);
      tkn = tkn->next;
      continue;
    }

    if (equal(tkn, "--")) {
      node = to_assign(tkn, new_sub(tkn, node, new_num(tkn, 1)));
      node->next = new_add(tkn, node->lhs, new_num(tkn, 1));
      node = new_commma(tkn, node);
      tkn = tkn->next;
      continue;
    }

    // "." or "->" operator
    add_type(node);

    if (equal(tkn, "->") && node->ty->kind != TY_PTR) {
      errorf_tkn(ER_COMPILE, tkn, "Need pointer type");
    }

    Type *ty = equal(tkn, ".") ? node->ty : node->ty->base;
    if (!is_struct_type(ty)) {
      errorf_tkn(ER_COMPILE, tkn, "Need struct or union type");
    }

    Member *member = find_member(ty->members, get_ident(tkn->next));
    if (member == NULL) {
      errorf_tkn(ER_COMPILE, tkn, "This member is not found");
    }

    node = new_unary(ND_ADD, tkn, node);
    node->rhs = new_num(tkn, member->offset);
    node = new_unary(ND_CONTENT, tkn, node);
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
      printf("%s\n", get_ident(tkn));
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
