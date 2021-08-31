#include "token/tokenize.h"
#include "parser/parser.h"

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

static Type *new_type(TypeKind kind, bool is_unsigned, int size) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = kind;
  ty->is_unsigned = is_unsigned;
  ty->var_size = size;
  return ty;
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
}

Type *pointer_to(Type *base) {
  Type *ty = new_type(TY_PTR, false, 8);
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

Obj *new_obj(Type *type, char *name) {
  if (type == NULL) {
    return NULL;
  }
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

  Obj *vars;
  Obj *others;  // Tag: function declaration / function definition
};

Scope *lscope;
Scope *gscope;
Obj *used_objs;

void new_scope() {
  Scope *scope = calloc(1, sizeof(Scope));
  scope->up = lscope;
  lscope = scope;
}

void del_scope() {
  if (lscope == NULL) {
    errorf(ER_INTERNAL, "Internal error at scope");
  }

  if (lscope->vars != NULL) {
    Obj *end = lscope->vars;
    while (end->next != NULL) {
      end = end->next;
    }
    end->next = used_objs;
    used_objs = lscope->vars;
  }

  lscope = lscope->up;
}

void add_lvar(Obj *var) {
  var->is_global = false;
  var->next = lscope->vars;
  lscope->vars = var;
}

void add_lobj(Obj *obj) {
  obj->is_global = false;
  obj->next = lscope->others;
  lscope->others = obj;
}

void add_gvar(Obj *var, bool is_substance) {
  var->is_global = true;
  if (gscope == NULL) {
    gscope = calloc(1, sizeof(Scope));
  }

  if (gscope->vars == NULL) {
    var->offset = 0;
  } else {
    var->offset = gscope->vars->offset + (is_substance ? 0 : 1);
  }

  var->next = gscope->vars;
  gscope->vars = var;
}

void add_gobj(Obj *obj) {
  obj->is_global = true;
  if (gscope == NULL) {
    gscope = calloc(1, sizeof(Scope));
  }

  obj->next = gscope->others;
  gscope->others = obj;
}

Obj *check_objs(Obj *head, char *name) {
  int len = strlen(name);
  for (Obj *obj = head; obj != NULL; obj = obj->next) {
    if ((len == obj->name_len) && (memcmp(name, obj->name, len) == 0)) {
      return obj;
    }
  }

  return NULL;
}

Obj *find_obj(char *name) {
  // Find in local object
  for (Scope *cur = lscope; cur != NULL; cur = cur->up) {
    Obj *obj = check_objs(cur->vars, name);
    if (obj != NULL) {
      return obj;
    }

    obj = check_objs(cur->others, name);
    if (obj != NULL) {
      return obj;
    }
  }

  if (gscope == NULL) {
    return NULL;
  }

  Obj *obj = check_objs(gscope->vars, name);
  if (obj != NULL) {
    return obj;
  }

  obj = check_objs(gscope->others, name);
  return obj;
}

bool check_scope(char *name) {
  bool ret = false;
  if (gscope == NULL) {
    return NULL;
  }

  if (lscope == NULL) {
    ret |= (check_objs(gscope->vars, name) != NULL);
    ret |= (check_objs(gscope->others, name) != NULL);
  } else {
    ret |= (check_objs(lscope->vars, name) != NULL);
    ret |= (check_objs(lscope->others, name) != NULL);
  }
  return ret;
}

bool check_func_params(Type *lty, Type *rty) {
  // If the parameters are same type, we can be redeclared.
  bool chk = (lty->param_cnt == rty->param_cnt);

  if (chk) {
    for (int i = 0; i < lty->param_cnt; i++) {
      if (!is_same_type(*(lty->params + i), *(rty->params + i))) {
        chk = false;
        break;
      }
    }
  }

  return chk;
}

bool declare_func(Type *ty) {
  ty->is_prototype = true;
  Obj *already = find_obj(ty->name);

  if (already == NULL) {
    add_gobj(new_obj(ty, ty->name));
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
    already->name_len = 0;
    add_gobj(new_obj(ty, ty->name));
    return true;
  }

  return check_func_params(ty, already->ty);
}

bool define_func(Type *ty) {
  ty->is_prototype = false;
  Obj *alrady = find_obj(ty->name);

  if (alrady == NULL) {
    add_gobj(new_obj(ty, ty->name));
    return true;
  }

  if (!alrady->ty->is_prototype || !check_func_params(ty, alrady->ty)) {
    return false;
  }

  alrady->name_len = 0;
  add_gobj(new_obj(ty, ty->name));
  return true;
}

int init_offset() {
  int now_address = 0;

  for (Obj *cur = used_objs; cur != NULL; cur = cur->next) {
    now_address += cur->ty->var_size;
    cur->offset = now_address;
  }

  used_objs = NULL;
  now_address += 16 - (now_address % 16);
  return now_address;
}

Obj *get_gvars() {
  return gscope->vars;
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
        case ND_LOGICALNOT:
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
    case ND_LOGICALNOT:
      node->ty = ty_bool;
      return;
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
