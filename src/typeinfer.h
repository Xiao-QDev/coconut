#pragma once
#include "ast.h"

typedef enum {
    TY_UNKNOWN, TY_INT, TY_FLOAT, TY_STR, TY_BOOL, TY_NIL, TY_LIST, TY_MAP, TY_FN
} InferType;

typedef struct TypeEnv {
    char           *name;
    InferType       type;
    struct TypeEnv *next;
} TypeEnv;

InferType infer_expr(AstNode *node, TypeEnv *env);
void      infer_program(AstNode *prog);
