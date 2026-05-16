#include "value.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Obj   *gc_objects       = NULL;
size_t gc_bytes_allocated = 0;
size_t gc_next_gc       = 1024 * 1024;

void *gc_alloc(size_t size) {
    gc_bytes_allocated += size;
    void *p = calloc(1, size);
    if (!p) { fprintf(stderr, "out of memory\n"); exit(1); }
    return p;
}

// ── 字符串驻留 ───────────────────────────────────────────────
#define STR_TABLE_SIZE 4096
static ObjStr *str_table[STR_TABLE_SIZE];

static uint32_t hash_str(const char *s, int len) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < len; i++) h = (h ^ (unsigned char)s[i]) * 16777619u;
    return h;
}

ObjStr *str_intern(const char *data, int len) {
    uint32_t h = hash_str(data, len);
    int slot = h & (STR_TABLE_SIZE - 1);
    for (ObjStr *s = str_table[slot]; s; s = (ObjStr*)s->hdr.next) {
        if (s->len == len && memcmp(s->data, data, len) == 0) return s;
    }
    ObjStr *s = gc_alloc(sizeof(ObjStr) + len + 1);
    s->hdr.type   = OBJ_STRING;
    s->hdr.marked = false;
    s->len  = len;
    s->hash = h;
    memcpy(s->data, data, len);
    s->data[len] = '\0';
    // 插入驻留表
    s->hdr.next = (Obj*)str_table[slot];
    str_table[slot] = s;
    // 插入 GC 链表
    ((Obj*)s)->next = gc_objects;
    gc_objects = (Obj*)s;
    return s;
}

ObjStr *str_concat(ObjStr *a, ObjStr *b) {
    int len = a->len + b->len;
    char *buf = malloc(len + 1);
    memcpy(buf, a->data, a->len);
    memcpy(buf + a->len, b->data, b->len);
    buf[len] = '\0';
    ObjStr *s = str_intern(buf, len);
    free(buf);
    return s;
}

// ── 列表 ─────────────────────────────────────────────────────
ObjList *list_new(void) {
    ObjList *l = gc_alloc(sizeof(ObjList));
    l->hdr.type = OBJ_LIST;
    l->items = NULL; l->len = 0; l->cap = 0;
    l->hdr.next = gc_objects; gc_objects = (Obj*)l;
    return l;
}

void list_push(ObjList *l, Value v) {
    if (l->len >= l->cap) {
        int nc = l->cap ? l->cap * 2 : 4;
        l->items = realloc(l->items, nc * sizeof(Value));
        l->cap = nc;
    }
    l->items[l->len++] = v;
}

Value list_get(ObjList *l, int i) {
    if (i < 0 || i >= l->len) return VAL_NIL_V;
    return l->items[i];
}

ObjMap *map_new(void) {
    ObjMap *m = gc_alloc(sizeof(ObjMap));
    m->hdr.type = OBJ_MAP;
    m->hdr.marked = false;
    m->hdr.next = gc_objects; gc_objects = (Obj*)m;
    m->len = 0;
    m->cap = 8;
    m->keys = malloc(sizeof(ObjStr*) * m->cap);
    m->vals = malloc(sizeof(Value) * m->cap);
    return m;
}

void map_set(ObjMap *m, ObjStr *key, Value val) {
    for (int i = 0; i < m->len; i++) {
        if (m->keys[i] == key) { m->vals[i] = val; return; }
    }
    if (m->len >= m->cap) {
        m->cap *= 2;
        m->keys = realloc(m->keys, sizeof(ObjStr*) * m->cap);
        m->vals = realloc(m->vals, sizeof(Value) * m->cap);
    }
    m->keys[m->len] = key;
    m->vals[m->len] = val;
    m->len++;
}

bool map_get(ObjMap *m, ObjStr *key, Value *out) {
    for (int i = 0; i < m->len; i++) {
        if (m->keys[i] == key) { *out = m->vals[i]; return true; }
    }
    return false;
}

ObjStructDef *struct_def_new(ObjStr *name, int field_count) {
    ObjStructDef *d = gc_alloc(sizeof(ObjStructDef));
    d->hdr.type = OBJ_STRUCT_DEF;
    d->hdr.marked = false;
    d->hdr.next = gc_objects; gc_objects = (Obj*)d;
    d->name = name;
    d->field_count = field_count;
    d->fields = malloc(sizeof(Field) * field_count);
    d->methods = map_new();
    return d;
}

ObjInstance *instance_new(ObjStructDef *def) {
    ObjInstance *i = gc_alloc(sizeof(ObjInstance));
    i->hdr.type = OBJ_INSTANCE;
    i->hdr.marked = false;
    i->hdr.next = gc_objects; gc_objects = (Obj*)i;
    i->def = def;
    i->fields = malloc(sizeof(Value) * def->field_count);
    for (int j = 0; j < def->field_count; j++) i->fields[j] = VAL_NIL_V;
    return i;
}


// ── 字典 ─────────────────────────────────────────────────────
ObjMap *map_new(void) {
    ObjMap *m = gc_alloc(sizeof(ObjMap));
    m->hdr.type = OBJ_MAP;
    m->keys = NULL; m->vals = NULL; m->len = 0; m->cap = 0;
    m->hdr.next = gc_objects; gc_objects = (Obj*)m;
    return m;
}

