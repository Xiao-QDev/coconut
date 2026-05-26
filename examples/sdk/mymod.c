/*
 * Extension module example: write a native Pico module in C
 * Build: gcc -std=c11 -shared -fPIC -Ipico/sdk -Ipico/src mymod.c -o mymod.so
 */
#include "pico_ext.h"
#include <math.h>

static PicoVal my_sqrt(int argc, PicoVal *argv) {
    if (argc < 1) return PICO_NIL_V;
    double x = argv[0].type == PICO_INT ? (double)argv[0].integer : argv[0].floating;
    return PICO_FLOAT_V(sqrt(x));
}

static PicoVal my_pow(int argc, PicoVal *argv) {
    if (argc < 2) return PICO_NIL_V;
    double a = argv[0].type == PICO_INT ? (double)argv[0].integer : argv[0].floating;
    double b = argv[1].type == PICO_INT ? (double)argv[1].integer : argv[1].floating;
    return PICO_FLOAT_V(pow(a, b));
}

PICO_MODULE(mymod) {
    PICO_EXPORT("sqrt", my_sqrt);
    PICO_EXPORT("pow",  my_pow);
}
