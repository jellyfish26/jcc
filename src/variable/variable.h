#pragma once
#include <stdbool.h>

// Define in order of decreasing size
typedef enum {
  TY_STR,   // String literal type
  TY_CHAR,  // "char" type
  TY_SHORT, // "short" type
  TY_INT,   // "int" type
  TY_LONG,  // "long" type
  TY_ADDR,  // Address value
  TY_PTR,   // Pointer type
  TY_ARRAY, // Array type
} TypeKind;

typedef struct Type Type;
typedef struct Var Var;
typedef struct ScopeVars ScopeVars;

struct Type {
  TypeKind kind;
  Type *content; // Content of variable if kind is TY_PTR

  int var_size;  // Variable size
  bool is_real;  // Whether or not value has a place to be stored. (etc. False is array, num)
};

Type *new_general_type(TypeKind kind, bool is_real);
Type *new_pointer_type(Type *content_type);
Type *new_array_dimension_type(Type *content_type, int dimension_size);
int pointer_movement_size(Type *var_type);

struct Var {
  Type *var_type;
  Var *next; // Next Var

  char *str;  // Variable name (String, if TY_STR)
  int len;    // Length of name
  int offset; // Offset
  bool global; // Global variable if this variable is true
};

Var *new_general_var(Type *var_type, char *str, int str_len);
void new_pointer_var(Var *var);
void new_array_dimension_var(Var *var, int dimension_size);
Var *new_content_var(Var *var);
Var *connect_var(Var *top_var, Type *var_type, char *str, int str_len);
int get_sizeof(Var *var);

struct ScopeVars {
  int depth; // Scope depth -> 0 is general, over 1 is local
  int use_address;
  ScopeVars *upper;
  Var *vars;
};

extern ScopeVars *lvars;
extern Var *tmp_vars;
extern Var *gvars;
extern Var *used_vars;

void new_scope_definition();
void out_scope_definition();
void add_lvar(Var *var);
void add_tmp_var(Var *var);
void add_gvar(Var *var);
Var *find_var(char *str, int str_len);
bool check_already_define(char *str, int str_len, bool is_global);
int init_offset();
