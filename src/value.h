#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ── 向前声明 ──────────────────────────────────────────────────
typedef struct ObjStr ObjStr;
typedef struct ObjList ObjList;
typedef struct ObjMap ObjMap;
typedef struct ObjStructDef ObjStructDef;
typedef struct ObjInstance ObjInstance;
typedef struct ObjFn ObjFn;
typedef struct ObjCoro ObjCoro;
typedef struct ObjThread ObjThread;
typedef struct ObjMutex ObjMutex;
typedef struct ObjChannel ObjChannel;
typedef struct Env Env;

// ── GC 对象头（所有堆对象共享）──────────────────────────────
typedef enum {
    OBJ_STRING, OBJ_LIST, OBJ_MAP, OBJ_STRUCT_DEF,
    OBJ_INSTANCE, OBJ_FN, OBJ_CORO, OBJ_THREAD,
    OBJ_MUTEX, OBJ_CHANNEL, OBJ_UPVALUE
} ObjType;

typedef struct Obj {
    uint8_t     type;  
    bool        marked;
    struct Obj *next;
} Obj;

typedef enum {
    VAL_NIL, VAL_BOOL, VAL_INT, VAL_FLOAT,

    VAL_STRING, VAL_LIST, VAL_MAP,
    VAL_STRUCT_DEF, VAL_INSTANCE, VAL_FN, VAL_NATIVE,
    VAL_COROUTINE, VAL_THREAD, VAL_MUTEX, VAL_CHANNEL
} ValueType;

typedef struct Value Value;
typedef Value (*NativeFn)(int argc, Value *argv);

struct Value {
    ValueType type;
    union {
        bool        boolean;
        int64_t     integer;
        double      floating;
        ObjStr     *string;
        ObjList    *list;
        ObjMap     *map;
        ObjStructDef *structdef;
        ObjInstance*instance;
        ObjFn      *fn;
        NativeFn    native;
        ObjCoro    *coro;
        ObjThread  *thread;
        ObjMutex   *mutex;
        ObjChannel *channel;
    };
};

// ── 堆对象定义 ───────────────────────────────────────────────
struct ObjStr {
    Obj     hdr;
    int     len;
    uint32_t hash;
    char    data[];  // flexible array
};

struct ObjList {
    Obj    hdr;
    Value *items;
    int    len;
    int    cap;
};

struct ObjMap {
    Obj     hdr;
    ObjStr **keys;
    Value   *vals;
    int      len;
    int      cap;
};

typedef struct {
    ObjStr *name;
    int     index;
} Field;

typedef struct ObjStructDef {
    Obj     hdr;
    ObjStr *name;
    Field  *fields;
    int     field_count;
    ObjMap *methods;
    struct ObjStructDef *parent;  // 继承链
} ObjStructDef;

struct ObjInstance {
    Obj          hdr;
    ObjStructDef *def;
    Value        *fields;
};

typedef struct ObjUpvalue {
    Obj    hdr;
    Value *loc;
    Value  closed;
    struct ObjUpvalue *next;
} ObjUpvalue;

struct ObjFn {
    Obj         hdr;
    ObjStr     *name;
    int         arity;
    void       *body;      // AstNode* (block)
    char      **params;    // param names[arity]
    Env        *closure;
    bool        is_generator;
    bool        is_async;
};

// ── 构造宏 ──────────────────────────────────────────────────
#define VAL_NIL_V       ((Value){.type=VAL_NIL})
#define VAL_BOOL_V(b)   ((Value){.type=VAL_BOOL,   .boolean=(b)})
#define VAL_INT_V(n)    ((Value){.type=VAL_INT,    .integer=(n)})
#define VAL_FLOAT_V(f)  ((Value){.type=VAL_FLOAT,  .floating=(f)})
#define VAL_STR_V(s)    ((Value){.type=VAL_STRING, .string=(s)})
#define VAL_LIST_V(l)   ((Value){.type=VAL_LIST,   .list=(l)})
#define VAL_MAP_V(m)    ((Value){.type=VAL_MAP,    .map=(m)})
#define VAL_STRUCT_DEF_V(d) ((Value){.type=VAL_STRUCT_DEF, .structdef=(d)})
#define VAL_INST_V(i)   ((Value){.type=VAL_INSTANCE, .instance=(i)})
#define VAL_FN_V(f)     ((Value){.type=VAL_FN,     .fn=(f)})
#define VAL_NATIVE_V(f) ((Value){.type=VAL_NATIVE, .native=(f)})
#define IS_NIL(v)       ((v).type==VAL_NIL)
#define IS_BOOL(v)      ((v).type==VAL_BOOL)
#define IS_INT(v)       ((v).type==VAL_INT)
#define IS_FLOAT(v)     ((v).type==VAL_FLOAT)
#define IS_NUM(v)       (IS_INT(v)||IS_FLOAT(v))
#define IS_STR(v)       ((v).type==VAL_STRING)
#define IS_LIST(v)      ((v).type==VAL_LIST)
#define IS_MAP(v)       ((v).type==VAL_MAP)
#define IS_FN(v)        ((v).type==VAL_FN)
#define IS_THREAD(v)    ((v).type==VAL_THREAD)
#define IS_TRUTHY(v)    (!IS_NIL(v) && !(IS_BOOL(v) && !(v).boolean))

// ── GC 全局链表 ─────────────────────────────────────────────
extern Obj *gc_objects;
extern size_t gc_bytes_allocated;
extern size_t gc_next_gc;

void *gc_alloc(size_t size);
void  gc_collect(void);
void  gc_mark_value(Value v);
void  gc_mark_env(Env *e);
void  gc_mark_roots(void);

// ── 字符串驻留 ───────────────────────────────────────────────
ObjStr *str_intern(const char *data, int len);
ObjStr *str_concat(ObjStr *a, ObjStr *b);

// ── 列表/字典 ────────────────────────────────────────────────
ObjList *list_new(void);
void     list_push(ObjList *l, Value v);
Value    list_get(ObjList *l, int i);

ObjMap  *map_new(void);
void     map_set(ObjMap *m, ObjStr *key, Value val);
bool     map_get(ObjMap *m, ObjStr *key, Value *out);
bool     map_del(ObjMap *m, ObjStr *key);

ObjStructDef *struct_def_new(ObjStr *name, int field_count);
ObjInstance  *instance_new(ObjStructDef *def);

// ── 打印 ─────────────────────────────────────────────────────
void value_print(Value v);
void value_println(Value v);
bool value_equal(Value a, Value b);
