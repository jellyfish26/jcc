#include "parser.h"
#include "../read/read.h"

Node *head_node;

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = kind;
    ret->lhs = lhs;
    ret->rhs = rhs;
    return ret;
}

// Prototype
Node *add();
Node *num();

// add = num (+ | -) num
Node *add() {
    Node *ret = new_node(ND_ADD, NULL, NULL);
    ret->lhs = num();
    if (use_operator("-")) {
        ret->kind = ND_SUB;
    } else {
        use_expect_operator("+");
    }
    ret->rhs = num();
    return ret;
}

Node *num() {
    Node *ret = new_node(ND_VAR, NULL, NULL);
    ret->val = use_expect_int();
    return ret;
}

void parse() {
    head_node = add();
}