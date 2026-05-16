#pragma once
#include "ast.h"
#include "value.h"

// ── 环境（变量作用域链）──────────────────────────────────────
#define ENV_SIZE 64
typedef struct Env {
    ObjStr     *keys[ENV_SIZE];
    Value       vals[ENV_SIZE];
    int         count;
    struct Env *parent;
} Env;

Env  *env_new(Env *parent);
void  env_set(Env *e, ObjStr *key, Value val);
bool  env_get(Env *e, ObjStr *key, Value *out);
bool  env_assign(Env *e, ObjStr *key, Value val);
void  env_free(Env *e);

// ── 解释器 ───────────────────────────────────────────────────
typedef struct {
    Env        *globals;
    const char *filename;
    // 控制流信号
    bool        returning;
    bool        breaking;
    bool        continuing;
    Value       return_val;
    // 错误
    bool        has_error;
    char        error_msg[256];
} Interpreter;

void  interp_init(Interpreter *vm);
void  interp_register_stdlib(Interpreter *vm);
Value interp_exec(Interpreter *vm, AstNode *node, Env *env);
Interpreter *interp_get_current();
Value interp_run_file(const char *filename);
Value interp_run_string(const char *src, const char *filename);

// GC roots（gc.c 调用）
void gc_mark_roots(void);