void map_set(ObjMap *m, ObjStr *key, Value val) {
    for (int i = 0; i < m->len; i++) {
        if (m->keys[i] == key) { m->vals[i] = val; return; }
    }
    if (m->len >= m->cap) {
        int nc = m->cap ? m->cap * 2 : 4;
        m->keys = realloc(m->keys, nc * sizeof(ObjStr*));
        m->vals = realloc(m->vals, nc * sizeof(Value));
        m->cap = nc;
    }
    m->keys[m->len] = key;
    m->vals[m->len] = val;
    m->len++;
}

bool map_get(ObjMap *m, ObjStr *key, Value *out) {
    for (int i = 0; i < m->len; i++) {
        if (m->keys[i] == key) { *out = m->vals[i]; return true; }
    }
    return false;
}

bool map_del(ObjMap *m, ObjStr *key) {
    for (int i = 0; i < m->len; i++) {
        if (m->keys[i] == key) {
            m->keys[i] = m->keys[m->len-1];
            m->vals[i] = m->vals[m->len-1];
            m->len--;
            return true;
        }
    }
    return false;
}

// ── 打印 ─────────────────────────────────────────────────────
void value_print(Value v) {
    switch (v.type) {
        case VAL_NIL:   printf("nil"); break;
        case VAL_BOOL:  printf("%s", v.boolean ? "true" : "false"); break;
        case VAL_INT:   printf("%lld", (long long)v.integer); break;
        case VAL_FLOAT: printf("%g", v.floating); break;
        case VAL_STRING:printf("%s", v.string->data); break;
        case VAL_LIST:
            printf("[");
            for (int i = 0; i < v.list->len; i++) {
                if (i) printf(", ");
                if (v.list->items[i].type == VAL_STRING)
                    printf("\"%s\"", v.list->items[i].string->data);
                else value_print(v.list->items[i]);
            }
            printf("]");
            break;
        case VAL_MAP:
            printf("{");
            for (int i = 0; i < v.map->len; i++) {
                if (i) printf(", ");
                printf("%s: ", v.map->keys[i]->data);
                value_print(v.map->vals[i]);
            }
            printf("}");
            break;
        case VAL_FN:
            printf("<fn %s>", v.fn->name ? v.fn->name->data : "?");
            break;
        case VAL_NATIVE: printf("<native>"); break;
        case VAL_INSTANCE:
            printf("<%s>", v.instance->def->name->data);
            break;
        default: printf("<value>"); break;
    }
}

void value_println(Value v) { value_print(v); printf("\n"); }

bool value_equal(Value a, Value b) {
    if (a.type != b.type) {
        if (IS_INT(a) && IS_FLOAT(b)) return (double)a.integer == b.floating;
        if (IS_FLOAT(a) && IS_INT(b)) return a.floating == (double)b.integer;
        return false;
    }
    switch (a.type) {
        case VAL_NIL:   return true;
        case VAL_BOOL:  return a.boolean == b.boolean;
        case VAL_INT:   return a.integer == b.integer;
        case VAL_FLOAT: return a.floating == b.floating;
        case VAL_STRING:return a.string == b.string;  // 驻留后指针比较
        default:        return a.string == b.string;  // 指针相等
    }
}

// ── GC（简单标记清除，实现在 gc.c）────────────────────────────
void gc_mark_env(Env *e) {
    while (e) {
        for (int i = 0; i < e->count; i++) {
            gc_mark_value(VAL_STR_V(e->keys[i]));
            gc_mark_value(e->vals[i]);
        }
        e = e->parent;
    }
}

void gc_mark_value(Value v) {
    Obj *o = NULL;
    switch (v.type) {
        case VAL_STRING:     o = (Obj*)v.string;   break;
        case VAL_LIST:       o = (Obj*)v.list;     break;
        case VAL_MAP:        o = (Obj*)v.map;      break;
        case VAL_STRUCT_DEF: o = (Obj*)v.structdef;break;
        case VAL_INSTANCE:   o = (Obj*)v.instance; break;
        case VAL_FN:         o = (Obj*)v.fn;       break;
        case VAL_COROUTINE:  o = (Obj*)v.coro;     break;
        case VAL_THREAD:     o = (Obj*)v.thread;   break;
        default: return;
    }
    if (!o || o->marked) return;
    o->marked = true;

    if (v.type == VAL_LIST) {
        for (int i = 0; i < v.list->len; i++) gc_mark_value(v.list->items[i]);
    } else if (v.type == VAL_MAP) {
        for (int i = 0; i < v.map->len; i++) gc_mark_value(v.map->vals[i]);
    } else if (v.type == VAL_STRUCT_DEF) {
        gc_mark_value(VAL_STR_V(v.structdef->name));
        gc_mark_value(VAL_MAP_V(v.structdef->methods));
    } else if (v.type == VAL_INSTANCE) {
        gc_mark_value(VAL_STRUCT_DEF_V(v.instance->def));
        for (int i = 0; i < v.instance->def->field_count; i++) gc_mark_value(v.instance->fields[i]);
    } else if (v.type == VAL_FN) {
        if (v.fn->name) gc_mark_value(VAL_STR_V(v.fn->name));
        gc_mark_env(v.fn->closure);
    } else if (v.type == VAL_THREAD) {
        gc_mark_value(VAL_FN_V(v.thread->fn));
        for (int i = 0; i < v.thread->argc; i++) gc_mark_value(v.thread->argv[i]);
        gc_mark_value(v.thread->result);
    }
}
