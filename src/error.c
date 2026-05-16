#include "error.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static bool use_color(void) { return isatty(fileno(stderr)); }

void pico_error(const char *file, int line, int col, const char *fmt, ...) {
    bool color = use_color();
    if (color) fprintf(stderr, "\033[1;31m");
    fprintf(stderr, "错误");
    if (color) fprintf(stderr, "\033[0m");
    fprintf(stderr, "：");
    va_list ap; va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap);
    fprintf(stderr, " [%s 第%d行，第%d列]\n", file ? file : "<input>", line, col);
}

void pico_error_suggest(const char *file, int line, int col,
                        const char *suggestion, const char *fmt, ...) {
    pico_error(file, line, col, fmt);  // reuse — va_list trick not needed here
    (void)fmt;
    if (suggestion)
        fprintf(stderr, "  = 你是否想说：'%s'？\n", suggestion);
}

// ── Levenshtein ──────────────────────────────────────────────
int edit_distance(const char *a, const char *b) {
    int la = strlen(a), lb = strlen(b);
    if (la > 32 || lb > 32) return 99;
    int dp[33][33];
    for (int i = 0; i <= la; i++) dp[i][0] = i;
    for (int j = 0; j <= lb; j++) dp[0][j] = j;
    for (int i = 1; i <= la; i++)
        for (int j = 1; j <= lb; j++) {
            int cost = a[i-1] != b[j-1];
            int mn = dp[i-1][j] + 1;
            if (dp[i][j-1]+1 < mn) mn = dp[i][j-1]+1;
            if (dp[i-1][j-1]+cost < mn) mn = dp[i-1][j-1]+cost;
            dp[i][j] = mn;
        }
    return dp[la][lb];
}

const char *best_match(const char *word, const char **names, int n) {
    const char *best = NULL;
    int best_d = 3;
    for (int i = 0; i < n; i++) {
        int d = edit_distance(word, names[i]);
        if (d < best_d) { best_d = d; best = names[i]; }
    }
    return best;
}
