#define _POSIX_C_SOURCE 200809L
#include "interpreter.h"
#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
EMSCRIPTEN_KEEPALIVE
const char *pico_wasm_run(const char *src) {
    static char buf[65536];
    buf[0] = '\0';
    FILE *mem = fmemopen(buf, sizeof(buf)-1, "w");
    if (!mem) return "error";
    FILE *old = stdout; stdout = mem;
    interp_run_string(src, "<wasm>");
    fflush(mem); fclose(mem);
    stdout = old;
    return buf;
}
#else
/* stub for non-WASM builds */
const char *pico_wasm_run(const char *src) { (void)src; return ""; }
#endif
