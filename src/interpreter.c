#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "error.h"
#include "thread.h"

static Interpreter *current_vm = NULL;

void gc_mark_roots(void) {
    if (!current_vm) return;
    Env *e = current_vm->globals;
    while (e) {
        for (int i = 0; i < e->count; i++)
            gc_mark_value(e->vals[i]);
        e = e->parent;
    }
}

Env *env_new(Env *parent) {
    Env *e = malloc(sizeof(Env));
    e->count = 0;
    e->parent = parent;
    return e;
}

void env_set(Env *e, ObjStr *key, Value val) {
    for (int i = 0; i < e->count; i++) {
        if (e->keys[i] == key) { e->vals[i] = val; return; }
    }
    if (e->count < ENV_SIZE) {
        e->keys[e->count] = key;
        e->vals[e->count] = val;
        e->count++;
    }
}

bool env_get(Env *e, ObjStr *key, Value *out) {
    while (e) {
        for (int i = 0; i < e->count; i++) {
            if (e->keys[i] == key) { *out = e->vals[i]; return true; }
        }
        e = e->parent;
    }
    return false;
}

bool env_assign(Env *e, ObjStr *key, Value val) {
    while (e) {
        for (int i = 0; i < e->count; i++) {
            if (e->keys[i] == key) { e->vals[i] = val; return true; }
        }
        e = e->parent;
    }
    return false;
}

void env_free(Env *e) { free(e); }

static void set_error(Interpreter *vm, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(vm->error_msg, sizeof(vm->error_msg), fmt, ap);
    va_end(ap);
    vm->has_error = true;
}

static double to_float(Value v) {
    if (v.type == VAL_INT) return (double)v.integer;
    return v.floating;
}

