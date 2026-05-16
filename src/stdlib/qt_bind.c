#include "qt_bind.h"
#include <stdio.h>

Value qt_window_new(int argc, Value *argv) {
    (void)argc; (void)argv;
    fprintf(stderr, "Qt not available on this platform\n");
    return VAL_NIL_V;
}

Value qt_button_new(int argc, Value *argv) {
    (void)argc; (void)argv;
    return VAL_NIL_V;
}

Value qt_app_exec(int argc, Value *argv) {
    (void)argc; (void)argv;
    return VAL_NIL_V;
}
