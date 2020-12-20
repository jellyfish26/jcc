#include "parser.h"
#include "../token/token.h"

Node *head_node;

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = kind;
    ret->lhs = lhs;
    ret->rhs = rhs;
    return ret;
}

Node *new_node_int(int val) {
    Node *ret = calloc(1, sizeof(Node));
    ret->kind = ND_INT;
    ret->val = val;
    return ret;
}

// Prototype
Node *add();
Node *mul();
Node *unary();
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

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
    Node *ret = unary();
    while (true) {
        if (use_symbol("*")) {
            ret = new_node(ND_MUL, ret, unary());
        } else if (use_symbol("/")) {
            ret = new_node(ND_DIV, ret, unary());
        } else {
            return ret;
        }
    }
    return ret;
}

// unary = ("+" | "-")? primariy
Node *unary() {
    if (use_symbol("+")) {
        Node *ret = new_node(ND_ADD, new_node_int(0), priority());
        return ret;
    } else if (use_symbol("-")) {
        Node *ret = new_node(ND_SUB, new_node_int(0), priority());
        return ret;
    }
    return priority();
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
    Node *ret = new_node(ND_INT, NULL, NULL);
    ret->val = use_expect_int();
    return ret;
}

void parse() {
    head_node = add();
}