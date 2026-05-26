/*
 * Embed example: run Pico from C
 * Build: gcc -std=c11 -Ipico/sdk -Ipico/src embed_example.c pico/sdk/pico.c pico/src/*.c pico/src/stdlib/*.c -lm -lpthread -o embed_example
 */
#include "pico.h"
#include <stdio.h>

static PicoVal c_add(int argc, PicoVal *argv) {
    if (argc < 2) return PICO_INT_V(0);
    return PICO_INT_V(argv[0].integer + argv[1].integer);
}

int main(void) {
    PicoVM *vm = pico_new();

    /* expose a C variable to Pico */
    pico_set_int(vm, "version", 1);
    pico_set_string(vm, "app_name", "MyApp");

    /* expose a C function to Pico */
    pico_register(vm, "c_add", c_add);

    /* run Pico code */
    pico_run_string(vm, "print(app_name, version)");
    pico_run_string(vm, "print(c_add(10, 32))");

    /* read a value back */
    pico_run_string(vm, "let result = c_add(100, 200)");
    PicoVal r = pico_get(vm, "result");
    printf("result from Pico: %lld\n", (long long)r.integer);

    if (pico_has_error(vm))
        fprintf(stderr, "error: %s\n", pico_error_msg(vm));

    pico_free(vm);
    return 0;
}
