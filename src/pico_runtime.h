/* pico_runtime.h — embedded runtime for compiled Pico programs */
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

typedef enum {
    PICO_NIL, PICO_BOOL, PICO_INT, PICO_FLOAT, PICO_STR, PICO_LIST
} PicoType;

typedef struct PicoVal PicoVal;
typedef struct PicoList { PicoVal *items; int len, cap; } PicoList;

struct PicoVal {
    PicoType type;
    union {
        bool     b;
        int64_t  i;
        double   f;
        char    *s;
        PicoList *l;
    };
};

static PicoVal PICO_NIL_V  = {PICO_NIL};
static inline PicoVal pico_int(int64_t n)  { return (PicoVal){PICO_INT,   {.i=n}}; }
static inline PicoVal pico_float(double d) { return (PicoVal){PICO_FLOAT, {.f=d}}; }
static inline PicoVal pico_bool(bool b)    { return (PicoVal){PICO_BOOL,  {.b=b}}; }
static inline PicoVal pico_str(const char *s) {
    PicoVal v = {PICO_STR}; v.s = strdup(s); return v;
}
static inline bool pico_is_truthy(PicoVal v) {
    if (v.type==PICO_NIL) return false;
    if (v.type==PICO_BOOL) return v.b;
    return true;
}
static inline PicoVal pico_add(PicoVal a, PicoVal b) {
    if (a.type==PICO_STR && b.type==PICO_STR) {
        char *r = malloc(strlen(a.s)+strlen(b.s)+1);
        strcpy(r,a.s); strcat(r,b.s);
        PicoVal v={PICO_STR}; v.s=r; return v;
    }
    double x = a.type==PICO_INT?a.i:a.f, y = b.type==PICO_INT?b.i:b.f;
    if (a.type==PICO_INT && b.type==PICO_INT) return pico_int(a.i+b.i);
    return pico_float(x+y);
}
static inline PicoVal pico_sub(PicoVal a, PicoVal b) {
    if (a.type==PICO_INT&&b.type==PICO_INT) return pico_int(a.i-b.i);
    return pico_float((a.type==PICO_INT?a.i:a.f)-(b.type==PICO_INT?b.i:b.f));
}
static inline PicoVal pico_mul(PicoVal a, PicoVal b) {
    if (a.type==PICO_INT&&b.type==PICO_INT) return pico_int(a.i*b.i);
    return pico_float((a.type==PICO_INT?a.i:a.f)*(b.type==PICO_INT?b.i:b.f));
}
static inline PicoVal pico_div(PicoVal a, PicoVal b) {
    return pico_float((a.type==PICO_INT?a.i:a.f)/(b.type==PICO_INT?b.i:b.f));
}
static inline PicoVal pico_mod(PicoVal a, PicoVal b) {
    if (a.type==PICO_INT&&b.type==PICO_INT) return pico_int(a.i%b.i);
    return pico_float(fmod(a.type==PICO_INT?a.i:a.f, b.type==PICO_INT?b.i:b.f));
}
static inline PicoVal pico_eq(PicoVal a, PicoVal b) {
    if (a.type!=b.type) return pico_bool(false);
    if (a.type==PICO_INT) return pico_bool(a.i==b.i);
    if (a.type==PICO_FLOAT) return pico_bool(a.f==b.f);
    if (a.type==PICO_STR) return pico_bool(strcmp(a.s,b.s)==0);
    if (a.type==PICO_BOOL) return pico_bool(a.b==b.b);
    return pico_bool(false);
}
static inline void pico_print(PicoVal v) {
    switch(v.type) {
    case PICO_NIL:   printf("nil"); break;
    case PICO_BOOL:  printf("%s", v.b?"true":"false"); break;
    case PICO_INT:   printf("%lld", (long long)v.i); break;
    case PICO_FLOAT: printf("%g", v.f); break;
    case PICO_STR:   printf("%s", v.s); break;
    default: printf("<value>"); break;
    }
    printf("\n");
}
/* Builtin: print */
static inline PicoVal _pico_print(PicoVal v) { pico_print(v); return PICO_NIL_V; }
