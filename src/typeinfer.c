#include "typeinfer.h"
#include "lexer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static TypeEnv *typeenv_get(TypeEnv *e, const char *name) {
    for (; e; e = e->next)
        if (strcmp(e->name, name) == 0) return e;
    return NULL;
}

static TypeEnv *typeenv_set(TypeEnv *e, const char *name, InferType t) {
    TypeEnv *entry = typeenv_get(e, name);
    if (entry) { entry->type = t; return e; }
    TypeEnv *n = malloc(sizeof(TypeEnv));
    n->name = strdup(name);
    n->type = t;
    n->next = e;
    return n;
}

static InferType ann_to_type(const char *ann) {
    if (!ann) return TY_UNKNOWN;
    if (strcmp(ann,"int")==0   || strcmp(ann,"整数")==0)   return TY_INT;
    if (strcmp(ann,"float")==0 || strcmp(ann,"浮点")==0)   return TY_FLOAT;
    if (strcmp(ann,"str")==0   || strcmp(ann,"字符串")==0) return TY_STR;
    if (strcmp(ann,"bool")==0  || strcmp(ann,"布尔")==0)   return TY_BOOL;
    if (strcmp(ann,"list")==0  || strcmp(ann,"列表")==0)   return TY_LIST;
    if (strcmp(ann,"map")==0   || strcmp(ann,"字典")==0)   return TY_MAP;
    if (strcmp(ann,"fn")==0    || strcmp(ann,"函数")==0)   return TY_FN;
    return TY_UNKNOWN;
}

static const char *type_name(InferType t) {
    switch (t) {
    case TY_INT:   return "int";
    case TY_FLOAT: return "float";
    case TY_STR:   return "str";
    case TY_BOOL:  return "bool";
    case TY_NIL:   return "nil";
    case TY_LIST:  return "list";
    case TY_MAP:   return "map";
    case TY_FN:    return "fn";
    default:       return "unknown";
    }
}

InferType infer_expr(AstNode *node, TypeEnv *env) {
    if (!node) return TY_UNKNOWN;
    switch (node->type) {
    case NODE_INT:    return TY_INT;
    case NODE_FLOAT:  return TY_FLOAT;
    case NODE_STRING:
    case NODE_FSTRING: return TY_STR;
    case NODE_BOOL:   return TY_BOOL;
    case NODE_NIL:    return TY_NIL;
    case NODE_IDENT: {
        TypeEnv *e = typeenv_get(env, node->sval.s);
        return e ? e->type : TY_UNKNOWN;
    }
    case NODE_BINOP: {
        InferType l = infer_expr(node->binop.left, env);
        InferType r = infer_expr(node->binop.right, env);
        PicoTokenType op = node->binop.op;
        if (op == TOK_EQ || op == TOK_NEQ || op == TOK_LT ||
            op == TOK_LE || op == TOK_GT  || op == TOK_GE) return TY_BOOL;
        if (op == TOK_AND || op == TOK_OR) return TY_BOOL;
        if (op == TOK_PLUS && l == TY_STR && r == TY_STR) return TY_STR;
        if (l == TY_FLOAT || r == TY_FLOAT) return TY_FLOAT;
        if (l == TY_INT   && r == TY_INT)   return TY_INT;
        return TY_UNKNOWN;
    }
    case NODE_UNOP:
        if (node->unop.op == TOK_NOT) return TY_BOOL;
        return infer_expr(node->unop.operand, env);
    case NODE_LIST:  return TY_LIST;
    case NODE_MAP:   return TY_MAP;
    case NODE_FN:
    case NODE_ASYNC_FN: return TY_FN;
    default: return TY_UNKNOWN;
    }
}

static TypeEnv *infer_stmt(AstNode *node, TypeEnv *env) {
    if (!node) return env;
    if (node->type == NODE_LET) {
        InferType t = infer_expr(node->let.value, env);
        if (node->let.type_ann) {
            InferType ann = ann_to_type(node->let.type_ann);
            if (ann != TY_UNKNOWN && t != TY_UNKNOWN && t != ann)
                fprintf(stderr, "warning: '%s' declared as %s but assigned %s\n",
                        node->let.name, type_name(ann), type_name(t));
            if (t == TY_UNKNOWN) t = ann;
        }
        env = typeenv_set(env, node->let.name, t);
    } else if (node->type == NODE_BLOCK || node->type == NODE_PROGRAM) {
        int count = node->type == NODE_BLOCK ? node->block.stmts.count : node->program.stmts.count;
        AstNode **items = node->type == NODE_BLOCK ? node->block.stmts.items : node->program.stmts.items;
        for (int i = 0; i < count; i++) env = infer_stmt(items[i], env);
    } else if (node->type == NODE_EXPR_STMT) {
        infer_expr(node->exprstmt.expr, env);
    }
    return env;
}

void infer_program(AstNode *prog) {
    TypeEnv *env = NULL;
    infer_stmt(prog, env);
}
