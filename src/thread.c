#include "thread.h"
#include "interpreter.h"
#include <stdlib.h>
#include <stdio.h>

static void *thread_wrapper(void *arg) {
    ObjThread *t = (ObjThread*)arg;
    Interpreter vm;
    interp_init(&vm);
    
    // Create a new env for the thread
    Env *env = env_new(t->fn->closure);
    for (int i = 0; i < t->fn->arity && i < t->argc; i++) {
        ObjStr *pname = str_intern(t->fn->params[i], (int)strlen(t->fn->params[i]));
        env_set(env, pname, t->argv[i]);
    }

    t->result = interp_exec(&vm, t->fn->body, env);
    t->done = true;
    
    env_free(env);
    // Note: vm.globals is leaked here if we don't manage it, 
    // but in this MVP it's acceptable for now.
    return NULL;
}

Value thread_spawn(ObjFn *fn, int argc, Value *argv) {
    ObjThread *t = gc_alloc(sizeof(ObjThread));
    t->hdr.type = OBJ_THREAD;
    t->hdr.marked = false;
    t->hdr.next = gc_objects; gc_objects = (Obj*)t;
    
    t->fn = fn;
    t->argc = argc;
    t->argv = malloc(sizeof(Value) * argc);
    for (int i = 0; i < argc; i++) t->argv[i] = argv[i];
    t->done = false;
    t->result = VAL_NIL_V;

    if (pthread_create(&t->thread, NULL, thread_wrapper, t) != 0) {
        fprintf(stderr, "无法创建线程\n");
        return VAL_NIL_V;
    }
    
    return (Value){VAL_THREAD, {.thread = t}};
}

Value thread_join(ObjThread *thread) {
    if (!thread->done) {
        pthread_join(thread->thread, NULL);
    }
    return thread->result;
}
