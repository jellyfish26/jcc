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

static Type *new_type(TypeKind kind, bool is_unsigned, int size) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = kind;
  ty->is_unsigned = is_unsigned;
  ty->var_size = size;
  ty->align = size;
  return ty;
}

Type *copy_type(Type *ty) {
  Type *cty = calloc(1, sizeof(Type));
  memcpy(cty, ty, sizeof(Type));
  return cty;
}

void init_type() {
  ty_void = new_type(TY_VOID, false, 0);
  ty_bool = new_type(TY_CHAR, false, 1);

  ty_i8  = new_type(TY_CHAR, false, 1);
  ty_i16 = new_type(TY_SHORT, false, 2);
  ty_i32 = new_type(TY_INT, false, 4);
  ty_i64 = new_type(TY_LONG, false, 8);

  ty_u8  = new_type(TY_CHAR, true, 1);
  ty_u16 = new_type(TY_SHORT, true, 2);
  ty_u32 = new_type(TY_INT, true, 4);
  ty_u64 = new_type(TY_LONG, true, 8);

  ty_f32 = new_type(TY_FLOAT, false, 4);
  ty_f64 = new_type(TY_DOUBLE, false, 8);
  ty_f80 = new_type(TY_LDOUBLE, false, 16);
}

// In the case of functions, we need to look at the type of the return type.
Type *extract_type(Type *ty) {
  if (ty->kind == TY_FUNC) {
    return ty->ret_ty;
  }
  return ty;
}

Type *extract_arr_type(Type *ty) {
  while (ty->base != NULL && ty->kind == TY_ARRAY) {
    ty = ty->base;
  }
  return ty;
}

Type *pointer_to(Type *base) {
  Type *ty = new_type(TY_PTR, false, 8);
  ty->base = base;
  return ty;
}

Type *array_to(Type *base, int array_len) {
  Type *ty = new_type(TY_ARRAY, false, array_len * base->var_size);
  ty->base = base;
  ty->array_len = array_len;
  ty->align = base->align;
  return ty;
}

bool is_integer_type(Type *ty) {
  switch (extract_type(ty)->kind) {
    case TY_CHAR:
    case TY_SHORT:
    case TY_INT:
    case TY_LONG:
      return true;
    default:
      return false;
  }
}

bool is_float_type(Type *ty) {
  switch (extract_type(ty)->kind) {
    case TY_FLOAT:
    case TY_DOUBLE:
    case TY_LDOUBLE:
      return true;
    default:
      return false;
  }
}

// The union are included in the structure the behave the same as structure,
// except that all members have zero offset.
bool is_struct_type(Type *ty) {
  switch (extract_type(ty)->kind) {
    case TY_STRUCT:
    case TY_UNION:
      return true;
    default:
      return false;
  }
}

bool is_ptr_type(Type *ty) {
  switch (extract_type(ty)->kind) {
    case TY_PTR:
    case TY_ARRAY:
    case TY_VLA:
    case TY_STRUCT:
    case TY_UNION:
      return true;
    default:
      return false;
  }
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
  HashMap type_def;
};

static Scope *scope = &(Scope){};

// The reason for allocating 8 bytes at the beginning is to keep
// track of how much space is allocated as a variable.
static int offset = 8;

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
  if (hashmap_get(&(scope->var), var->name) != NULL) {
    errorf(ER_COMPILE, "Variable '%s' is already declare", var->name);
  }
  hashmap_insert(&(scope->var), var->name, var);

  if (set_offset) {
    int sz = var->ty->var_size;
    offset = align_to(offset + sz, 8);
    var->offset = offset;
  }
}

void add_tag(Type *ty, char *name) {
  if (hashmap_get(&(scope->tag), name) != NULL) {
    errorf_tkn(ER_COMPILE, ty->tkn, "This tag is already declare");
  }
  hashmap_insert(&(scope->tag), name, ty);
}

void add_type_def(Type *ty, char *name) {
  if (hashmap_get(&(scope->type_def), name) != NULL) {
    errorf_tkn(ER_COMPILE, ty->tkn, "Name '%s' is already define", name);
  }
  hashmap_insert(&(scope->type_def), name, ty);
}

Obj *find_var(char *name) {
  for (Scope *cur = scope; cur != NULL; cur = cur->up) {
    Obj *obj = hashmap_get(&(cur->var), name);
    if (obj != NULL) {
      return obj;
    }
  }

  return NULL;
}

Type *find_tag(char *name) {
  for (Scope *cur = scope; cur != NULL; cur = cur->up) {
    Type *ty = hashmap_get(&(cur->tag), name);
    if (ty != NULL) {
      return ty;
    }
  }

  return NULL;
}

Type *find_type_def(char *name) {
  for (Scope *cur = scope; cur != NULL; cur = cur->up) {
    Type *ty = hashmap_get(&(cur->type_def), name);
    if (ty != NULL) {
      return ty;
    }
  }

  return NULL;
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

bool declare_func(Type *ty, bool is_static) {
  ty->is_prototype = true;
  Obj *already = find_var(ty->name);

  if (already == NULL) {
    Obj *obj = new_obj(ty, ty->name);
    obj->is_static = is_static;
    add_var(obj, false);

    if (is_static) {
      obj->name = new_unique_label();
    }

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
    Obj *obj = new_obj(ty, ty->name);
    obj->is_static = already->is_static | is_static;
    hashmap_insert(&(scope->var), ty->name, obj);

    if (obj->is_static) {
      obj->name = new_unique_label();
    }

    return true;
  }

  return check_func_params(ty, already->ty);
}

bool define_func(Type *ty, bool is_static) {
  ty->is_prototype = false;
  Obj *alrady = find_var(ty->name);

  if (alrady == NULL) {
    Obj *obj = new_obj(ty, ty->name);
    obj->is_static = is_static;
    add_var(obj, false);

    if (is_static) {
      obj->name = new_unique_label();
    }

    return true;
  }

  if (!alrady->ty->is_prototype || !check_func_params(ty, alrady->ty)) {
    return false;
  }

  Obj *obj = new_obj(ty, ty->name);
  obj->is_static = alrady->is_static | is_static;
  hashmap_insert(&(scope->var), ty->name, obj);

  if (obj->is_static) {
    obj->name = new_unique_label();
  }

  return true;
}

int init_offset() {
  int sz = offset + 16 - (offset % 16);
  offset = 8;
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
  Type *ty = comp_type(extract_type((*lhs)->ty), extract_type((*rhs)->ty)) ? (*rhs)->ty : (*lhs)->ty;
  *lhs = new_cast(*lhs, extract_type(ty));
  *rhs = new_cast(*rhs, extract_type(ty));
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
      node->ty = node->var->ty;
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
      if (extract_type(node->lhs->ty)->kind >= TY_PTR) {
        node->ty = node->lhs->ty;
        return;
      }
      if (extract_type(node->rhs->ty)->kind >= TY_PTR) {
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
    case ND_COMMA:
      node->ty = last_stmt(node->deep)->ty;
    case ND_INIT:
      return;
    default:
      if (node->lhs != NULL) node->ty = node->lhs->ty;
      if (node->rhs != NULL) node->ty = node->rhs->ty;
  }
}

Member *find_member(Member *head, char *name) {
  for (Member *member = head; member != NULL; member = member->next) {
    if (member->name != NULL && memcmp(member->name, name, strlen(name)) == 0) {
      return member;
    }
  }
  return NULL;
}

void check_member(Member *head, char *name, Token *represent) {
  if (find_member(head, name) != NULL) {
    errorf_tkn(ER_COMPILE, represent, "Duplicate member");
  }
}
