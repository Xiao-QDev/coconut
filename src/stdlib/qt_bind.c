#include "qt_bind.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef QT_AVAILABLE
/* Qt C++ implementation — compiled separately as qt_impl.cpp */
extern void *qt_create_window(const char *title, int w, int h);
extern void *qt_create_button(const char *text);
extern void *qt_create_label(const char *text);
extern void *qt_create_input(const char *placeholder);
extern void  qt_widget_show_impl(void *w);
extern void  qt_window_add_impl(void *win, void *child);
extern void  qt_button_connect_impl(void *btn, void (*cb)(void*), void *userdata);
extern void  qt_app_run(void);
extern void  qt_app_init(void);
#endif

/* Widget handle: store pointer as VAL_INT */
#ifdef QT_AVAILABLE
static Value wrap_widget(void *ptr) {
    return VAL_INT_V((int64_t)(uintptr_t)ptr);
}
static void *unwrap_widget(Value v) {
    return (void*)(uintptr_t)v.integer;
}
#endif

Value qt_app_new(int argc, Value *argv) {
    (void)argc; (void)argv;
#ifdef QT_AVAILABLE
    qt_app_init();
    return VAL_BOOL_V(true);
#else
    fprintf(stderr, "[ui] Qt not available — stub mode\n");
    return VAL_NIL_V;
#endif
}

Value qt_window_new(int argc, Value *argv) {
#ifdef QT_AVAILABLE
    const char *title = (argc > 0 && IS_STR(argv[0])) ? argv[0].string->data : "Pico";
    int w = (argc > 1 && IS_INT(argv[1])) ? (int)argv[1].integer : 800;
    int h = (argc > 2 && IS_INT(argv[2])) ? (int)argv[2].integer : 600;
    return wrap_widget(qt_create_window(title, w, h));
#else
    (void)argc; (void)argv;
    return VAL_INT_V(1);  // stub handle
#endif
}

Value qt_button_new(int argc, Value *argv) {
#ifdef QT_AVAILABLE
    const char *text = (argc > 0 && IS_STR(argv[0])) ? argv[0].string->data : "Button";
    return wrap_widget(qt_create_button(text));
#else
    (void)argc; (void)argv;
    return VAL_INT_V(2);
#endif
}

Value qt_label_new(int argc, Value *argv) {
#ifdef QT_AVAILABLE
    const char *text = (argc > 0 && IS_STR(argv[0])) ? argv[0].string->data : "";
    return wrap_widget(qt_create_label(text));
#else
    (void)argc; (void)argv;
    return VAL_INT_V(3);
#endif
}

Value qt_input_new(int argc, Value *argv) {
#ifdef QT_AVAILABLE
    const char *ph = (argc > 0 && IS_STR(argv[0])) ? argv[0].string->data : "";
    return wrap_widget(qt_create_input(ph));
#else
    (void)argc; (void)argv;
    return VAL_INT_V(4);
#endif
}

Value qt_widget_show(int argc, Value *argv) {
#ifdef QT_AVAILABLE
    if (argc > 0 && IS_INT(argv[0])) qt_widget_show_impl(unwrap_widget(argv[0]));
#else
    (void)argc; (void)argv;
#endif
    return VAL_NIL_V;
}

Value qt_widget_set_title(int argc, Value *argv) {
    /* title set at creation time; no-op for now */
    (void)argc; (void)argv;
    return VAL_NIL_V;
}

Value qt_widget_add(int argc, Value *argv) {
#ifdef QT_AVAILABLE
    if (argc >= 2 && IS_INT(argv[0]) && IS_INT(argv[1]))
        qt_window_add_impl(unwrap_widget(argv[0]), unwrap_widget(argv[1]));
#else
    (void)argc; (void)argv;
#endif
    return VAL_NIL_V;
}

Value qt_widget_on_click(int argc, Value *argv) {
    /* Callback wiring requires Qt event loop integration — deferred */
    (void)argc; (void)argv;
    return VAL_NIL_V;
}

Value qt_app_exec(int argc, Value *argv) {
    (void)argc; (void)argv;
#ifdef QT_AVAILABLE
    qt_app_run();
#endif
    return VAL_NIL_V;
}
