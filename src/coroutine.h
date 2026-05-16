#pragma once
#include "value.h"

struct ObjCoro {
    Obj       hdr;
    void     *fiber;       // LPVOID fiber
    void     *caller;      // LPVOID caller
    ObjFn    *fn;
    Env      *env;
    Value     yielded;
    bool      is_done;
    bool      is_started;
};

Value coro_new(ObjFn *fn, Env *env);
Value coro_resume(ObjCoro *coro, Value arg);
void  coro_yield(Value val);
