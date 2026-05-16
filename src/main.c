#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

void run_repl(void);

static void build_file(const char *src_path, const char *out_path) {
    // read source
    FILE *f = fopen(src_path, "rb");
    if (!f) { fprintf(stderr, "无法打开 %s\n", src_path); return; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *src = malloc(sz + 1);
    if (fread(src, 1, sz, f) != (size_t)sz && sz > 0) {}
    src[sz] = '\0'; fclose(f);

    // parse
    Lexer lex; lexer_init(&lex, src);
    Arena arena; arena_init(&arena);
    Parser parser; parser_init(&parser, &lex, &arena, src_path);
    AstNode *prog = parse_program(&parser);

    if (parser.error_count > 0) {
        fprintf(stderr, "解析失败，中止编译\n");
        free(src); arena_free(&arena); return;
    }

    // determine output name
    char out_bin[512];
    if (out_path) {
        snprintf(out_bin, sizeof(out_bin), "%s", out_path);
    } else {
        // strip .pico extension
        snprintf(out_bin, sizeof(out_bin), "%s", src_path);
        char *dot = strrchr(out_bin, '.');
        if (dot) *dot = '\0';
    }

    // generate C to temp file
    char tmp_c[512];
    snprintf(tmp_c, sizeof(tmp_c), "%s.pico_tmp.c", out_bin);

    // copy pico_runtime.h next to tmp file (same dir as binary)
    // embed runtime inline — codegen already includes it via path
    pico_codegen_c(prog, tmp_c);

    // compile with gcc
    char cmd[1024];
    // find runtime header: same dir as pico executable or src/
    snprintf(cmd, sizeof(cmd),
        "gcc -std=c11 -O2 -I\"%s\" \"%s\" -lm -o \"%s\"",
        PICO_RUNTIME_DIR, tmp_c, out_bin);
    int ret = system(cmd);
    if (ret == 0) {
        printf("编译成功：%s\n", out_bin);
        remove(tmp_c);
    } else {
        fprintf(stderr, "编译失败（gcc 返回 %d）\n生成的 C 代码保留在：%s\n", ret, tmp_c);
    }

    free(src); arena_free(&arena);
}

int main(int argc, char **argv) {
    if (argc < 2) { run_repl(); return 0; }

    if (strcmp(argv[1], "run") == 0 || strcmp(argv[1], "运行") == 0) {
        if (argc < 3) { fprintf(stderr, "用法: pico run <文件>\n"); return 1; }
        interp_run_file(argv[2]);
    } else if (strcmp(argv[1], "build") == 0 || strcmp(argv[1], "编译") == 0) {
        if (argc < 3) { fprintf(stderr, "用法: pico build <文件> [输出]\n"); return 1; }
        build_file(argv[2], argc >= 4 ? argv[3] : NULL);
    } else {
        interp_run_file(argv[1]);
    }
    return 0;
}
