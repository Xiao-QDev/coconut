#include "pico.h"
#include "../src/interpreter.h"
#include "../src/lexer.h"
#include "../src/parser.h"
#include "../src/gc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct PicoVM {
    Interpreter interp;
    Arena       arena;
};

static PicoVal to_picoval(Value v) {
    switch (v.type) {
    case VAL_NIL:    return PICO_NIL_V;
    case VAL_BOOL:   return PICO_BOOL_V(v.boolean);
    case VAL_INT:    return PICO_INT_V(v.integer);
    case VAL_FLOAT:  return PICO_FLOAT_V(v.floating);
    case VAL_STRING: return PICO_STR_V(v.string->data);
    default:         return PICO_NIL_V;
    }
}

static Value from_picoval(PicoVal v) {
    switch (v.type) {
    case PICO_NIL:    return VAL_NIL_V;
    case PICO_BOOL:   return VAL_BOOL_V(v.boolean);
    case PICO_INT:    return VAL_INT_V(v.integer);
    case PICO_FLOAT:  return VAL_FLOAT_V(v.floating);
    case PICO_STRING: return VAL_STR_V(str_intern(v.string, (int)strlen(v.string)));
    default:          return VAL_NIL_V;
    }
}

PicoVM *pico_new(void) {
    PicoVM *vm = calloc(1, sizeof(PicoVM));
    arena_init(&vm->arena);
    interp_init(&vm->interp);
    return vm;
}

void pico_free(PicoVM *vm) {
    arena_free(&vm->arena);
    free(vm);
}

static PicoVal run(PicoVM *vm, const char *src, const char *filename) {
    Lexer l; lexer_init(&l, src);
    Parser p; parser_init(&p, &l, &vm->arena, filename);
    AstNode *prog = parse_program(&p);
    if (p.error_count) { vm->interp.has_error = true; return PICO_ERR_V("parse error"); }
    Value res = interp_exec(&vm->interp, prog, vm->interp.globals);
    if (vm->interp.returning) { res = vm->interp.return_val; vm->interp.returning = false; }
    return to_picoval(res);
}

PicoVal pico_run_string(PicoVM *vm, const char *src)  { return run(vm, src, "<string>"); }
PicoVal pico_run_file(PicoVM *vm, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { vm->interp.has_error = true; return PICO_ERR_V("file not found"); }
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *buf = malloc(sz + 1);
    (void)fread(buf, 1, sz, f); buf[sz] = 0; fclose(f);
    PicoVal r = run(vm, buf, path);
    free(buf); return r;
}

void pico_set_int(PicoVM *vm, const char *n, int64_t v) {
    env_set(vm->interp.globals, str_intern(n,(int)strlen(n)), VAL_INT_V(v));
}
void pico_set_float(PicoVM *vm, const char *n, double v) {
    env_set(vm->interp.globals, str_intern(n,(int)strlen(n)), VAL_FLOAT_V(v));
}
void pico_set_bool(PicoVM *vm, const char *n, bool v) {
    env_set(vm->interp.globals, str_intern(n,(int)strlen(n)), VAL_BOOL_V(v));
}
void pico_set_string(PicoVM *vm, const char *n, const char *v) {
    env_set(vm->interp.globals, str_intern(n,(int)strlen(n)),
            VAL_STR_V(str_intern(v,(int)strlen(v))));
}

PicoVal pico_get(PicoVM *vm, const char *name) {
    Value v;
    if (env_get(vm->interp.globals, str_intern(name,(int)strlen(name)), &v))
        return to_picoval(v);
    return PICO_NIL_V;
}

/* --- native function registration via per-slot trampolines --- */
#define MAX_NATIVE 64
static PicoNativeFn native_table[MAX_NATIVE];
static int          native_count = 0;

/* Generate 64 trampoline functions that each call native_table[N] */
#define TRAMPOLINE(N) \
    static Value _tramp_##N(int argc, Value *argv) { \
        PicoVal pargs[32]; int n = argc < 32 ? argc : 32; \
        for (int i = 0; i < n; i++) pargs[i] = to_picoval(argv[i]); \
        return from_picoval(native_table[N](n, pargs)); \
    }