Value interp_exec(Interpreter *vm, AstNode *node, Env *env) {
    if (!node || vm->has_error) return VAL_NIL_V;

    switch (node->type) {
    case NODE_INT:    return VAL_INT_V(node->ival);
    case NODE_FLOAT:  return VAL_FLOAT_V(node->fval);
    case NODE_BOOL:   return VAL_BOOL_V(node->bval);
    case NODE_NIL:    return VAL_NIL_V;
    case NODE_STRING: return VAL_STR_V(str_intern(node->sval.s, node->sval.len));

    case NODE_FSTRING: {
        char buf[4096]; buf[0] = 0;
        for (int i = 0; i < node->fstr.parts.count; i++) {
            AstNode *part = node->fstr.parts.items[i];
            Value v = interp_exec(vm, part, env);
            if (vm->has_error) return VAL_NIL_V;
            char tmp[512];
            if (IS_STR(v)) snprintf(tmp, sizeof(tmp), "%.*s", v.string->len, v.string->data);
            else if (IS_INT(v)) snprintf(tmp, sizeof(tmp), "%lld", (long long)v.integer);
            else if (IS_FLOAT(v)) snprintf(tmp, sizeof(tmp), "%g", v.floating);
            else if (IS_BOOL(v)) snprintf(tmp, sizeof(tmp), "%s", v.boolean ? "true" : "false");
            else snprintf(tmp, sizeof(tmp), "nil");
            strncat(buf, tmp, sizeof(buf) - strlen(buf) - 1);
        }
        return VAL_STR_V(str_intern(buf, (int)strlen(buf)));
    }

    case NODE_IDENT: {
        ObjStr *key = str_intern(node->sval.s, node->sval.len);
        Value v;
        if (env_get(env, key, &v)) return v;
        set_error(vm, "undefined variable '%s'", node->sval.s);
        return VAL_NIL_V;
    }

    case NODE_LET: {
        Value v = node->let.value ? interp_exec(vm, node->let.value, env) : VAL_NIL_V;
        if (vm->has_error) return VAL_NIL_V;
        ObjStr *key = str_intern(node->let.name, (int)strlen(node->let.name));
        env_set(env, key, v);
        return VAL_NIL_V;
    }

    case NODE_ASSIGN: {
        Value v = interp_exec(vm, node->assign.value, env);
        if (vm->has_error) return VAL_NIL_V;
        ObjStr *key = str_intern(node->assign.name, (int)strlen(node->assign.name));
        if (node->assign.op == TOK_PLUS_ASSIGN || node->assign.op == TOK_MINUS_ASSIGN) {
            Value cur;
            if (!env_get(env, key, &cur)) { set_error(vm, "undefined '%s'", node->assign.name); return VAL_NIL_V; }
            if (IS_INT(cur) && IS_INT(v))
                v = VAL_INT_V(node->assign.op == TOK_PLUS_ASSIGN ? cur.integer + v.integer : cur.integer - v.integer);
            else
                v = VAL_FLOAT_V(node->assign.op == TOK_PLUS_ASSIGN ? to_float(cur) + to_float(v) : to_float(cur) - to_float(v));
        }
        if (!env_assign(env, key, v)) env_set(env, key, v);
        return VAL_NIL_V;
    }

    case NODE_BINOP: {
        if (node->binop.op == TOK_AND) {
            Value l = interp_exec(vm, node->binop.left, env);
            if (vm->has_error) return VAL_NIL_V;
            if (!IS_TRUTHY(l)) return l;
            return interp_exec(vm, node->binop.right, env);
        }
        if (node->binop.op == TOK_OR) {
            Value l = interp_exec(vm, node->binop.left, env);
            if (vm->has_error) return VAL_NIL_V;
            if (IS_TRUTHY(l)) return l;
            return interp_exec(vm, node->binop.right, env);
        }
        Value l = interp_exec(vm, node->binop.left, env);
        if (vm->has_error) return VAL_NIL_V;
        Value r = interp_exec(vm, node->binop.right, env);
        if (vm->has_error) return VAL_NIL_V;
        TokenType op = node->binop.op;
        if (op == TOK_EQ)  return VAL_BOOL_V(value_equal(l, r));
        if (op == TOK_NEQ) return VAL_BOOL_V(!value_equal(l, r));
        if (op == TOK_DOTDOT) {
            if (!IS_INT(l) || !IS_INT(r)) { set_error(vm, ".. requires ints"); return VAL_NIL_V; }
            ObjList *res = list_new();
            for (int64_t i = l.integer; i <= r.integer; i++) list_push(res, VAL_INT_V(i));
            return VAL_LIST_V(res);
        }
        if (op == TOK_PLUS && IS_STR(l) && IS_STR(r))
            return VAL_STR_V(str_concat(l.string, r.string));
        if (IS_INT(l) && IS_INT(r)) {
            int64_t a = l.integer, b = r.integer;
            switch (op) {
            case TOK_PLUS:    return VAL_INT_V(a + b);
            case TOK_MINUS:   return VAL_INT_V(a - b);
            case TOK_STAR:    return VAL_INT_V(a * b);
            case TOK_SLASH:   if (!b) { set_error(vm, "division by zero"); return VAL_NIL_V; }
                              return VAL_INT_V(a / b);
            case TOK_PERCENT: if (!b) { set_error(vm, "division by zero"); return VAL_NIL_V; }
                              return VAL_INT_V(a % b);
            case TOK_LT:  return VAL_BOOL_V(a < b);
            case TOK_LE:  return VAL_BOOL_V(a <= b);
            case TOK_GT:  return VAL_BOOL_V(a > b);
            case TOK_GE:  return VAL_BOOL_V(a >= b);
            default: break;
            }
        }
        if (IS_NUM(l) && IS_NUM(r)) {
            double a = to_float(l), b = to_float(r);
            switch (op) {
            case TOK_PLUS:    return VAL_FLOAT_V(a + b);
            case TOK_MINUS:   return VAL_FLOAT_V(a - b);
            case TOK_STAR:    return VAL_FLOAT_V(a * b);
            case TOK_SLASH:   return VAL_FLOAT_V(a / b);
            case TOK_PERCENT: return VAL_FLOAT_V(fmod(a, b));
            case TOK_LT:  return VAL_BOOL_V(a < b);
            case TOK_LE:  return VAL_BOOL_V(a <= b);
            case TOK_GT:  return VAL_BOOL_V(a > b);
            case TOK_GE:  return VAL_BOOL_V(a >= b);
            default: break;
            }
        }
        set_error(vm, "invalid operands for binary op");
        return VAL_NIL_V;
    }

    case NODE_UNOP: {
        Value v = interp_exec(vm, node->unop.operand, env);
        if (vm->has_error) return VAL_NIL_V;
        if (node->unop.op == TOK_MINUS) {
            if (IS_INT(v))   return VAL_INT_V(-v.integer);
            if (IS_FLOAT(v)) return VAL_FLOAT_V(-v.floating);
        }
        if (node->unop.op == TOK_NOT) return VAL_BOOL_V(!IS_TRUTHY(v));
        set_error(vm, "invalid unary op");
        return VAL_NIL_V;
    }

    case NODE_IF: {
        Value cond = interp_exec(vm, node->ifnode.cond, env);
        if (vm->has_error) return VAL_NIL_V;
        if (IS_TRUTHY(cond)) return interp_exec(vm, node->ifnode.then, env);
        if (node->ifnode.els) return interp_exec(vm, node->ifnode.els, env);
        return VAL_NIL_V;
    }

    case NODE_WHILE: {
        while (1) {
            Value cond = interp_exec(vm, node->whilenode.cond, env);
            if (vm->has_error || !IS_TRUTHY(cond)) break;
            interp_exec(vm, node->whilenode.body, env);
            if (vm->has_error || vm->returning) break;
            if (vm->breaking) { vm->breaking = false; break; }
            if (vm->continuing) vm->continuing = false;
        }
        return VAL_NIL_V;
    }

    case NODE_FOR: {
        Value iter = interp_exec(vm, node->fornode.iter, env);
        if (vm->has_error) return VAL_NIL_V;
        ObjStr *var = str_intern(node->fornode.var, (int)strlen(node->fornode.var));
        Env *loop_env = env_new(env);
        if (IS_LIST(iter)) {
            for (int i = 0; i < iter.list->len; i++) {
                env_set(loop_env, var, iter.list->items[i]);
                interp_exec(vm, node->fornode.body, loop_env);
                if (vm->has_error || vm->returning) break;
                if (vm->breaking) { vm->breaking = false; break; }
                if (vm->continuing) vm->continuing = false;
            }
        } else {
            set_error(vm, "for: not iterable");
        }
        env_free(loop_env);
        return VAL_NIL_V;
    }

    case NODE_FN:
    case NODE_ASYNC_FN: {
        ObjFn *fn = gc_alloc(sizeof(ObjFn));
        fn->hdr.type = OBJ_FN;
        fn->hdr.marked = false;
        fn->hdr.next = gc_objects; gc_objects = (Obj*)fn;
        fn->name = node->fn.name ? str_intern(node->fn.name, (int)strlen(node->fn.name)) : NULL;
        fn->arity = node->fn.param_count;
        fn->body = node->fn.body;
        fn->closure = env;
        fn->is_generator = node->fn.is_generator;
        fn->is_async = (node->type == NODE_ASYNC_FN);
        fn->params = malloc(sizeof(char*) * fn->arity);
        for (int i = 0; i < fn->arity; i++) fn->params[i] = strdup(node->fn.params[i]);
        Value v = VAL_FN_V(fn);
        if (node->fn.name) {
            ObjStr *key = str_intern(node->fn.name, (int)strlen(node->fn.name));
            env_set(env, key, v);
        }
        return v;
    }

    case NODE_CALL: {
        Value callee = interp_exec(vm, node->call.callee, env);
        if (vm->has_error) return VAL_NIL_V;
        int argc = node->call.args.count;
        Value *argv = argc ? malloc(sizeof(Value) * argc) : NULL;
        for (int i = 0; i < argc; i++) {
            argv[i] = interp_exec(vm, node->call.args.items[i], env);
            if (vm->has_error) { free(argv); return VAL_NIL_V; }
        }
        Value result = VAL_NIL_V;
        if (callee.type == VAL_NATIVE) {
            result = callee.native(argc, argv);
        } else if (callee.type == VAL_FN) {
            ObjFn *fn = callee.fn;
            Env *call_env = env_new(fn->closure);
            for (int i = 0; i < fn->arity && i < argc; i++) {
                ObjStr *pname = str_intern(fn->params[i], (int)strlen(fn->params[i]));
                env_set(call_env, pname, argv[i]);
            }
            result = interp_exec(vm, fn->body, call_env);
            if (vm->returning) {
                result = vm->return_val;
                vm->returning = false;
            }
            env_free(call_env);
        } else {
            set_error(vm, "not callable");
        }
        free(argv);
        return result;
    }

    case NODE_RETURN: {
        Value v = node->retnode.value ? interp_exec(vm, node->retnode.value, env) : VAL_NIL_V;
        vm->returning = true;
        vm->return_val = v;
        return v;
    }

    case NODE_BLOCK: {
        Env *new_env = env_new(env);
        Value last = VAL_NIL_V;
        for (int i = 0; i < node->block.stmts.count; i++) {
            last = interp_exec(vm, node->block.stmts.items[i], new_env);
            if (vm->has_error || vm->returning || vm->breaking || vm->continuing) break;
        }
        env_free(new_env);
        return last;
    }

    case NODE_PROGRAM: {
        Value last = VAL_NIL_V;
        for (int i = 0; i < node->program.stmts.count; i++) {
            last = interp_exec(vm, node->program.stmts.items[i], env);
            if (vm->has_error) break;
        }
        return last;
    }

    case NODE_EXPR_STMT:
        return interp_exec(vm, node->exprstmt.expr, env);

    case NODE_MATCH: {
        Value subject = interp_exec(vm, node->matchnode.subject, env);
        if (vm->has_error) return VAL_NIL_V;
        bool matched = false;
        for (int i = 0; i < node->matchnode.patterns.count; i++) {
            Value pat = interp_exec(vm, node->matchnode.patterns.items[i], env);
            if (value_equal(subject, pat)) {
                interp_exec(vm, node->matchnode.bodies.items[i], env);
                matched = true; break;
            }
        }
        if (!matched && node->matchnode.default_body) {
            interp_exec(vm, node->matchnode.default_body, env);
        }
        return VAL_NIL_V;
    }

    case NODE_LIST: {
        ObjList *l = list_new();
        for (int i = 0; i < node->list.elements.count; i++) {
            list_push(l, interp_exec(vm, node->list.elements.items[i], env));
            if (vm->has_error) return VAL_NIL_V;
        }
        return VAL_LIST_V(l);
    }

    case NODE_MAP: {
        ObjMap *m = map_new();
        for (int i = 0; i < node->map.keys.count; i++) {
            Value k = interp_exec(vm, node->map.keys.items[i], env);
            if (!IS_STR(k)) { set_error(vm, "map key must be string"); return VAL_NIL_V; }
            Value v = interp_exec(vm, node->map.vals.items[i], env);
            map_set(m, k.string, v);
            if (vm->has_error) return VAL_NIL_V;
        }
        return VAL_MAP_V(m);
    }

    case NODE_INDEX: {
        Value obj = interp_exec(vm, node->index.obj, env);
        Value idx = interp_exec(vm, node->index.idx, env);
        if (IS_LIST(obj)) {
            if (!IS_INT(idx)) { set_error(vm, "index must be int"); return VAL_NIL_V; }
            return list_get(obj.list, (int)idx.integer);
        }
        if (IS_MAP(obj)) {
            if (!IS_STR(idx)) { set_error(vm, "map index must be string"); return VAL_NIL_V; }
            Value res;
            if (map_get(obj.map, idx.string, &res)) return res;
            return VAL_NIL_V;
        }
        set_error(vm, "not indexable");
        return VAL_NIL_V;
    }

    case NODE_TRY: {
        Env *try_env = env_new(env);
        Value res = interp_exec(vm, node->trynode.body, try_env);
        if (vm->has_error) {
            vm->has_error = false;
            if (node->trynode.catch_body) {
                Env *catch_env = env_new(env);
                if (node->trynode.catch_var) {
                    ObjStr *msg = str_intern(vm->error_msg, (int)strlen(vm->error_msg));
                    env_set(catch_env, str_intern(node->trynode.catch_var->sval.s, (int)strlen(node->trynode.catch_var->sval.s)), VAL_STR_V(msg));
                }
                res = interp_exec(vm, node->trynode.catch_body, catch_env);
                env_free(catch_env);
            }
        }
        env_free(try_env);
        return res;
    }

    case NODE_BREAK:    vm->breaking = true; return VAL_NIL_V;
    case NODE_CONTINUE: vm->continuing = true; return VAL_NIL_V;

    case NODE_SPAWN: {
        Value v = interp_exec(vm, node->spawnnode.expr, env);
        if (v.type != VAL_FN) { set_error(vm, "spawn requires a function"); return VAL_NIL_V; }
        return thread_spawn(v.fn, 0, NULL);
    }

    case NODE_METHOD_CALL: {
        Value obj = interp_exec(vm, node->mcall.obj, env);
        if (vm->has_error) return VAL_NIL_V;
        if (IS_THREAD(obj)) {
            if (strcmp(node->mcall.method, "wait") == 0 || strcmp(node->mcall.method, "等待") == 0) {
                return thread_join(obj.thread);
            }
        }
        set_error(vm, "unknown method '%s'", node->mcall.method);
        return VAL_NIL_V;
    }

    default:
        return VAL_NIL_V;
    }
}

