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
Node *mul();
Node *num();

// add = mul ("+" mul | "-" mul)*
Node *add() {
    Node *ret = mul();
    while (true) {
        if (use_operator("+")) {
            ret = new_node(ND_ADD, ret, mul());
        } else if (use_operator("-")) {
            ret = new_node(ND_SUB, ret, mul());
        } else {
            return ret;
        }
    }
    return ret;
}

// mul = num ("*" num | "/" num)*
Node *mul() {
    Node *ret = num();
    while (true) {
        if (use_operator("*")) {
            ret = new_node(ND_MUL, ret, num());
        } else if (use_operator("/")) {
            ret = new_node(ND_DIV, ret, num());
        } else {
            return ret;
        }
    }
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