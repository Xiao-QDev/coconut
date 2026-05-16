#define _WIN32_WINNT 0x0600
#include "coroutine.h"
#include "interpreter.h"
#include <windows.h>
#include <stdio.h>

static __thread void* thread_fiber = NULL;
static __thread ObjCoro* active_coro = NULL;

void CALLBACK coro_entry(LPVOID arg) {
    ObjCoro *coro = (ObjCoro*)arg;
    
    // 获取当前的 VM
    Interpreter *vm = interp_get_current();
    
    // 执行函数体
    coro->yielded = interp_exec(vm, coro->fn->body, coro->env);
    
    coro->is_done = true;
    SwitchToFiber(coro->caller);
}

Value coro_new(ObjFn *fn, Env *env) {
    ObjCoro *coro = gc_alloc(sizeof(ObjCoro));
    coro->hdr.type = OBJ_CORO;
    coro->hdr.marked = false;
    coro->hdr.next = gc_objects; gc_objects = (Obj*)coro;

    coro->fn = fn;
    coro->env = env_new(env); // 为协程创建独立环境
    coro->fiber = CreateFiber(0, coro_entry, coro);
    coro->caller = NULL;
    coro->yielded = VAL_NIL_V;
    coro->is_done = false;
    coro->is_started = false;

    return (Value){VAL_COROUTINE, {.coro = coro}};
}

Value coro_resume(ObjCoro *coro, Value arg) {
    if (coro->is_done) return VAL_NIL_V;

    if (!thread_fiber) {
        thread_fiber = ConvertThreadToFiber(NULL);
    }

    coro->caller = GetCurrentFiber();
    active_coro = coro;
    
    SwitchToFiber(coro->fiber);
    
    active_coro = NULL;
    return coro->yielded;
}

void coro_yield(Value val) {
    if (!active_coro) return;
    
    active_coro->yielded = val;
    SwitchToFiber(active_coro->caller);
}
