#pragma once
#include "token/tokenize.h"
#include "util/util.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct Node Node;
typedef struct Type Type;
typedef struct Obj Obj;
typedef struct Member Member;

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
  ND_COND,        // "?:" conditional
  ND_ADDR,        // "&" (Address-of)
  ND_CONTENT,     // "*" (Indirection, dereference)
  ND_COMMA,       // ","
  ND_ASSIGN,      // Assign
  ND_VAR,         // Variable
  ND_RETURN,      // "return" statement
  ND_IF,          // "if" statement
  ND_ELSE,        // "else" statement
  ND_FOR,         // "for" or "while" statement
  ND_DO,          // "do-while"
  ND_BLOCK,       // Block statement
  ND_SWITCH,      // "switch" statement
  ND_CASE,        // "case" statement
  ND_DEFAULT,     // "default" statement
  ND_BREAK,       // "break" statement
  ND_CONTINUE,    // "continue" statement
  ND_GOTO,        // "goto" statement
  ND_LABEL,       // label statement
  ND_FUNCCALL,    // Function call
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

  Obj *var; // Variable

  Node *init;  // Initialization (variable, for statement)
  Node *cond;  // condition (if, for, while)
  Node *then;  // cond true statement
  Node *other; // cond false statement
  Node *loop;  // Loop statement

  // Function call
  Obj *func;
  int argc;
  Node *args;
  bool pass_by_stack;

  char *label;
  char *break_label;
  char *conti_label;

  // swtich
  Node *case_stmt;
  Node *default_stmt;

  int64_t val;        // Value if kind is ND_NUM
  long double fval;   // Floating-value if kind is ND_NUM
};

char *new_unique_label();
int align_to(int bytes, int align);
Node *new_cast(Node *expr, Type *ty);
Node *new_var(Token *tkn, Obj *obj);
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
  TY_ARRAY,    // Array type
  TY_VLA,      // VLA (variable-length-array)
  TY_FUNC,     // Function
  TY_ENUM,     // Enum type
  TY_STRUCT,   // Struct type
  TY_UNION,    // Union type
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

  // VLA
  Node *vla_size;

  // Function type
  Type *ret_ty;
  bool is_prototype;

  // Function params
  int param_cnt;
  Type *params;
  Type *next;

  // Struct type
  int num_members;
  Member *members;
  int align;      // Alignment
  int bit_field;  // Bit field
  int bit_offset; // Bit offset 
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
extern Type *ty_f80;

Type *copy_type(Type *ty);
void init_type();
Type *pointer_to(Type *type);
Type *array_to(Type *type, int array_len);
bool is_integer_type(Type *ty);
bool is_float_type(Type *ty);
bool is_struct_type(Type *ty);
bool is_ptr_type(Type *ty);
bool is_same_type(Type *lty, Type *rty);
Type *extract_type(Type *ty);
Type *extract_arr_type(Type *ty);

// Variable or Function
struct Obj {
  Type *ty;  // Obj type
  
  char *name;
  int name_len;
  bool is_global;  // Global or Local

  // Local variable
  int offset;

  // Variable attribute
  bool is_static;

  // VLA
  bool is_allocate;

  // Function
  Obj *params;
  Obj *next;
  int vars_size;

  int64_t val;
  long double fval;
};

void add_type(Node *node);
Obj *new_obj(Type *type, char *name);
void enter_scope();
void leave_scope();
void add_var(Obj *var, bool set_offset);
void add_tag(Type *ty, char *name);
void add_type_def(Type *ty, char *name);
Obj *find_var(char *name);
Type *find_tag(char *name);
Type *find_type_def(char *name);
Obj *find_obj(char *name);
int init_offset();
bool declare_func(Type *ty, bool is_static);
bool define_func(Type *ty, bool is_static);

struct Member {
  Type *ty;
  Token *tkn;
  Member *next;

  char *name;
  int offset;
};

Member *find_member(Member *head, char *name);
void check_member(Member *head, char *name, Token *represent);
