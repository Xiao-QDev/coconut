#include "thread.h"
#include "interpreter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    ObjFn    *fn;
    int       argc;
    Value    *argv;
    ObjThread *handle;
} Task;

static Task           queue[PICO_TASK_QUEUE];
static int            q_head, q_tail, q_count;
static pthread_mutex_t q_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  q_cv = PTHREAD_COND_INITIALIZER;
static pthread_t      workers[PICO_WORKERS];
static bool           sched_running = false;

static void *worker(void *arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&q_mu);
        while (q_count == 0 && sched_running)
            pthread_cond_wait(&q_cv, &q_mu);
        if (!sched_running && q_count == 0) { pthread_mutex_unlock(&q_mu); break; }
        Task t = queue[q_head];
        q_head = (q_head + 1) % PICO_TASK_QUEUE;
        q_count--;
        pthread_mutex_unlock(&q_mu);

        Interpreter vm;
        interp_init(&vm);
        Env *env = env_new(t.fn->closure);
        for (int i = 0; i < t.fn->arity && i < t.argc; i++) {
            ObjStr *p = str_intern(t.fn->params[i], (int)strlen(t.fn->params[i]));
            env_set(env, p, t.argv[i]);
        }
        Value result = interp_exec(&vm, t.fn->body, env);
        if (vm.returning) result = vm.return_val;
        env_free(env);
        free(t.argv);

        pthread_mutex_lock(&t.handle->mu);
        t.handle->result = result;
        t.handle->done   = true;
        pthread_cond_broadcast(&t.handle->cv);
        pthread_mutex_unlock(&t.handle->mu);
    }
    return NULL;
}

void scheduler_init(void) {
    if (sched_running) return;
    sched_running = true;
    q_head = q_tail = q_count = 0;
    for (int i = 0; i < PICO_WORKERS; i++)
        pthread_create(&workers[i], NULL, worker, NULL);
}

Value thread_spawn(ObjFn *fn, int argc, Value *argv) {
    scheduler_init();

    ObjThread *h = gc_alloc(sizeof(ObjThread));
    h->hdr.type = OBJ_THREAD; h->hdr.marked = false;
    h->hdr.next = gc_objects; gc_objects = (Obj*)h;
    h->done = false; h->result = VAL_NIL_V;
    pthread_mutex_init(&h->mu, NULL);
    pthread_cond_init(&h->cv, NULL);

    Value *args = malloc(sizeof(Value) * (argc > 0 ? argc : 1));
    for (int i = 0; i < argc; i++) args[i] = argv[i];

    pthread_mutex_lock(&q_mu);
    if (q_count < PICO_TASK_QUEUE) {
        queue[q_tail] = (Task){fn, argc, args, h};
        q_tail = (q_tail + 1) % PICO_TASK_QUEUE;
        q_count++;
        pthread_cond_signal(&q_cv);
    }
    pthread_mutex_unlock(&q_mu);

    return (Value){VAL_THREAD, {.thread = h}};
}

Value thread_join(ObjThread *t) {
    pthread_mutex_lock(&t->mu);
    while (!t->done) pthread_cond_wait(&t->cv, &t->mu);
    pthread_mutex_unlock(&t->mu);
    return t->result;
}

Value mutex_new(void) {
    ObjMutex *m = gc_alloc(sizeof(ObjMutex));
    m->hdr.type = OBJ_MUTEX; m->hdr.marked = false;
    m->hdr.next = gc_objects; gc_objects = (Obj*)m;
    pthread_mutex_init(&m->mutex, NULL);
    return (Value){VAL_MUTEX, {.mutex = m}};
}
void mutex_lock(ObjMutex *m)   { pthread_mutex_lock(&m->mutex); }
void mutex_unlock(ObjMutex *m) { pthread_mutex_unlock(&m->mutex); }

Value channel_new(int capacity) {
    ObjChannel *c = gc_alloc(sizeof(ObjChannel));
    c->hdr.type = OBJ_CHANNEL; c->hdr.marked = false;
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
    while (c->count == c->capacity) pthread_cond_wait(&c->not_full, &c->mutex);
    c->buffer[c->tail] = val;
    c->tail = (c->tail + 1) % c->capacity;
    c->count++;
    pthread_cond_signal(&c->not_empty);
    pthread_mutex_unlock(&c->mutex);
}

Value channel_recv(ObjChannel *c) {
    pthread_mutex_lock(&c->mutex);
    while (c->count == 0) pthread_cond_wait(&c->not_empty, &c->mutex);
    Value val = c->buffer[c->head];
    c->head = (c->head + 1) % c->capacity;
    c->count--;
    pthread_cond_signal(&c->not_full);
    pthread_mutex_unlock(&c->mutex);
    return val;
}
