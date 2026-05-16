#include "thread.h"
#include "interpreter.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

Value mutex_new() {
    ObjMutex *m = gc_alloc(sizeof(ObjMutex));
    m->hdr.type = OBJ_MUTEX;
    m->hdr.marked = false;
    m->hdr.next = gc_objects; gc_objects = (Obj*)m;
    pthread_mutex_init(&m->mutex, NULL);
    return (Value){VAL_MUTEX, {.mutex = m}};
}

void mutex_lock(ObjMutex *m) { pthread_mutex_lock(&m->mutex); }
void mutex_unlock(ObjMutex *m) { pthread_mutex_unlock(&m->mutex); }

Value channel_new(int capacity) {
    ObjChannel *c = gc_alloc(sizeof(ObjChannel));
    c->hdr.type = OBJ_CHANNEL;
    c->hdr.marked = false;
    c->hdr.next = gc_objects; gc_objects = (Obj*)c;
    c->capacity = capacity > 0 ? capacity : 1;
    c->buffer = malloc(sizeof(Value) * c->capacity);
    c->head = c->tail = c->count = 0;
    pthread_mutex_init(&c->mutex, NULL);
    pthread_cond_init(&c->not_empty, NULL);
    pthread_cond_init(&c->not_full, NULL);
    return (Value){VAL_CHANNEL, {.channel = c}};
}

void channel_send(ObjChannel *c, Value val) {
    pthread_mutex_lock(&c->mutex);
    while (c->count == c->capacity) {
        pthread_cond_wait(&c->not_full, &c->mutex);
    }
    c->buffer[c->tail] = val;
    c->tail = (c->tail + 1) % c->capacity;
    c->count++;
    pthread_cond_signal(&c->not_empty);
    pthread_mutex_unlock(&c->mutex);
}

Value channel_recv(ObjChannel *c) {
    pthread_mutex_lock(&c->mutex);
    while (c->count == 0) {
        pthread_cond_wait(&c->not_empty, &c->mutex);
    }
    Value val = c->buffer[c->head];
    c->head = (c->head + 1) % c->capacity;
    c->count--;
    pthread_cond_signal(&c->not_full);
    pthread_mutex_unlock(&c->mutex);
    return val;
}
