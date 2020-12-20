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
Node *formula();
Node *same_comp();
Node *size_comp();
Node *add();
Node *mul();
Node *unary();
Node *priority();
Node *num();

// formula = same_comp
Node *formula() {
    return same_comp();
}

// same_comp = size_comp ("==" size_comp | "!=" size_comp)*
Node *same_comp() {
    Node *ret = size_comp();
    while (true) {
        if (use_symbol("==")) {
            ret = new_node(ND_EQ, ret, size_comp());
        } else if (use_symbol("!=")) {
            ret = new_node(ND_NEQ, ret, size_comp());
        } else {
            break;
        }
    }
    return ret;
}

// size_comp = add ("<" add | ">" add | "<=" add | ">=" add)*
Node *size_comp() {
    Node *ret = add();
    while (true) {
        if (use_symbol("<")) {
            ret = new_node(ND_LC, ret, add());
        } else if (use_symbol(">")) {
            ret = new_node(ND_RC, ret, add());
        } else if (use_symbol("<=")) {
            ret = new_node(ND_LEC, ret, add());
        } else if (use_symbol(">=")) {
            ret = new_node(ND_REC, ret, add());
        } else {
            break;
        }
    }
    return ret;
}

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

// priority = num | "(" formula ")"
Node *priority() {
    if (use_symbol("(")) {
        Node *ret = formula();
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
    head_node = formula();
}