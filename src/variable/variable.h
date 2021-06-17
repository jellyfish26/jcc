#pragma once

//
// type.c
//

typedef enum {
  TY_INT,   // "int" type
  TY_LONG,  // "long" type
  TY_PTR,   // Pointer type
  TY_ARRAY, // Array type
  TY_ADDR,  // Base address
} TypeKind;

typedef struct Type Type;
typedef struct Var Var;

struct Type {
  TypeKind kind;
  Type *content; // Content of variable if kind is TY_PTR

  int var_size; // Variable size
};

Type *new_general_type(TypeKind kind);
Type *new_pointer_type(Type *content_type);
Type *new_array_dimension_type(Type *content_type, int dimension_size);

//
// variable.c
//

struct Var {
  Type *var_type;
  Var *next; // Next Var

  char *str;  // Variable name
  int len;    // Length of name
  int offset; // Offset
};

Var *new_general_var(Type *var_type, char *str, int str_len);
void new_pointer_var(Var *var);
void new_array_dimension_var(Var *var, int dimension_size);
Var *new_content_var(Var *var);
Var *connect_var(Var *top_var, Type *var_type, char *str, int str_len);
Var *find_var(Var *top_var, char *str, int str_len);
int pointer_movement_size(Var *var);
int init_offset(Var *top_var);
