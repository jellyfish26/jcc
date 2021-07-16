#pragma once
#include "token/tokenize.h"
#include "variable/variable.h"

#include <stdbool.h>

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
  ND_FUNCARG,     // Function argument
  ND_ADDR,        // "&" (Address-of)
  ND_CONTENT,     // "*" (Indirection, dereference)
  ND_PREFIX_INC,  // Prefix increment
  ND_PREFIX_DEC,  // Prefix decrement
  ND_SUFFIX_INC,  // Suffix increment
  ND_SUFFIX_DEC,  // Suffix decrement
  ND_SIZEOF,      // "sizeof"
  ND_INT,         // Number (int)
} NodeKind;

struct Node {
  NodeKind kind; // Type of Node
  Node *lhs;     // Left side node
  Node *rhs;     // Right side node

  Type *equation_type; // Size of equation
  Var *use_var; // Use target
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

  char *func_name;   // Function name
  int func_name_len; // Function name length
  Node *func_arg;    // Function arguments
  int func_args_idx; // Index of argument

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
  Var *vars; // Definition variables

  Node *func_args; // Function arguments
  int func_argc;   // Count of function arguments
  int vars_size;

  bool global_var_define;
};
