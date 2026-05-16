#pragma once
#include "ast.h"
#include "lexer.h"

typedef struct {
    Lexer   *lexer;
    Token    cur;
    Token    peek;
    Arena   *arena;
    const char *filename;
    int      error_count;
} Parser;

void     parser_init(Parser *p, Lexer *l, Arena *a, const char *filename);
AstNode *parse_program(Parser *p);
