#pragma once
#include <stdarg.h>

typedef struct {
    const char *file;
    int         line;
    int         col;
    char        msg[256];
    char        suggestion[128];
} PicoError;

void pico_error(const char *file, int line, int col, const char *fmt, ...);
void pico_error_suggest(const char *file, int line, int col,
                        const char *suggestion, const char *fmt, ...);
// Levenshtein 距离
int  edit_distance(const char *a, const char *b);
// 在 names[n] 中找最近的，距离<=2 返回，否则 NULL
const char *best_match(const char *word, const char **names, int n);
