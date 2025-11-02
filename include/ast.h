#ifndef INTERPRETER_AST_H
#define INTERPRETER_AST_H

#include "common.h"
#include "token.h"

typedef enum {
    AST_LITERAL,
} AstKind;

typedef struct AstNode AstNode;
typedef AstNode* AstNodeId;

typedef struct {
    void* ptr;
    size_t length;
    size_t capacity;
    size_t obj_size;
} AstComponent;

typedef struct {
    AstKind kind;
    size_t value;
} Base;

typedef struct {
    AstComponent base;
    AstComponent children;
    AstComponent token;
} Ast;

void ast_init(Ast* ast);
void ast_free(Ast* ast);

AstKind ast_node_kind(Ast* ast, AstNodeId id);

AstNodeId ast_literal_alloc(Ast* ast, Token token);
Token ast_literal_token(Ast* ast, AstNodeId id);

#endif
