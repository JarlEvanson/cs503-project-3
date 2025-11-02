#include <stdlib.h>

#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "util.h"

void parser_init(Parser* parser, Ast* ast, FILE* file, FILE* prompt) {
    lexer_init(&parser->lexer, file, prompt);

    parser->ast = ast;
    parser->has_peek = false;
}

void parser_init_s8(Parser* parser, Ast* ast, s8 input) {
    lexer_init_s8(&parser->lexer, input);

    parser->ast = ast;
    parser->has_peek = false;
}

void parser_free(Parser* parser) {
    lexer_free(&parser->lexer);

    parser->has_peek = false;
}

static Token peek(Parser* parser) {
    if (parser->has_peek) return parser->peek;
    parser->has_peek = true;
    return parser->peek = lexer_next_token(&parser->lexer);
}

static Token advance(Parser* parser) {
    if (parser->has_peek) {
        parser->has_peek = false;
        return parser->peek;
    }

    return lexer_next_token(&parser->lexer);
}

static bool is_at_end(Parser* parser) {
    return peek(parser).kind == TOKEN_EOF;
}

static bool check(Parser* parser, TokenKind kind) {
    return !is_at_end(parser) && peek(parser).kind == kind;
}

static Token consume(Parser* parser, TokenKind kind) {
    if (peek(parser).kind == kind) {
        return advance(parser);
    }

    PANIC("implement consume errors");
}

static AstNodeId primary(Parser* parser) {
    
}

AstNodeId parser_parse(Parser* parser) {
    
}