TRAMPOLINE(0)  TRAMPOLINE(1)  TRAMPOLINE(2)  TRAMPOLINE(3)
TRAMPOLINE(4)  TRAMPOLINE(5)  TRAMPOLINE(6)  TRAMPOLINE(7)
TRAMPOLINE(8)  TRAMPOLINE(9)  TRAMPOLINE(10) TRAMPOLINE(11)
TRAMPOLINE(12) TRAMPOLINE(13) TRAMPOLINE(14) TRAMPOLINE(15)
TRAMPOLINE(16) TRAMPOLINE(17) TRAMPOLINE(18) TRAMPOLINE(19)
TRAMPOLINE(20) TRAMPOLINE(21) TRAMPOLINE(22) TRAMPOLINE(23)
TRAMPOLINE(24) TRAMPOLINE(25) TRAMPOLINE(26) TRAMPOLINE(27)
TRAMPOLINE(28) TRAMPOLINE(29) TRAMPOLINE(30) TRAMPOLINE(31)
TRAMPOLINE(32) TRAMPOLINE(33) TRAMPOLINE(34) TRAMPOLINE(35)
TRAMPOLINE(36) TRAMPOLINE(37) TRAMPOLINE(38) TRAMPOLINE(39)
TRAMPOLINE(40) TRAMPOLINE(41) TRAMPOLINE(42) TRAMPOLINE(43)
TRAMPOLINE(44) TRAMPOLINE(45) TRAMPOLINE(46) TRAMPOLINE(47)
TRAMPOLINE(48) TRAMPOLINE(49) TRAMPOLINE(50) TRAMPOLINE(51)
TRAMPOLINE(52) TRAMPOLINE(53) TRAMPOLINE(54) TRAMPOLINE(55)
TRAMPOLINE(56) TRAMPOLINE(57) TRAMPOLINE(58) TRAMPOLINE(59)
TRAMPOLINE(60) TRAMPOLINE(61) TRAMPOLINE(62) TRAMPOLINE(63)

static NativeFn tramp_ptrs[MAX_NATIVE] = {
    _tramp_0,  _tramp_1,  _tramp_2,  _tramp_3,
    _tramp_4,  _tramp_5,  _tramp_6,  _tramp_7,
    _tramp_8,  _tramp_9,  _tramp_10, _tramp_11,
    _tramp_12, _tramp_13, _tramp_14, _tramp_15,
    _tramp_16, _tramp_17, _tramp_18, _tramp_19,
    _tramp_20, _tramp_21, _tramp_22, _tramp_23,
    _tramp_24, _tramp_25, _tramp_26, _tramp_27,
    _tramp_28, _tramp_29, _tramp_30, _tramp_31,
    _tramp_32, _tramp_33, _tramp_34, _tramp_35,
    _tramp_36, _tramp_37, _tramp_38, _tramp_39,
    _tramp_40, _tramp_41, _tramp_42, _tramp_43,
    _tramp_44, _tramp_45, _tramp_46, _tramp_47,
    _tramp_48, _tramp_49, _tramp_50, _tramp_51,
    _tramp_52, _tramp_53, _tramp_54, _tramp_55,
    _tramp_56, _tramp_57, _tramp_58, _tramp_59,
    _tramp_60, _tramp_61, _tramp_62, _tramp_63,
};

void pico_register(PicoVM *vm, const char *name, PicoNativeFn fn) {
    if (native_count >= MAX_NATIVE) return;
    int idx = native_count++;
    native_table[idx] = fn;
    env_set(vm->interp.globals, str_intern(name,(int)strlen(name)),
            VAL_NATIVE_V(tramp_ptrs[idx]));
}

bool        pico_has_error(PicoVM *vm)  { return vm->interp.has_error; }
const char *pico_error_msg(PicoVM *vm)  { return vm->interp.error_msg; }

/* --- extension module context --- */
struct PicoModuleCtx { PicoVM *vm; };

void pico_mod_export(PicoModuleCtx *ctx, const char *name, PicoNativeFn fn) {
    pico_register(ctx->vm, name, fn);
}
