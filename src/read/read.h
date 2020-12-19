#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

//
// readsrc.c
//

typedef struct SourceLine SourceLine;

struct SourceLine {
    char *str;         // String 
    int len;           // Length of string
    int number;        // Number of lines
    SourceLine *next;  // Next line
};

extern SourceLine *source_code;
void readfile(char *file_path);

//
// tokenize.c
//

typedef enum {
    TK_NUM_INT,   // Number (Int)
    TK_OPERATOR,  // Operator
    TK_EOF,       // End of File
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind; // Type of Token
    Token *next;     // Next token
    char *str;       // Token String
    int str_len;     // Token length
    int val;         // Value if kind is TK_NUM_INT
};

extern Token *source_token;  // Warn: Don't operate it directly.
void *tokenize();

//
// operate.c
//

int use_expect_int();
bool use_operator(char *op);
void use_expect_operator(char *op);