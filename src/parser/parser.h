#include <stdlib.h>

//
// parse.c
//

typedef enum {
    ND_ADD,  // +
    ND_VAR,  // Variable
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