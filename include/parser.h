#ifndef INTERPRETER_PARSER_H
#define INTERPRETER_PARSER_H

#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "s8.h"

typedef struct {
    Ast* ast;
    Lexer lexer;

    bool has_peek;
    Token peek;
} Parser;

void parser_init(Parser* parser, Ast* ast, FILE* file, FILE* prompt);
void parser_init_s8(Parser* parser, Ast* ast, s8 input);
void parser_free(Parser* parser);

AstNodeId parser_parse(Parser* parser);

#endif
