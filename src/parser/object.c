#include "token/tokenize.h"
#include "parser/parser.h"
#include "util/util.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Type *ty_void;
Type *ty_bool;

Type *ty_i8;
Type *ty_i16;
Type *ty_i32;
Type *ty_i64;

Type *ty_u8;
Type *ty_u16;
Type *ty_u32;
Type *ty_u64;

Type *ty_f32;
Type *ty_f64;
Type *ty_f80;

static Type *new_ty(TypeKind kind, bool is_unsigned, int size) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = kind;
  ty->is_unsigned = is_unsigned;
  ty->var_size = size;
  return ty;
}

Type *copy_ty(Type *ty) {
  Type *cty = calloc(1, sizeof(Type));
  memcpy(cty, ty, sizeof(Type));
  return cty;
}

void init_type() {
  ty_void = new_ty(TY_VOID, false, 0);
  ty_bool = new_ty(TY_CHAR, false, 1);

  ty_i8  = new_ty(TY_CHAR, false, 1);
  ty_i16 = new_ty(TY_SHORT, false, 2);
  ty_i32 = new_ty(TY_INT, false, 4);
  ty_i64 = new_ty(TY_LONG, false, 8);

  ty_u8  = new_ty(TY_CHAR, true, 1);
  ty_u16 = new_ty(TY_SHORT, true, 2);
  ty_u32 = new_ty(TY_INT, true, 4);
  ty_u64 = new_ty(TY_LONG, true, 8);

  ty_f32 = new_ty(TY_FLOAT, false, 4);
  ty_f64 = new_ty(TY_DOUBLE, false, 8);
  ty_f80 = new_ty(TY_LDOUBLE, false, 16);
}

Type *pointer_to(Type *base) {
  Type *ty = new_ty(TY_PTR, false, 8);
  ty->base = base;
  return ty;
}

Type *array_to(Type *type, int array_len) {
  Type *ret = calloc(1, sizeof(Type));
  ret->kind = TY_ARRAY;
  ret->base = type;
  ret->var_size = array_len * type->var_size;
  ret->array_len = array_len;
  return ret;
}

bool is_integer_ty(Type *ty) {
  switch (ty->kind) {
    case TY_CHAR:
    case TY_SHORT:
    case TY_INT:
    case TY_LONG:
      return true;
  }
  return false;
}

bool is_float_ty(Type *ty) {
  switch (ty->kind) {
    case TY_FLOAT:
    case TY_DOUBLE:
    case TY_LDOUBLE:
      return true;
  }
  return false;
}

bool is_same_type(Type *lty, Type *rty) {
  while (lty != NULL && rty != NULL) {
    if (lty->kind != rty->kind) {
      return false;
    }

    if (lty->kind == TY_ARRAY) {
      // If the size of either array is undecided, return true.
      if (lty->var_size != 0 && rty->var_size != 0 && lty->var_size != rty->var_size) {
        return false;
      }
    }

    lty = lty->base;
    rty = rty->base;
  }

  if (lty != NULL || rty != NULL) {
    return false;
  }

  return true;
}

// In the case of functions, we need to look at the type of the return type.
Type *extract_ty(Type *ty) {
  if (ty->kind == TY_FUNC) {
    return ty->ret_ty;
  }
  return ty;
}

Obj *new_obj(Type *type, char *name) {
  int name_len = strlen(name);
  Obj *ret = calloc(1, sizeof(Obj));
  ret->ty = type;
  ret->name = calloc(name_len + 1, sizeof(char));
  memcpy(ret->name, name, name_len);
  ret->name_len = name_len;
  return ret;
}

typedef struct Scope Scope;
struct Scope {
  Scope *up;

  HashMap var;
  HashMap tag;
};

static Scope *scope = &(Scope){};
static int offset;

void enter_scope() {
  Scope *sc = calloc(1, sizeof(Scope));
  sc->up = scope;
  scope = sc;
}

void leave_scope() {
  if (scope == NULL) {
    errorf(ER_INTERNAL, "Internal error at scope");
  }

  scope = scope->up;
}

void add_var(Obj *var, bool set_offset) {
  hashmap_insert(&(scope->var), var->name, var);

  if (set_offset) {
    int sz = var->ty->var_size;
    var->offset = (offset += sz);
  }
}

void add_tag(Type *ty, char *name) {
  hashmap_insert(&(scope->tag), name, ty);
}

Obj *find_var(char *name) {
  // Find in local object
  for (Scope *cur = scope; cur != NULL; cur = cur->up) {
    Obj *obj = hashmap_get(&(cur->var), name);
    if (obj != NULL) {
      return obj;
    }
  }

  return NULL;
}

Type *find_tag(char *name) {
  // Find in local tag
  for (Scope *cur = scope; cur != NULL; cur = cur->up) {
    Type *ty = hashmap_get(&(cur->tag), name);
    if (ty != NULL) {
      return ty;
    }
  }

  return NULL;
}

