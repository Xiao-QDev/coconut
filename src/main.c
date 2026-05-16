#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"

void run_repl();

int main(int argc, char **argv) {
    if (argc < 2) {
        run_repl();
        return 0;
    }

    if (strcmp(argv[1], "run") == 0 || strcmp(argv[1], "运行") == 0) {
        if (argc < 3) {
            fprintf(stderr, "用法: pico run <文件>\n");
            return 1;
        }
        interp_run_file(argv[2]);
    } else {
        // 默认作为文件执行
        interp_run_file(argv[1]);
    }

    return 0;
}
