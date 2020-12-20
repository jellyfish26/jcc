#include <stdlib.h>

//
// parse.c
//

typedef enum {
    ND_ADD,  // +
    ND_SUB,  // -
    ND_MUL,  // *
    ND_DIV,  // /
    ND_EQ,   // ==
    ND_NEQ,  // !=
    ND_LC,   // <  (Left Compare)
    ND_LEC,  // <= (Left Equal Compare)
    ND_RC,   // >  (Right Compare)
    ND_REC,  // >= (Right Equal Compare)
    ND_INT,  // Number (int)
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;  // Type of Node
    Node *lhs;      // Left side node
    Node *rhs;      // right side node

    int val; // value if kind is ND_VAR
};

extern Node *head_node;

void parse();