bool can_declare_var(char *name) {
  return hashmap_get(&(scope->var), name) == NULL;
}

bool can_declare_tag(char *name) {
  return hashmap_get(&(scope->tag), name) == NULL;
}


static bool check_func_params(Type *lty, Type *rty) {
  if (lty->param_cnt != rty->param_cnt) {
    return false;
  }
  lty = lty->params;
  rty = rty->params;

  while (lty != NULL && rty != NULL) {
    if (!is_same_type(lty, rty)) {
      return false;
    }

    lty = lty->next;
    rty = rty->next;
  }
  return true;
}

bool declare_func(Type *ty) {
  ty->is_prototype = true;
  Obj *already = find_var(ty->name);

  if (already == NULL) {
    add_var(new_obj(ty, ty->name), false);
    return true;
  }

  // If the return type is different from a function that already declared,
  // we cannot be redeclared.
  if (!is_same_type(ty->ret_ty, already->ty->ret_ty)) {
    return false;
  }

  // If the number of parameters in the function declaration is zero,
  // we will not update the already declared function,
  // but function can be declare as many times as we want, so we will return true.
  if (ty->params == NULL) {
    return true;
  }

  // If the number of parameters in the function declaration is zero,
  // we can update the function declaration.
  if (already->params == NULL) {
    add_var(new_obj(ty, ty->name), false);
    return true;
  }

  return check_func_params(ty, already->ty);
}

bool define_func(Type *ty) {
  ty->is_prototype = false;
  Obj *alrady = find_var(ty->name);

  if (alrady == NULL) {
    add_var(new_obj(ty, ty->name), false);
    return true;
  }

  if (!alrady->ty->is_prototype || !check_func_params(ty, alrady->ty)) {
    return false;
  }

  add_var(new_obj(ty, ty->name), false);
  return true;
}

int init_offset() {
  int sz = offset + 16 - (offset % 16);
  offset = 0;
  return sz;
}

// if type size left < right is true
// other is false
static bool comp_type(Type *left, Type *right) {
  if (left->kind < right->kind) {
    return true;
  }

  if (left->kind == right->kind) {
    return right->is_unsigned;
  }

  return false;
}

static void implicit_cast(Node **lhs, Node **rhs) {
  Type *type = comp_type((*lhs)->ty, (*rhs)->ty) ? (*rhs)->ty : (*lhs)->ty;
  *lhs = new_cast((*lhs)->tkn, *lhs, type);
  *rhs = new_cast((*rhs)->tkn, *rhs, type);
}

void add_type(Node *node) {
  if (node == NULL || node->ty != NULL) return;

  add_type(node->lhs);
  add_type(node->rhs);
  add_type(node->cond);
  add_type(node->then);
  add_type(node->other);
  add_type(node->init);
  add_type(node->loop);
  add_type(node->next);
  add_type(node->deep);

  switch (node->kind) {
    case ND_VAR:
      node->ty = node->use_var->ty;
      if (node->ty->kind == TY_ENUM) {
        node->ty = node->ty->base;
      }
      return;
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_REMAINDER:
    case ND_LEFTSHIFT:
    case ND_RIGHTSHIFT:
    case ND_BITWISEAND:
    case ND_BITWISEOR:
    case ND_BITWISEXOR:
    case ND_COND:
    case ND_EQ:
    case ND_NEQ:
    case ND_LC:
    case ND_LEC:
    case ND_LOGICALAND:
    case ND_LOGICALOR:
      if (node->lhs->ty->kind >= TY_PTR) {
        node->ty = node->lhs->ty;
        return;
      }
      if (node->rhs->ty->kind >= TY_PTR) {
        node->ty = node->rhs->ty;
        return;
      }
      implicit_cast(&node->lhs, &node->rhs);

      switch (node->kind) {
        case ND_EQ:
        case ND_NEQ:
        case ND_LC:
        case ND_LEC:
        case ND_LOGICALAND:
        case ND_LOGICALOR:
          node->ty = ty_bool;
          break;
        default:
          node->ty = node->lhs->ty;
      }
      return;
    case ND_ASSIGN:
    case ND_BITWISENOT:
      node->ty = node->lhs->ty;
      break;
    case ND_ADDR: {
      Type *ty = pointer_to(node->lhs->ty);
      node->ty = ty;
      return;
    }
    case ND_CONTENT: {
      node->ty = node->lhs->ty;
      if (node->ty != NULL) {
        node->ty = node->ty->base;
      }
      return;
    }
    case ND_FUNCCALL:
      node->ty = node->func->ty->ret_ty;
      return;
    case ND_BLOCK:
      node->ty = last_stmt(node->deep)->ty;
    case ND_INIT:
      return;
    default:
      if (node->lhs != NULL) node->ty = node->lhs->ty;
      if (node->rhs != NULL) node->ty = node->rhs->ty;
  }
}
