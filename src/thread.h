#pragma once
#include <pthread.h>
#include "value.h"

#define PICO_WORKERS 8
#define PICO_TASK_QUEUE 1024

struct ObjThread {
    Obj      hdr;
    Value    result;
    bool     done;
    pthread_mutex_t mu;
    pthread_cond_t  cv;
};

struct ObjMutex {
    Obj             hdr;
    pthread_mutex_t mutex;
};

struct ObjChannel {
    Obj             hdr;
    Value          *buffer;
    int             capacity;
    int             head, tail, count;
    pthread_mutex_t mutex;
    pthread_cond_t  not_empty, not_full;
};

void  scheduler_init(void);

Value thread_spawn(ObjFn *fn, int argc, Value *argv);
Value thread_join(ObjThread *t);

Value mutex_new(void);
void  mutex_lock(ObjMutex *m);
void  mutex_unlock(ObjMutex *m);

Value channel_new(int capacity);
void  channel_send(ObjChannel *c, Value val);
Value channel_recv(ObjChannel *c);
