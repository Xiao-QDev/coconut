#define _POSIX_C_SOURCE 200809L
#include "../value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Value file_read(int argc, Value *argv) {
    if (argc < 1 || !IS_STR(argv[0])) return VAL_NIL_V;
    char path[512]; int plen = argv[0].string->len < 511 ? argv[0].string->len : 511;
    memcpy(path, argv[0].string->data, plen); path[plen] = '\0';
    FILE *f = fopen(path, "rb");
    if (!f) return VAL_NIL_V;
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *buf = malloc(sz + 1);
    if (fread(buf, 1, sz, f) != (size_t)sz && sz > 0) {}
    buf[sz] = '\0'; fclose(f);
    Value r = VAL_STR_V(str_intern(buf, (int)sz));
    free(buf); return r;
}

static Value file_write(int argc, Value *argv) {
    if (argc < 2 || !IS_STR(argv[0]) || !IS_STR(argv[1])) return VAL_NIL_V;
    char path[512]; int plen = argv[0].string->len < 511 ? argv[0].string->len : 511;
    memcpy(path, argv[0].string->data, plen); path[plen] = '\0';
    FILE *f = fopen(path, "wb");
    if (!f) return VAL_NIL_V;
    fwrite(argv[1].string->data, 1, argv[1].string->len, f);
    fclose(f); return VAL_NIL_V;
}

static Value file_append(int argc, Value *argv) {
    if (argc < 2 || !IS_STR(argv[0]) || !IS_STR(argv[1])) return VAL_NIL_V;
    char path[512]; int plen = argv[0].string->len < 511 ? argv[0].string->len : 511;
    memcpy(path, argv[0].string->data, plen); path[plen] = '\0';
    FILE *f = fopen(path, "ab");
    if (!f) return VAL_NIL_V;
    fwrite(argv[1].string->data, 1, argv[1].string->len, f);
    fclose(f); return VAL_NIL_V;
}

static Value file_exists(int argc, Value *argv) {
    if (argc < 1 || !IS_STR(argv[0])) return VAL_BOOL_V(false);
    char path[512]; int plen = argv[0].string->len < 511 ? argv[0].string->len : 511;
    memcpy(path, argv[0].string->data, plen); path[plen] = '\0';
    FILE *f = fopen(path, "rb");
    if (!f) return VAL_BOOL_V(false);
    fclose(f); return VAL_BOOL_V(true);
}

static Value file_lines(int argc, Value *argv) {
    if (argc < 1 || !IS_STR(argv[0])) return VAL_NIL_V;
    char path[512]; int plen = argv[0].string->len < 511 ? argv[0].string->len : 511;
    memcpy(path, argv[0].string->data, plen); path[plen] = '\0';
    FILE *f = fopen(path, "rb");
    if (!f) return VAL_NIL_V;
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *buf = malloc(sz + 1);
    if (fread(buf, 1, sz, f) != (size_t)sz && sz > 0) {}
    buf[sz] = '\0'; fclose(f);
    ObjList *list = list_new();
    char *p = buf, *end = buf + sz;
    while (p <= end) {
        char *nl = memchr(p, '\n', end - p);
        if (!nl) nl = end;
        int len = (int)(nl - p);
        if (len > 0 && p[len-1] == '\r') len--;
        list_push(list, VAL_STR_V(str_intern(p, len)));
        if (nl == end) break;
        p = nl + 1;
    }
    free(buf);
    return VAL_LIST_V(list);
}

ObjMap *stdlib_file_module(void) {
    ObjMap *m = map_new();
    map_set(m, str_intern("read",   4), VAL_NATIVE_V(file_read));
    map_set(m, str_intern("write",  5), VAL_NATIVE_V(file_write));
    map_set(m, str_intern("append", 6), VAL_NATIVE_V(file_append));
    map_set(m, str_intern("exists", 6), VAL_NATIVE_V(file_exists));
    map_set(m, str_intern("lines",  5), VAL_NATIVE_V(file_lines));
    return m;
}
