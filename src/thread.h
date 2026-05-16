#pragma once
#include <pthread.h>
#include "value.h"

struct ObjThread {
    Obj       hdr;
    pthread_t thread;
    Value     result;
    bool      done;
    ObjFn    *fn;
    int       argc;
    Value    *argv;
};

struct ObjMutex {
    Obj             hdr;
    pthread_mutex_t mutex;
};

struct ObjChannel {
    Obj             hdr;
    Value          *buffer;
    int             capacity;
    int             head;
    int             tail;
    int             count;
    pthread_mutex_t mutex;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
};

Value thread_spawn(ObjFn *fn, int argc, Value *argv);
Value thread_join(ObjThread *thread);

Value mutex_new();
void  mutex_lock(ObjMutex *m);
void  mutex_unlock(ObjMutex *m);

Value channel_new(int capacity);
void  channel_send(ObjChannel *c, Value val);
Value channel_recv(ObjChannel *c);
