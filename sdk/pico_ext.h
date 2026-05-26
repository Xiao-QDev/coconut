/*
 * Pico Extension SDK — write native modules in C
 *
 * Usage:
 *   #include "pico_ext.h"
 *
 *   static PicoVal my_add(int argc, PicoVal *argv) {
 *       return PICO_INT_V(argv[0].integer + argv[1].integer);
 *   }
 *
 *   PICO_MODULE(mymod) {
 *       PICO_EXPORT("add", my_add);
 *   }
 *
 * Build: gcc -shared -fPIC -o mymod.so mymod.c -Ipico/sdk
 * Load:  import mymod
 */
#pragma once
#include "pico.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PicoModuleCtx PicoModuleCtx;

/* register a native function into the module */
void pico_mod_export(PicoModuleCtx *ctx, const char *name, PicoNativeFn fn);

/* module entry point — implement this in your module */
typedef void (*PicoModuleInit)(PicoModuleCtx *ctx);

/* convenience macros */
#define PICO_MODULE(name) \
    void pico_module_init_##name(PicoModuleCtx *ctx); \
    void pico_module_init(PicoModuleCtx *ctx) { pico_module_init_##name(ctx); } \
    void pico_module_init_##name(PicoModuleCtx *ctx)

#define PICO_EXPORT(name, fn) pico_mod_export(ctx, name, fn)

/* argument helpers */
static inline bool pico_check_argc(int got, int want, PicoVal *out) {
    if (got < want) { *out = PICO_ERR_V("too few arguments"); return false; }
    return true;
}

#ifdef __cplusplus
}
#endif
