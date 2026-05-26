/*
 * Pico Embed SDK — run Pico scripts from C
 *
 * Usage:
 *   PicoVM *vm = pico_new();
 *   pico_run_string(vm, "print(\"hello\")");
 *   pico_free(vm);
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PicoVM PicoVM;

typedef enum {
    PICO_NIL, PICO_BOOL, PICO_INT, PICO_FLOAT, PICO_STRING,
    PICO_LIST, PICO_MAP, PICO_FN, PICO_ERROR
} PicoType;

typedef struct {
    PicoType type;
    union {
        bool     boolean;
        int64_t  integer;
        double   floating;
        char    *string;   /* owned by VM, valid until next call */
    };
} PicoVal;

/* lifecycle */
PicoVM *pico_new(void);
void    pico_free(PicoVM *vm);

/* execution */
PicoVal pico_run_string(PicoVM *vm, const char *src);
PicoVal pico_run_file(PicoVM *vm, const char *path);

/* globals */
void    pico_set_int(PicoVM *vm, const char *name, int64_t v);
void    pico_set_float(PicoVM *vm, const char *name, double v);
void    pico_set_string(PicoVM *vm, const char *name, const char *v);
void    pico_set_bool(PicoVM *vm, const char *name, bool v);
PicoVal pico_get(PicoVM *vm, const char *name);

/* native function registration */
typedef PicoVal (*PicoNativeFn)(int argc, PicoVal *argv);
void    pico_register(PicoVM *vm, const char *name, PicoNativeFn fn);

/* error */
bool        pico_has_error(PicoVM *vm);
const char *pico_error_msg(PicoVM *vm);

/* helpers */
static inline bool   pico_is_nil(PicoVal v)    { return v.type == PICO_NIL; }
static inline bool   pico_is_int(PicoVal v)    { return v.type == PICO_INT; }
static inline bool   pico_is_string(PicoVal v) { return v.type == PICO_STRING; }

#define PICO_NIL_V      ((PicoVal){.type=PICO_NIL})
#define PICO_INT_V(n)   ((PicoVal){.type=PICO_INT,    .integer=(n)})
#define PICO_FLOAT_V(f) ((PicoVal){.type=PICO_FLOAT,  .floating=(f)})
#define PICO_BOOL_V(b)  ((PicoVal){.type=PICO_BOOL,   .boolean=(b)})
#define PICO_STR_V(s)   ((PicoVal){.type=PICO_STRING, .string=(char*)(s)})
#define PICO_ERR_V(s)   ((PicoVal){.type=PICO_ERROR,  .string=(char*)(s)})

#ifdef __cplusplus
}
#endif
