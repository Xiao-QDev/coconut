#pragma once
#include "value.h"

void gc_collect(void);
void gc_mark_roots(void);  // 由 interpreter.c 实现
