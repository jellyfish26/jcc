#pragma once
#include "../token/token.h"

#include <stdbool.h>

//
// type.c
//

typedef enum {
  TY_INT,       // "int" type
  TY_LONG,      // "long" type
  TY_PTR,       // Pointer type
  TY_ARRAY,     // Array type
  TY_ADDR,      // Base address
} TypeKind;

typedef struct Type Type;
typedef struct Var Var;
typedef struct Node Node;
typedef struct Function Function;

struct Type {
  TypeKind kind;
  Type *content; // Content of variable if kind is TY_PTR

  int type_size; // Variable size
  int move_size; // Movement size
};

Type *gen_type();
Type *connect_ptr_type(Type *before);
Type *connect_array_type(Type *before, int array_size);
Type *get_type_for_node(Node *target);
void raise_type_for_node(Node *target);

//
// variable.c
//

struct Var {
  Type *var_type;
  Var *next; // Next Var
  Var *definition;  // Variable definition if var_type is TY_PTR

  char *str;  // Variable name
  int len;    // Length of naem
  int offset; // Offset
};

Var *add_var(Type *var_type, char *str, int len);
Var *find_var(Token *target);
void init_offset(Function *target);
Var *down_type_level(Var *target);

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
  ND_LOGICALAND,  // "&&" (Logical AND)
  ND_ASSIGN,      // =
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
  ND_INT,         // Number (int)
  ND_ADDR,        // "&" (Address-of)
  ND_CONTENT,     // "*" (Indirection, dereference)
} NodeKind;

struct Node {
  NodeKind kind; // Type of Node
  Node *lhs;     // Left side node
  Node *rhs;     // Right side node

  Var *var; // Variable type if kind is ND_VAR

  Node *judge;     // judge ("if" statement, "for" statement, "while" statement)
  Node *exec_if;   // exec ("if" statement)
  Node *exec_else; // exec ("else" statement)

  Node *init_for;   // init before exec "for"
  Node *repeat_for; // repeact in exec "for"
  Node *stmt_for;   // statement in exec "for"

  Node *next_stmt; // Block statement

  int val;   // value if kind is ND_INT
  int label; // label (only "for" or "while" statement)

  char *func_name;   // Function name
  int func_name_len; // Function name length
  Node *func_arg;    // Function arguments
  int func_args_idx; // Index of argument
};

void program();

struct Function {
  char *func_name;   // Function name
  int func_name_len; // Function name length

  Node *stmt;     // Node of statement
  Function *next; // Next function
  Type *ret_type; // Type of function return

  Var *vars;

  Node *func_args; // Function arguments
  int func_argc;   // Count of function arguments
  int vars_size;
};

extern Function *top_func; // parse.c
extern Function *exp_func;
