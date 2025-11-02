#ifndef INTERPRETER_LEXER_H
#define INTERPRETER_LEXER_H

#include <stdio.h>

#include "common.h"
#include "s8.h"
#include "token.h"

typedef struct {
    FILE* file;
    FILE* prompt;

    s8 input;
    size_t input_capacity;

    size_t start;
    size_t current;

    size_t line;
    size_t column;
} Lexer;

void lexer_init(Lexer* lexer, FILE* file, FILE* prompt);
void lexer_init_s8(Lexer* lexer, s8 input);
void lexer_free(Lexer* lexer);

Token lexer_next_token(Lexer* lexer);

#endif
