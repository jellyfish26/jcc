#pragma once
#include "../parser/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

//
// codegen.c
//

void codegen();

//
// assembly.c
//

void gen_compare(char *comp);
void gen_var(Node *node);