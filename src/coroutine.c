#ifdef _WIN32
#define _WIN32_WINNT 0x0600
#include <windows.h>
#endif
#include "coroutine.h"
#include "interpreter.h"
#include <stdio.h>

#ifdef _WIN32
static __thread void* thread_fiber = NULL;
static __thread ObjCoro* active_coro = NULL;

void CALLBACK coro_entry(LPVOID arg) {
    ObjCoro *coro = (ObjCoro*)arg;
    Interpreter *vm = interp_get_current();
    coro->yielded = interp_exec(vm, coro->fn->body, coro->env);
    coro->is_done = true;
    SwitchToFiber(coro->caller);
}
#endif

Value coro_new(ObjFn *fn, Env *env) {
    ObjCoro *coro = gc_alloc(sizeof(ObjCoro));
    coro->hdr.type = OBJ_CORO;
    coro->hdr.marked = false;
    coro->hdr.next = gc_objects; gc_objects = (Obj*)coro;

    coro->fn = fn;
    coro->env = env; 
    coro->yielded = VAL_NIL_V;
    coro->is_done = false;
    coro->is_started = false;

#ifdef _WIN32
    coro->fiber = CreateFiber(0, coro_entry, coro);
#else
    coro->fiber = NULL;
#endif
    coro->caller = NULL;

    return (Value){VAL_COROUTINE, {.coro = coro}};
}

Value coro_resume(ObjCoro *coro, Value arg) {
    (void)arg;
#ifdef _WIN32
    if (coro->is_done) return VAL_NIL_V;
    if (!thread_fiber) thread_fiber = ConvertThreadToFiber(NULL);
    coro->caller = GetCurrentFiber();
    active_coro = coro;
    SwitchToFiber(coro->fiber);
    active_coro = NULL;
    return coro->yielded;
#else
    (void)coro;
    return VAL_NIL_V;
#endif
}

void coro_yield(Value val) {
#ifdef _WIN32
    if (!active_coro) return;
    active_coro->yielded = val;
    SwitchToFiber(active_coro->caller);
#else
    (void)val;
#endif
}
