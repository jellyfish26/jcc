#include "token/tokenize.h"
#include "parser/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Type *new_type(TypeKind kind, bool is_real) {
  Type *ret = calloc(sizeof(Type), 1);
  ret->kind = kind;
  ret->is_real = is_real;
  switch(kind) {
    case TY_VOID:
      ret->var_size = 0;
      break;
    case TY_CHAR:
      ret->var_size = 1;
      break;
    case TY_SHORT:
      ret->var_size = 2;
      break;
    case TY_INT:
      ret->var_size = 4;
      break;
    case TY_STR:
    case TY_LONG:
    case TY_PTR:
      ret->var_size = 8;
      break;
    default:
      return NULL;
  }
  return ret;
}

Type *pointer_to(Type *type) {
  Type *ret = new_type(TY_PTR, true);
  ret->base = type;
  type->is_real = false;
  return ret;
}

Type *array_to(Type *type, int array_len) {
  Type *ret = calloc(1, sizeof(Type));
  ret->kind = TY_ARRAY;
  ret->is_real = false;
  ret->base = type;
  ret->var_size = array_len * type->var_size;
  ret->array_len = array_len;
  return ret;
}

Obj *new_obj(Type *type, char *name) {
  if (type == NULL) {
    return NULL;
  }
  int name_len = strlen(name);
  Obj *ret = calloc(1, sizeof(Obj));
  ret->type = type;
  ret->name = calloc(name_len + 1, sizeof(char));
  memcpy(ret->name, name, name_len);
  ret->name_len = name_len;
  return ret;
}

ScopeObj *lvars;
Obj *gvars;
Obj *used_vars;

void new_scope_definition() {
  ScopeObj *new_scope = calloc(1, sizeof(ScopeObj));
  if (lvars == NULL) {
    new_scope->depth = 0;
  } else {
    new_scope->depth = lvars->depth + 1;
  }
  new_scope->upper = lvars;
  lvars = new_scope;
}

void out_scope_definition() {
  if (!lvars) {
    errorf(ER_INTERNAL, "Internal Error at scope");
  }
  Obj *used = lvars->objs;
  while (used && used->next) {
    used = used->next;
  }
  if (used) {
    used->next = used_vars;
    used_vars = lvars->objs;
  }
  lvars = lvars->upper;
}

void add_lvar(Obj *var) {
  var->is_global = false;
  var->next = lvars->objs;
  lvars->objs = var;
}

void add_gvar(Obj *var, bool is_substance) {
  var->is_global = true;
  var->next = gvars;
  if (gvars == NULL) {
    var->offset = 0;
  } else {
    if (is_substance) {
      var->offset = gvars->offset;
    } else {
      var->offset = gvars->offset + 1;
    }
  }
  gvars = var;
}

Obj *find_var(char *name) {
  int name_len = strlen(name);
  // Find in local variable
  for (ScopeObj *now_scope = lvars; now_scope != NULL; now_scope = now_scope->upper) {
    for (Obj *obj = now_scope->objs; obj != NULL; obj = obj->next) {
      if (name_len != obj->name_len) {
        continue;
      }

      if (memcmp(name, obj->name, name_len) == 0) {
        return obj;
      }
    }
  }

  // Find in global varaible
  for (Obj *obj = gvars; obj != NULL; obj = obj->next) {
    if (name_len != obj->name_len) {
      continue;
    }

    if (memcmp(name, obj->name, name_len) == 0) {
      return obj;
    }
  }
  return NULL;
}


int init_offset() {
  int now_address = 0;
  Obj *now = used_vars;
  while (now) {
    now_address += now->type->var_size;
    now->offset = now_address;
    now = now->next;
  }
  used_vars = NULL;
  now_address += 16 - (now_address % 16);
  return now_address;
}

bool check_already_define(char *name, bool is_global) {
  int name_len = strlen(name);
  if (is_global) {
     for (Obj *gvar = gvars; gvar != NULL; gvar = gvar->next) {
      if (gvar->name_len != name_len) {
        continue;
      }
      if (memcmp(name, gvar->name, name_len) == 0) {
        return true;
      }
    }
    return false;
  }
  if (lvars == NULL) {
    return false;
  }
  for (Obj *now_var = lvars->objs; now_var != NULL; now_var = now_var->next) {
    if (now_var->name_len != name_len) {
      continue;
    }

    if (memcmp(name, now_var->name, name_len) == 0) {
      return true;
    }
  }
  return false;
}

// if type size left < right is true
// other is false
static bool comp_type(Type *left, Type *right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  if (left->kind < right->kind) {
    return true;
  }
  return false;
}

static void implicit_cast(Node **lhs, Node **rhs) {
  Type *type = comp_type((*lhs)->type, (*rhs)->type) ? (*rhs)->type : (*lhs)->type;
  *lhs = new_cast((*lhs)->tkn, *lhs, type);
  *rhs = new_cast((*rhs)->tkn, *rhs, type);
}

void add_type(Node *node) {
  if (node == NULL || node->type != NULL) return;

  add_type(node->lhs);
  add_type(node->rhs);
  add_type(node->cond);
  add_type(node->then);
  add_type(node->other);
  add_type(node->init);
  add_type(node->loop);
  add_type(node->next_stmt);
  add_type(node->next_block);

  switch (node->kind) {
    case ND_VAR:
      node->type = node->use_var->type;
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
    case ND_TERNARY:
      if (node->lhs->type != NULL && node->lhs->type->kind >= TY_PTR) {
        node->type = node->lhs->type;
        return;
      }
      if (node->rhs->type != NULL && node->rhs->type->kind >= TY_PTR) {
        node->type = node->rhs->type;
        return;
      }
      implicit_cast(&node->lhs, &node->rhs);
      node->type = node->lhs->type;
      return;
    case ND_ASSIGN:
      node->type = node->lhs->type;
      break;
    case ND_BITWISENOT:
    case ND_EQ:
    case ND_NEQ:
    case ND_LC:
    case ND_LEC:
    case ND_LOGICALAND:
    case ND_LOGICALOR:
    case ND_LOGICALNOT:
    case ND_INT:
      node->type = new_type(TY_INT, false);
      return;
    case ND_ADDR: {
      Type *type = new_type(TY_PTR, false);
      type->base = node->lhs->type;
      node->type = type;
      return;
    }
    case ND_CONTENT: {
      node->type = node->lhs->type;
      if (node->type != NULL) {
        node->type = node->type->base;
      }
      return;
    }
    case ND_FUNCCALL:
      node->type = node->func->type;
    default:
      if (node->lhs != NULL) node->type = node->lhs->type;
      if (node->rhs != NULL) node->type = node->rhs->type;
  }
}
