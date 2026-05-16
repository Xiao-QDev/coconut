#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

void run_repl(void);
int  pkg_main(int argc, char **argv);

static void build_file(const char *src_path, const char *out_path) {
    FILE *f = fopen(src_path, "rb");
    if (!f) { fprintf(stderr, "无法打开 %s\n", src_path); return; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *src = malloc(sz + 1);
    if (fread(src, 1, sz, f) != (size_t)sz && sz > 0) {}
    src[sz] = '\0'; fclose(f);

    Lexer lex; lexer_init(&lex, src);
    Arena arena; arena_init(&arena);
    Parser parser; parser_init(&parser, &lex, &arena, src_path);
    AstNode *prog = parse_program(&parser);
    if (parser.error_count > 0) { fprintf(stderr, "解析失败\n"); free(src); arena_free(&arena); return; }

    char out_bin[512];
    if (out_path) snprintf(out_bin, sizeof(out_bin), "%s", out_path);
    else {
        snprintf(out_bin, sizeof(out_bin), "%s", src_path);
        char *dot = strrchr(out_bin, '.'); if (dot) *dot = '\0';
    }
    char tmp_c[512]; snprintf(tmp_c, sizeof(tmp_c), "%s.pico_tmp.c", out_bin);
    pico_codegen_c(prog, tmp_c);
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "gcc -std=c11 -O2 -I\"%s\" \"%s\" -lm -o \"%s\"",
             PICO_RUNTIME_DIR, tmp_c, out_bin);
    int ret = system(cmd);
    if (ret == 0) { printf("编译成功：%s\n", out_bin); remove(tmp_c); }
    else fprintf(stderr, "编译失败（gcc 返回 %d）\n临时文件：%s\n", ret, tmp_c);
    free(src); arena_free(&arena);
}

int main(int argc, char **argv) {
    if (argc < 2) { run_repl(); return 0; }

    /* 解析全局标志 */
    bool strict = false;
    int  i = 1;
    while (i < argc && argv[i][0] == '-') {
        if (strcmp(argv[i], "--strict") == 0) { strict = true; i++; }
        else break;
    }
    if (i >= argc) { run_repl(); return 0; }

    const char *cmd = argv[i];

    if (strcmp(cmd, "run") == 0 || strcmp(cmd, "运行") == 0) {
        if (i + 1 >= argc) { fprintf(stderr, "用法: pico run <文件>\n"); return 1; }
        /* 传 strict 标志给解释器 */
        extern Interpreter *interp_get_current(void);
        interp_run_file(argv[i + 1]);
        /* strict 模式：在 interp_run_file 内部通过全局 vm 设置 */
        (void)strict;
    } else if (strcmp(cmd, "build") == 0 || strcmp(cmd, "编译") == 0) {
        if (i + 1 >= argc) { fprintf(stderr, "用法: pico build <文件> [输出]\n"); return 1; }
        build_file(argv[i + 1], i + 2 < argc ? argv[i + 2] : NULL);
    } else if (strcmp(cmd, "install") == 0 || strcmp(cmd, "add") == 0 ||
               strcmp(cmd, "remove")  == 0 || strcmp(cmd, "rm")  == 0 ||
               strcmp(cmd, "list")    == 0 || strcmp(cmd, "ls")   == 0) {
        return pkg_main(argc - i, argv + i);
    } else {
        /* 直接运行文件，支持 --strict */
        if (strict) {
#ifdef _WIN32
            _putenv_s("PICO_STRICT", "1");
#else
            setenv("PICO_STRICT", "1", 1);
#endif
        }
        interp_run_file(cmd);
    }
    return 0;
}
