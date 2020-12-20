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
Node *priority();
Node *num();

// add = mul ("+" mul | "-" mul)*
Node *add() {
    Node *ret = mul();
    while (true) {
        if (use_symbol("+")) {
            ret = new_node(ND_ADD, ret, mul());
        } else if (use_symbol("-")) {
            ret = new_node(ND_SUB, ret, mul());
        } else {
            return ret;
        }
    }
    return ret;
}

// mul = priority ("*" priority | "/" priority)*
Node *mul() {
    Node *ret = priority();
    while (true) {
        if (use_symbol("*")) {
            ret = new_node(ND_MUL, ret, priority());
        } else if (use_symbol("/")) {
            ret = new_node(ND_DIV, ret, priority());
        } else {
            return ret;
        }
    }
    return ret;
}

// priority = num | "(" add ")"
Node *priority() {
    if (use_symbol("(")) {
        Node *ret = add();
        use_expect_symbol(")");
        return ret;
    }
    return num();
}


Node *num() {
    Node *ret = new_node(ND_VAR, NULL, NULL);
    ret->val = use_expect_int();
    return ret;
}

void parse() {
    head_node = add();
}