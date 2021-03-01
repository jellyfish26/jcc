#include <stdlib.h>

#include "parser.h"

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
void program();
Node *statement();
Node *assign();
Node *same_comp();
Node *size_comp();
Node *add();
Node *mul();
Node *unary();
Node *priority();
Node *num();

void *function();

Function *top_func;
Function *exp_func;

void program() {
    int i = 0;
    while (!is_eof()) {
        if (exp_func) {
            exp_func->next = calloc(1, sizeof(Function));
            exp_func = exp_func->next;
            function(exp_func);
        } else {
            exp_func = calloc(1, sizeof(Function));
            top_func = exp_func;
            function(exp_func);
        }
    }
}

// function = ident "(" params?")" statement
// params = ident ("," ident)*
void *function(Function *target) {
    Token *tkn = use_any_kind(TK_IDENT);
    if (tkn) {
        use_expect_symbol("(");
        if (!use_symbol(")")) {
            // Set arguments
            while (true) {
                Token *tkn = use_any_kind(TK_IDENT);
                if (tkn) {
                    Var *local_var = find_var(tkn);
                    if (local_var) {
                        errorf_at(ER_COMPILE, source_token, "This variable is redefinition.");
                    }
                    local_var = add_var(VR_INT, tkn->str, tkn->str_len);
                    if (target->func_args) {
                        target->func_args = new_node(ND_VAR, target->func_args, NULL);
                        target->func_args->var = local_var;
                    } else {
                        target->func_args = new_node(ND_VAR, NULL, NULL);
                        target->func_args->var = local_var;
                    }
                    target->func_argc++;
                    
                    if (use_symbol(",")) {
                        continue;
                    }
                    use_expect_symbol(")");
                    break;
                } else {
                    errorf_at(ER_COMPILE, source_token, "Declare variable.");
                }
            }
        }
        target->func_name = tkn->str;
        target->func_name_len = tkn->str_len;
        target->stmt = statement();
    } else {
        errorf_at(ER_COMPILE, source_token, "Start the function with an identifier.");
    }
}

Node* inside_roop;  // inside for or while

// statement = { statement* } |
//             ("return")? assign ";" |
//             "if" "(" assign ")" statement ("else" statement)? |
//             "for" "("assign?; assign?; assign?")" statement |
//             "while" "(" assign ")" statement |
//             "break;" |
//             "continue;" |
//             assign ";"
Node *statement() {
    Node *ret;

    // Block statement
    if (use_symbol("{")) {
        ret = new_node(ND_BLOCK, NULL, NULL);
        Node *now = NULL;
        while (!use_symbol("}")) {
            if (now) {
                now->next_stmt = statement();
                now = now->next_stmt;
            } else {
                now = statement();
                ret->next_stmt = now;
            }
        }
        return ret;
    }

    if (use_any_kind(TK_IF)) {
        ret = new_node(ND_IF, NULL, NULL);
        use_expect_symbol("(");
        ret->judge = assign();
        use_expect_symbol(")");
        ret->exec_if = statement();
        if (use_any_kind(TK_ELSE)) {
            ret->exec_else = statement();
        }
        return ret;
    }

    if (use_any_kind(TK_FOR)) {
        Node* roop_state = inside_roop;
        ret = new_node(ND_FOR, NULL, NULL);
        inside_roop = ret;

        use_expect_symbol("(");
        if (!use_symbol(";")) {
            ret->init_for = assign();
            use_expect_symbol(";");
        }

        if (!use_symbol(";")) {
            ret->judge = assign();
            use_expect_symbol(";");
        }

        if (!use_symbol(")")) {
            ret->repeat_for = assign();
            use_expect_symbol(")");
        }
        ret->stmt_for = statement();

        inside_roop = roop_state;
        return ret;
    }

    if (use_any_kind(TK_WHILE)) {
        Node* roop_state = inside_roop;
        ret = new_node(ND_WHILE, NULL, NULL);
        inside_roop = ret;

        use_expect_symbol("(");
        ret->judge = assign();
        use_expect_symbol(")");
        ret->stmt_for = statement();

        inside_roop = roop_state;
        return ret;
    }

    if (use_any_kind(TK_RETURN)) {
        ret = new_node(ND_RETURN, assign(), NULL);
        use_expect_symbol(";");
        return ret;
    }


    if (use_any_kind(TK_BREAK)) {
        if (!inside_roop) {
            errorf_at(ER_COMPILE, before_token, "%s", "break statement not whithin loop");
        }
        ret = new_node(ND_LOOPBREAK, NULL, NULL);
        ret->lhs = inside_roop;
        use_expect_symbol(";");
        return ret;
    }

    if (use_any_kind(TK_CONTINUE)) {
        if (!inside_roop) {
            errorf_at(ER_COMPILE, before_token, "%s", "continue statement not within loop");
        }
        ret = new_node(ND_CONTINUE, NULL, NULL);
        ret->lhs = inside_roop;
        use_expect_symbol(";");
        return ret;
    }

    ret = assign();
    use_expect_symbol(";");

    return ret;
}

// assign = same_comp | 
//          same_comp "=" same_comp
Node *assign() {
    Node *ret = same_comp();

    if (use_symbol("=")) {
        ret = new_node(ND_ASSIGN, ret, same_comp());
    }
    return ret;
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

// unary = ("+" | "-")? priority
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

// priority = num | 
//            "(" assign ")" |
//            ident "(" params? ")"
//            ident
// params = assign ("," assign)?
Node *priority() {
    if (use_symbol("(")) {
        Node *ret = assign();
        use_expect_symbol(")");
        return ret;
    }

    Token *tkn = use_any_kind(TK_IDENT);

    if (tkn) {
        // function call
        if (use_symbol("(")) {
            Node *ret = new_node(ND_FUNCCALL, NULL, NULL);
            ret->func_name = tkn->str;
            ret->func_name_len = tkn->str_len;

            if (use_symbol(")")) {
                return ret;
            }

            int argc = 1;
            Node *now_arg = ret;
            while (true) {
                now_arg->func_arg = assign();
                now_arg = now_arg->func_arg;
                now_arg->func_args_idx = argc++;
                
                if (use_symbol(",")) {
                    continue;
                }

                if (use_symbol(")")) {
                    break;
                }
            }
            return ret;
        }

        Node *ret = new_node(ND_VAR, NULL, NULL);
        Var *result = find_var(tkn);
        if (result) {
            ret->var = result;
        } else {
            ret->var = add_var(VR_INT, tkn->str, tkn->str_len);
        }
        return ret;
    }

    return num();
}


Node *num() {
    return new_node_int(use_expect_int());
}