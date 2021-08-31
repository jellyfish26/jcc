#pragma once
#include "token/tokenize.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct Type Type;
typedef struct Obj Obj;
typedef struct Node Node;
typedef struct Function Function;

//
// parse.c
//

typedef enum {
  ND_VOID,        // Nothing
  ND_FUNC,        // Function
  ND_ADD,         // +
  ND_SUB,         // -
  ND_MUL,         // *
  ND_DIV,         // /
  ND_REMAINDER,   // %
  ND_EQ,          // ==
  ND_NEQ,         // !=
  ND_LC,          // <  (Left Compare)
  ND_LEC,         // <= (Left Equal Compare)
  ND_LEFTSHIFT,   // << (Bitwise left shift)
  ND_RIGHTSHIFT,  // >> (Bitwise right shift)
  ND_BITWISEAND,  // "&" (Bitwise AND)
  ND_BITWISEXOR,  // "^" (Bitwise XOR)
  ND_BITWISEOR,   // "|" (Bitwise OR)
  ND_BITWISENOT,  // "~" (Bitwise NOT)
  ND_LOGICALAND,  // "&&" (Logical AND)
  ND_LOGICALOR,   // "||" (Logical OR)
  ND_LOGICALNOT,  // "!" (Logical NOT)
  ND_COND,        // "?:" conditional
  ND_ASSIGN,      // Assign
  ND_VAR,         // Variable
  ND_RETURN,      // "return" statement
  ND_IF,          // "if" statement
  ND_ELSE,        // "else" statement
  ND_FOR,         // "for" or "while" statement
  ND_BLOCK,       // Block statement
  ND_BREAK,       // "break" statement
  ND_CONTINUE,    // "continue" statement
  ND_FUNCCALL,    // Function call
  ND_ADDR,        // "&" (Address-of)
  ND_CONTENT,     // "*" (Indirection, dereference)
  ND_NUM,         // Number (int)
  ND_CAST,        // Cast
  ND_INIT,        // Initializer
} NodeKind;

struct Node {
  NodeKind kind; // Node kind
  Token *tkn;    // Representative token
  Type *ty;      // Node type
  Node *next;    // Next node

  Node *lhs;     // Left side node
  Node *rhs;     // Right side node

  Node *deep;    // Block or statement

  Obj *use_var; // Use target

  Node *init;  // Initialization (variable, for statement)
  Node *cond;  // condition (if, for, while)
  Node *then;  // cond true statement
  Node *other; // cond false statement
  Node *loop;  // Loop statement

  char *break_label;
  char *conti_label;

  Obj *func;    // Function or Function call
  int argc;     // Arguments count
  Node **args;  // Arguemnts

  int64_t val;        // Value if kind is ND_NUM
  long double fval;   // Floating-value if kind is ND_NUM
};

Node *new_node(NodeKind kind, Token *tkn);
Node *new_num(Token *tkn, int64_t val);
Node *new_cast(Token *tkn, Node *lhs, Type *ty);
Node *new_var(Token *tkn, Obj *obj);
Node *new_strlit(Token *tkn, char *strlit);
Node *new_assign(Token *tkn, Node *lhs, Node *rhs);
Node *to_assign(Token *tkn, Node *rhs);
Node *last_stmt(Node *now);

Node *program(Token *tkn);

//
// object.c
//

// Define in order of decreasing size
typedef enum {
  TY_VOID,     // "void" 
  TY_CHAR,     // "char"
  TY_SHORT,    // "short"
  TY_INT,      // "int"
  TY_LONG,     // "long" or "long long"
  TY_FLOAT,    // "float"
  TY_DOUBLE,   // "double"
  TY_LDOUBLE,  // "long double"
  TY_PTR,      // Pointer type
  TY_STR,      // String literal type
  TY_ARRAY,    // Array type
  TY_FUNC,     // Function
} TypeKind;


struct Type {
  TypeKind kind;
  Token *tkn;  // Representative token

  bool is_const;
  bool is_unsigned;

  // Declaration
  char *name;

  // Pointer and Array require the same behavior,
  // and need a base type to calculate the memory movement distance.
  // Otherwise, this variable have raw type.
  Type *base;

  int var_size;  // Variable size
  int array_len; // Array length if kind is TY_ARRAY

  // Function type
  Type *ret_ty;
  int param_cnt;
  Type **params;
  Type *next;
  bool is_prototype;
};

// Must call init_type function before use.
extern Type *ty_void;
extern Type *ty_bool;

extern Type *ty_i8;
extern Type *ty_i16;
extern Type *ty_i32;
extern Type *ty_i64;

extern Type *ty_u8;
extern Type *ty_u16;
extern Type *ty_u32;
extern Type *ty_u64;

extern Type *ty_f32;
extern Type *ty_f64;

void init_type();
Type *pointer_to(Type *type);
Type *array_to(Type *type, int array_len);
bool is_float_ty(Type *ty);
bool is_same_type(Type *lty, Type *rty);
bool declare_func(Type *ty);
bool define_func(Type *ty);

// Variable or Function
struct Obj {
  Type *ty;  // Obj type
  Obj *next;
  
  char *name;
  int name_len;
  bool is_global;  // Global or Local

  // Local variable
  int offset;

  // Function
  Obj **params;
  int vars_size;

  int64_t val;
  long double fval;
};

void add_type(Node *node);
Obj *new_obj(Type *type, char *name);
void new_scope();
void del_scope();
void add_lvar(Obj *var);
void add_lobj(Obj *obj);
void add_gvar(Obj *var, bool is_substance);
void add_gobj(Obj *obj);
Obj *find_obj(char *name);
Obj *get_gvars();
bool check_scope(char *name);
int init_offset();
