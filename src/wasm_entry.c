#define _POSIX_C_SOURCE 200809L
#include "interpreter.h"
#include "value.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

/* Global output buffer — value_print writes here when set */
static char  *wasm_buf     = NULL;
static int    wasm_buf_pos = 0;
static int    wasm_buf_cap = 0;

void pico_wasm_write(const char *s, int len) {
    if (!wasm_buf) return;
    if (wasm_buf_pos + len >= wasm_buf_cap - 1) return;
    memcpy(wasm_buf + wasm_buf_pos, s, len);
    wasm_buf_pos += len;
    wasm_buf[wasm_buf_pos] = '\0';
}

/* Override printf for WASM output capture */
int pico_printf(const char *fmt, ...) {
    if (!wasm_buf) { va_list ap; va_start(ap,fmt); int r=vprintf(fmt,ap); va_end(ap); return r; }
    char tmp[4096];
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    pico_wasm_write(tmp, n < (int)sizeof(tmp) ? n : (int)sizeof(tmp)-1);
    return n;
}

EMSCRIPTEN_KEEPALIVE
const char *pico_wasm_run(const char *src) {
    static char buf[65536];
    buf[0] = '\0';
    wasm_buf     = buf;
    wasm_buf_pos = 0;
    wasm_buf_cap = (int)sizeof(buf);
    interp_run_string(src, "<wasm>");
    wasm_buf = NULL;
    return buf;
}

#else
const char *pico_wasm_run(const char *src) { (void)src; return ""; }
#endif