static Value native_print(int argc, Value *argv) {
    for (int i = 0; i < argc; i++) {
        if (i > 0) printf(" ");
        value_print(argv[i]);
    }
    printf("\n");
    return VAL_NIL_V;
}

void interp_init(Interpreter *vm) {
    memset(vm, 0, sizeof(Interpreter));
    vm->globals = env_new(NULL);
    current_vm = vm;
    interp_register_stdlib(vm);
}

void interp_register_stdlib(Interpreter *vm) {
    env_set(vm->globals, str_intern("print", 5), VAL_NATIVE_V(native_print));
    env_set(vm->globals, str_intern("打印", 6), VAL_NATIVE_V(native_print));
}

Value interp_run_string(const char *src, const char *filename) {
    Interpreter vm;
    interp_init(&vm);
    vm.filename = filename;

    Lexer l;
    lexer_init(&l, src);
    Arena a;
    arena_init(&a);
    Parser p;
    parser_init(&p, &l, &a, filename);

    AstNode *prog = parse_program(&p);
    Value res = VAL_NIL_V;
    if (p.error_count == 0) {
        res = interp_exec(&vm, prog, vm.globals);
        if (vm.has_error) {
            fprintf(stderr, "运行时错误: %s\n", vm.error_msg);
        }
    }

    arena_free(&a);
    // env_free(vm.globals); // globals might contain functions pointing to AST... 
    // Wait, if we free Arena, the AST is gone. So we can't really run functions after this.
    // For a simple script, it's fine.
    return res;
}

Value interp_run_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) { fprintf(stderr, "无法打开文件: %s\n", filename); return VAL_NIL_V; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *buf = malloc(size + 1);
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    Value res = interp_run_string(buf, filename);
    free(buf);
    return res;
}
