#include "gc.h"
#include "value.h"
#include "thread.h"
#include <stdlib.h>

void gc_collect(void) {
    gc_mark_roots();
    // 清除未标记对象
    Obj **obj = &gc_objects;
    while (*obj) {
        if (!(*obj)->marked) {
            Obj *unreached = *obj;
            *obj = unreached->next;
            // 释放附属内存
            if (unreached->type == OBJ_LIST) {
                free(((ObjList*)unreached)->items);
            } else if (unreached->type == OBJ_MAP) {
                free(((ObjMap*)unreached)->keys);
                free(((ObjMap*)unreached)->vals);
            } else if (unreached->type == OBJ_INSTANCE) {
                free(((ObjInstance*)unreached)->fields);
            } else if (unreached->type == OBJ_FN) {
                ObjFn *fn = (ObjFn*)unreached;
                for (int i = 0; i < fn->arity; i++) free(fn->params[i]);
                free(fn->params);
            } else if (unreached->type == OBJ_THREAD) {
                ObjThread *t = (ObjThread*)unreached;
                if (!t->done) pthread_detach(t->thread);
                free(t->argv);
            }
            free(unreached);
        } else {
            (*obj)->marked = false;
            obj = &(*obj)->next;
        }
    }
    gc_next_gc = gc_bytes_allocated * 2;
}
