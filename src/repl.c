#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

void run_repl() {
    printf("Pico 语言 REPL (输入 :退出 或 :exit 退出)\n");
    
    Interpreter vm;
    interp_init(&vm);
    Arena arena;
    arena_init(&arena);

    char line[1024];
    while (1) {
        printf("pico> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        
        if (strcmp(line, ":退出\n") == 0 || strcmp(line, ":exit\n") == 0 || strcmp(line, ":quit\n") == 0) break;
        
        if (line[0] == '\n') continue;

        Lexer l;
        lexer_init(&l, line);
        Parser p;
        parser_init(&p, &l, &arena, "repl");

        // 尝试解析为表达式
        // Note: Our parser doesn't have a special "parse_expr_stmt" for REPL that prints results.
        // We'll just parse as a program and execute.
        AstNode *prog = parse_program(&p);
        if (p.error_count == 0) {
            Value res = interp_exec(&vm, prog, vm.globals);
            if (vm.has_error) {
                fprintf(stderr, "错误: %s\n", vm.error_msg);
                vm.has_error = false;
            } else {
                // 如果最后一条语句是表达式，打印它
                if (prog->program.stmts.count > 0) {
                    AstNode *last = prog->program.stmts.items[prog->program.stmts.count-1];
                    if (last->type == NODE_EXPR_STMT) {
                        value_println(res);
                    }
                }
            }
        }
        // In REPL, we might want to keep the Arena or clear it.
        // If we keep it, we can define functions.
        // But if we clear it, we lose them.
        // For now, don't clear it, but beware of memory.
    }
    arena_free(&arena);
}
