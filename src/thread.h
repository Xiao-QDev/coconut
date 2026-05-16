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

Value thread_spawn(ObjFn *fn, int argc, Value *argv);
Value thread_join(ObjThread *thread);
void  thread_init();
