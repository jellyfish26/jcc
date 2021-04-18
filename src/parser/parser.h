#pragma once
#include "../token/token.h"

#include <stdbool.h>

//
// type.c
//

typedef enum {
  TY_INT,  // "int" type
  TY_LONG, // "long" type
  TY_PTR   // pointer type
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
Type *ptr_type(Type *before);

//
// variable.c
//

struct Var {
  Type *var_type;
  Var *next; // Next Var

  char *str;  // Variable name
  int len;    // Length of naem
  int offset; // Offset
};

Var *add_var(Type *var_type, char *str, int len);
Var *find_var(Token *target);
void init_offset(Function *target);

//
// parse.c
//

typedef enum {
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_EQ,        // ==
  ND_NEQ,       // !=
  ND_LC,        // <  (Left Compare)
  ND_LEC,       // <= (Left Equal Compare)
  ND_RC,        // >  (Right Compare)
  ND_REC,       // >= (Right Equal Compare)
  ND_ASSIGN,    // =
  ND_VAR,       // Variable
  ND_RETURN,    // "return" statement
  ND_IF,        // "if" statement
  ND_ELSE,      // "else" statement
  ND_FOR,       // "for" statement
  ND_WHILE,     // "while" statement
  ND_BLOCK,     // Block statement
  ND_LOOPBREAK, // "break" statement (only for and while)
  ND_CONTINUE,  // "continue" statement
  ND_FUNCCALL,  // Function call
  ND_INT,       // Number (int)
  ND_ADDR,      // "&"
  ND_CONTENT,   // "*" hoge
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
