#define _POSIX_C_SOURCE 200809L
#include "../value.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

static Value str_upper(int argc, Value *argv) {
    if (argc < 1 || !IS_STR(argv[0])) return VAL_NIL_V;
    ObjStr *s = argv[0].string;
    char *buf = malloc(s->len + 1);
    for (int i = 0; i < s->len; i++) buf[i] = (char)toupper((unsigned char)s->data[i]);
    buf[s->len] = '\0';
    Value r = VAL_STR_V(str_intern(buf, s->len));
    free(buf);
    return r;
}

static Value str_lower(int argc, Value *argv) {
    if (argc < 1 || !IS_STR(argv[0])) return VAL_NIL_V;
    ObjStr *s = argv[0].string;
    char *buf = malloc(s->len + 1);
    for (int i = 0; i < s->len; i++) buf[i] = (char)tolower((unsigned char)s->data[i]);
    buf[s->len] = '\0';
    Value r = VAL_STR_V(str_intern(buf, s->len));
    free(buf);
    return r;
}

static Value str_trim(int argc, Value *argv) {
    if (argc < 1 || !IS_STR(argv[0])) return VAL_NIL_V;
    const char *s = argv[0].string->data;
    int len = argv[0].string->len;
    int start = 0, end = len - 1;
    while (start <= end && isspace((unsigned char)s[start])) start++;
    while (end >= start && isspace((unsigned char)s[end])) end--;
    return VAL_STR_V(str_intern(s + start, end - start + 1));
}

static Value str_split(int argc, Value *argv) {
    if (argc < 2 || !IS_STR(argv[0]) || !IS_STR(argv[1])) return VAL_NIL_V;
    const char *s = argv[0].string->data;
    int slen = argv[0].string->len;
    const char *sep = argv[1].string->data;
    int seplen = argv[1].string->len;
    ObjList *list = list_new();
    if (seplen == 0) {
        for (int i = 0; i < slen; i++) list_push(list, VAL_STR_V(str_intern(s + i, 1)));
        return VAL_LIST_V(list);
    }
    const char *p = s, *end = s + slen;
    while (p <= end) {
        const char *found = seplen > 0 ? strstr(p, sep) : NULL;
        if (!found || found > end) found = end;
        list_push(list, VAL_STR_V(str_intern(p, (int)(found - p))));
        if (found == end) break;
        p = found + seplen;
    }
    return VAL_LIST_V(list);
}

static Value str_join(int argc, Value *argv) {
    if (argc < 2 || !IS_LIST(argv[0]) || !IS_STR(argv[1])) return VAL_NIL_V;
    ObjList *list = argv[0].list;
    const char *sep = argv[1].string->data;
    int seplen = argv[1].string->len;
    int total = 0;
    for (int i = 0; i < list->len; i++) {
        if (!IS_STR(list->items[i])) continue;
        total += list->items[i].string->len;
        if (i < list->len - 1) total += seplen;
    }
    char *buf = malloc(total + 1);
    int pos = 0;
    for (int i = 0; i < list->len; i++) {
        if (!IS_STR(list->items[i])) continue;
        memcpy(buf + pos, list->items[i].string->data, list->items[i].string->len);
        pos += list->items[i].string->len;
        if (i < list->len - 1) { memcpy(buf + pos, sep, seplen); pos += seplen; }
    }
    buf[pos] = '\0';
    Value r = VAL_STR_V(str_intern(buf, pos));
    free(buf);
    return r;
}

static Value str_contains(int argc, Value *argv) {
    if (argc < 2 || !IS_STR(argv[0]) || !IS_STR(argv[1])) return VAL_BOOL_V(false);
    char tmp0[argv[0].string->len + 1], tmp1[argv[1].string->len + 1];
    memcpy(tmp0, argv[0].string->data, argv[0].string->len); tmp0[argv[0].string->len] = '\0';
    memcpy(tmp1, argv[1].string->data, argv[1].string->len); tmp1[argv[1].string->len] = '\0';
    return VAL_BOOL_V(strstr(tmp0, tmp1) != NULL);
}

static Value str_replace(int argc, Value *argv) {
    if (argc < 3 || !IS_STR(argv[0]) || !IS_STR(argv[1]) || !IS_STR(argv[2])) return argc > 0 ? argv[0] : VAL_NIL_V;
    const char *s = argv[0].string->data; int slen = argv[0].string->len;
    const char *old = argv[1].string->data; int oldlen = argv[1].string->len;
    const char *newstr = argv[2].string->data; int newlen = argv[2].string->len;
    if (oldlen == 0) return argv[0];
    // count occurrences
    int count = 0;
    const char *p = s;
    while ((p = strstr(p, old)) && (p - s) < slen) { count++; p += oldlen; }
    int outlen = slen + count * (newlen - oldlen);
    char *buf = malloc(outlen + 1);
    char *out = buf;
    p = s;
    while (1) {
        const char *found = strstr(p, old);
        if (!found || (found - s) >= slen) { memcpy(out, p, slen - (p - s)); out += slen - (p - s); break; }
        memcpy(out, p, found - p); out += found - p;
        memcpy(out, newstr, newlen); out += newlen;
        p = found + oldlen;
    }
    *out = '\0';
    Value r = VAL_STR_V(str_intern(buf, (int)(out - buf)));
    free(buf);
    return r;
}

static Value str_len(int argc, Value *argv) {
    if (argc < 1 || !IS_STR(argv[0])) return VAL_INT_V(0);
    return VAL_INT_V(argv[0].string->len);
}

static Value str_starts_with(int argc, Value *argv) {
    if (argc < 2 || !IS_STR(argv[0]) || !IS_STR(argv[1])) return VAL_BOOL_V(false);
    ObjStr *s = argv[0].string, *pre = argv[1].string;
    if (pre->len > s->len) return VAL_BOOL_V(false);
    return VAL_BOOL_V(memcmp(s->data, pre->data, pre->len) == 0);
}

static Value str_ends_with(int argc, Value *argv) {
    if (argc < 2 || !IS_STR(argv[0]) || !IS_STR(argv[1])) return VAL_BOOL_V(false);
    ObjStr *s = argv[0].string, *suf = argv[1].string;
    if (suf->len > s->len) return VAL_BOOL_V(false);
    return VAL_BOOL_V(memcmp(s->data + s->len - suf->len, suf->data, suf->len) == 0);
}

ObjMap *stdlib_str_module(void) {
    ObjMap *m = map_new();
    map_set(m, str_intern("upper",       5), VAL_NATIVE_V(str_upper));
    map_set(m, str_intern("lower",       5), VAL_NATIVE_V(str_lower));
    map_set(m, str_intern("trim",        4), VAL_NATIVE_V(str_trim));
    map_set(m, str_intern("split",       5), VAL_NATIVE_V(str_split));
    map_set(m, str_intern("join",        4), VAL_NATIVE_V(str_join));
    map_set(m, str_intern("contains",    8), VAL_NATIVE_V(str_contains));
    map_set(m, str_intern("replace",     7), VAL_NATIVE_V(str_replace));
    map_set(m, str_intern("len",         3), VAL_NATIVE_V(str_len));
    map_set(m, str_intern("starts_with", 11), VAL_NATIVE_V(str_starts_with));
    map_set(m, str_intern("ends_with",   9), VAL_NATIVE_V(str_ends_with));
    return m;
}
