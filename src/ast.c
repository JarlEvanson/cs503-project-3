#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "common.h"
#include "util.h"

static void component_init(
    AstComponent* component,
    size_t obj_size
) {
    component->ptr = NULL;
    component->length = 0;
    component->capacity = 0;

    component->obj_size = obj_size;
}

static void component_free(AstComponent* component) {
    free(component->ptr);
    component->ptr = NULL;
    component->length = 0;
    component->capacity = 0;

    component->obj_size = 0;
}

static size_t component_alloc(AstComponent* component) {
    if (component->length >= component->capacity) {
        bool result = GROW(
            &component->ptr,
            &component->capacity,
            component->obj_size,
            8
        );
        if (!result) {
            fprintf(stderr, "failed to grow ast component\n");
            exit(EXIT_FAILURE);
        }
    }
    
    size_t id = component->length;
    component->length += 1;
    return id;
}

static void* component_index(AstComponent* component, size_t index) {
    ASSERT(index < component->length, "`index` must be smaller than length");
    return ((uint8_t*) component->ptr) + index * component->obj_size;
}

void ast_init(Ast* ast) {
    component_init(&ast->base, sizeof(Base));
    component_init(&ast->children, sizeof(AstNodeId));
    component_init(&ast->token, sizeof(Token));
}

void ast_free(Ast* ast) {
    component_free(&ast->base);
    component_free(&ast->children);
    component_free(&ast->token);
}

AstKind ast_node_kind(Ast* ast, AstNodeId id) {
    return ((Base*) component_index(&ast->base, (size_t) id))->kind;
}

AstNodeId ast_literal_alloc(Ast* ast, Token token) {
    size_t token_id = component_alloc(&ast->token);
    *((Token*) component_index(&ast->token, token_id)) = token;

    size_t id = component_alloc(&ast->base);
    Base* base = (Base*) component_index(&ast->base, id);
    base->kind = AST_LITERAL;
    base->value = token_id;
    return (AstNodeId) id;
}

Token ast_literal_token(Ast* ast, AstNodeId id) {
    Base* base = (Base*) component_index(&ast->base, (size_t) id);
    return *((Token*) component_index(&ast->token, base->value));
}
