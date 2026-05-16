#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../value.h"

// 极简 JSON 序列化
static void stringify_value(Value v, char *buf, int *pos) {
    if (IS_NIL(v)) { *pos += sprintf(buf + *pos, "null"); }
    else if (IS_BOOL(v)) { *pos += sprintf(buf + *pos, v.boolean ? "true" : "false"); }
    else if (IS_INT(v)) { *pos += sprintf(buf + *pos, "%lld", v.integer); }
    else if (IS_FLOAT(v)) { *pos += sprintf(buf + *pos, "%g", v.floating); }
    else if (IS_STR(v)) { *pos += sprintf(buf + *pos, "\"%s\"", v.string->data); }
    else if (IS_LIST(v)) {
        *pos += sprintf(buf + *pos, "[");
        for (int i = 0; i < v.list->len; i++) {
            if (i > 0) *pos += sprintf(buf + *pos, ",");
            stringify_value(v.list->items[i], buf, pos);
        }
        *pos += sprintf(buf + *pos, "]");
    }
    else if (IS_MAP(v)) {
        *pos += sprintf(buf + *pos, "{");
        for (int i = 0; i < v.map->len; i++) {
            if (i > 0) *pos += sprintf(buf + *pos, ",");
            *pos += sprintf(buf + *pos, "\"%s\":", v.map->keys[i]->data);
            stringify_value(v.map->vals[i], buf, pos);
        }
        *pos += sprintf(buf + *pos, "}");
    }
}

Value json_stringify(int argc, Value *argv) {
    if (argc == 0) return VAL_NIL_V;
    char buf[8192] = {0};
    int pos = 0;
    stringify_value(argv[0], buf, &pos);
    return VAL_STR_V(str_intern(buf, (int)strlen(buf)));
}
