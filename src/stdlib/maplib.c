#include "../value.h"

static Value maplib_keys(int argc, Value *argv) {
    if (argc < 1 || !IS_MAP(argv[0])) return VAL_NIL_V;
    ObjMap *m = argv[0].map;
    ObjList *l = list_new();
    for (int i = 0; i < m->cap; i++)
        if (m->keys[i]) list_push(l, VAL_STR_V(m->keys[i]));
    return VAL_LIST_V(l);
}

static Value maplib_values(int argc, Value *argv) {
    if (argc < 1 || !IS_MAP(argv[0])) return VAL_NIL_V;
    ObjMap *m = argv[0].map;
    ObjList *l = list_new();
    for (int i = 0; i < m->cap; i++)
        if (m->keys[i]) list_push(l, m->vals[i]);
    return VAL_LIST_V(l);
}

static Value maplib_has(int argc, Value *argv) {
    if (argc < 2 || !IS_MAP(argv[0]) || !IS_STR(argv[1])) return VAL_BOOL_V(false);
    Value out; return VAL_BOOL_V(map_get(argv[0].map, argv[1].string, &out));
}

static Value maplib_delete(int argc, Value *argv) {
    if (argc < 2 || !IS_MAP(argv[0]) || !IS_STR(argv[1])) return VAL_NIL_V;
    map_del(argv[0].map, argv[1].string);
    return VAL_NIL_V;
}

static Value maplib_size(int argc, Value *argv) {
    if (argc < 1 || !IS_MAP(argv[0])) return VAL_INT_V(0);
    return VAL_INT_V(argv[0].map->len);
}

ObjMap *stdlib_maplib_module(void) {
    ObjMap *m = map_new();
    map_set(m, str_intern("keys",   4), VAL_NATIVE_V(maplib_keys));
    map_set(m, str_intern("values", 6), VAL_NATIVE_V(maplib_values));
    map_set(m, str_intern("has",    3), VAL_NATIVE_V(maplib_has));
    map_set(m, str_intern("delete", 6), VAL_NATIVE_V(maplib_delete));
    map_set(m, str_intern("size",   4), VAL_NATIVE_V(maplib_size));
    return m;
}
