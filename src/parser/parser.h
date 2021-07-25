#pragma once
#include "token/tokenize.h"

#include <stdbool.h>

typedef struct Type Type;
typedef struct Obj Obj;
typedef struct ScopeObj ScopeObj;
typedef struct Node Node;
typedef struct Function Function;

//
// parse.c
//

typedef enum {
  ND_ADD,         // +
  ND_SUB,         // -
  ND_MUL,         // *
  ND_DIV,         // /
  ND_REMAINDER,   // %
  ND_EQ,          // ==
  ND_NEQ,         // !=
  ND_LC,          // <  (Left Compare)
  ND_LEC,         // <= (Left Equal Compare)
  ND_RC,          // >  (Right Compare)
  ND_REC,         // >= (Right Equal Compare)
  ND_LEFTSHIFT,   // << (Bitwise left shift)
  ND_RIGHTSHIFT,  // >> (Bitwise right shift)
  ND_BITWISEAND,  // "&" (Bitwise AND)
  ND_BITWISEXOR,  // "^" (Bitwise XOR)
  ND_BITWISEOR,   // "|" (Bitwise OR)
  ND_BITWISENOT,  // "~" (Bitwise NOT)
  ND_LOGICALAND,  // "&&" (Logical AND)
  ND_LOGICALOR,   // "||" (Logical OR)
  ND_LOGICALNOT,  // "!" (Logical NOT)
  ND_TERNARY,     // Ternay operator
  ND_ASSIGN,      // Assign
  ND_VAR,         // Variable
  ND_RETURN,      // "return" statement
  ND_IF,          // "if" statement
  ND_ELSE,        // "else" statement
  ND_FOR,         // "for" statement
  ND_WHILE,       // "while" statement
  ND_BLOCK,       // Block statement
  ND_LOOPBREAK,   // "break" statement (only for and while)
  ND_CONTINUE,    // "continue" statement
  ND_FUNCCALL,    // Function call
  ND_ADDR,        // "&" (Address-of)
  ND_CONTENT,     // "*" (Indirection, dereference)
  ND_PREFIX_INC,  // Prefix increment
  ND_PREFIX_DEC,  // Prefix decrement
  ND_SUFFIX_INC,  // Suffix increment
  ND_SUFFIX_DEC,  // Suffix decrement
  ND_SIZEOF,      // "sizeof"
  ND_INT,         // Number (int)
  ND_CAST,        // Cast
} NodeKind;

struct Node {
  NodeKind kind; // Type of Node
  Node *lhs;     // Left side node
  Node *rhs;     // Right side node
  Type *type;

  Obj *use_var; // Use target
  bool is_var_define_only;

  Node *judge;     // judge ("if" statement, "for" statement, "while" statement)
  Node *exec_if;   // exec ("if" statement)
  Node *exec_else; // exec ("else" statement)

  Node *init_for;   // init before exec "for"
  Node *repeat_for; // repeact in exec "for"
  Node *stmt_for;   // statement in exec "for"

  Node *next_stmt;  // Next statement
  Node *next_block; // Next Block

  NodeKind assign_type; // Type of assign

  int label; // label (only "for" or "while" statement)

  Obj *func;  // Function call

  int val;        // value if kind is ND_INT
  char *str_lit;  // value if kind is ND_STR
  int str_lit_label;
};

Function *program(Token *tkn);

struct Function {
  char *func_name;   // Function name
  int func_name_len; // Function name length

  Node *stmt;     // Node of statement
  Function *next; // Next function
  Type *ret_type; // Type of function return
  Obj *vars;      // Definition variables

  Node *func_args; // Function arguments
  int func_argc;   // Count of function arguments
  int vars_size;
};

//
// object.c
//

// Define in order of decreasing size
typedef enum {
  TY_CHAR,  // "char" type
  TY_SHORT, // "short" type
  TY_INT,   // "int" type
  TY_LONG,  // "long" type
  TY_PTR,   // Pointer type
  TY_STR,   // String literal type
  TY_ARRAY, // Array type
} TypeKind;


struct Type {
  TypeKind kind;
  Type *content; // Content of variable if kind is TY_PTR

  int var_size;  // Variable size
  bool is_real;  // Whether or not value has a place to be stored. (etc. False is array, num)
};

Type *new_type(TypeKind kind, bool is_real);
Type *pointer_to(Type *type);
Type *array_to(Type *type, int dim_size);

// Variable or Function
struct Obj {
  Type *type;
  Obj *next;
  
  char *name;
  int name_len;
  bool is_global;  // Global or Local

  // Local variable
  int offset;

  // Function
  int argc;
  Node *args;
};

Obj *new_obj(Type *type, char *name);

struct ScopeObj {
  int depth; // Scope depth -> 0 is general, over 1 is local
  int use_address;
  ScopeObj *upper;
  Obj *objs;
};

extern ScopeObj *lvars;
extern Obj *gvars;
extern Obj *used_vars;

void new_scope_definition();
void out_scope_definition();
void add_lvar(Obj *var);
void add_gvar(Obj *var);
Obj *find_var(char *name);
bool check_already_define(char *name, bool is_global);
int init_offset();
