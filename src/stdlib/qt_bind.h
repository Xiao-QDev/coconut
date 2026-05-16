#pragma once
#include "../value.h"

Value qt_window_new(int argc, Value *argv);
Value qt_button_new(int argc, Value *argv);
Value qt_label_new(int argc, Value *argv);
Value qt_input_new(int argc, Value *argv);
Value qt_app_exec(int argc, Value *argv);
Value qt_app_new(int argc, Value *argv);
Value qt_widget_show(int argc, Value *argv);
Value qt_widget_set_title(int argc, Value *argv);
Value qt_widget_add(int argc, Value *argv);
Value qt_widget_on_click(int argc, Value *argv);
