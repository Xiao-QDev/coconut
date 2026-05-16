#include <stdio.h>
#include <string.h>
#include "ast.h"

static void gen_node(FILE *out, AstNode *node);

static void gen_block(FILE *out, AstNode *node) {
    fprintf(out, "{\n");
    for (int i = 0; i < node->block.stmts.count; i++) {
        gen_node(out, node->block.stmts.items[i]);
        fprintf(out, ";\n");
    }
    fprintf(out, "}\n");
}

static void gen_node(FILE *out, AstNode *node) {
    if (!node) return;
    switch (node->type) {
        case NODE_INT:    fprintf(out, "pico_int(%lld)", (long long)node->ival); break;
        case NODE_FLOAT:  fprintf(out, "pico_float(%g)", node->fval); break;
        case NODE_STRING: fprintf(out, "pico_str(\"%.*s\")", node->sval.len, node->sval.s); break;
        case NODE_IDENT:  fprintf(out, "%s", node->sval.s); break;
        
        case NODE_BINOP:
            fprintf(out, "(");
            gen_node(out, node->binop.left);
            fprintf(out, " %s ", token_type_name(node->binop.op));
            gen_node(out, node->binop.right);
            fprintf(out, ")");
            break;

        case NODE_LET:
            fprintf(out, "PicoValue %s = ", node->let.name);
            gen_node(out, node->let.value);
            break;

        case NODE_IF:
            fprintf(out, "if (pico_is_truthy(");
            gen_node(out, node->ifnode.cond);
            fprintf(out, ")) ");
            gen_node(out, node->ifnode.then);
            if (node->ifnode.els) {
                fprintf(out, " else ");
                gen_node(out, node->ifnode.els);
            }
            break;

        case NODE_BLOCK:
            gen_block(out, node);
            break;

        case NODE_FN:
            fprintf(out, "PicoValue %s(", node->fn.name ? node->fn.name : "anon");
            for (int i = 0; i < node->fn.param_count; i++) {
                if (i > 0) fprintf(out, ", ");
                fprintf(out, "PicoValue %s", node->fn.params[i]);
            }
            fprintf(out, ") ");
            gen_node(out, node->fn.body);
            break;

        case NODE_CALL:
            gen_node(out, node->call.callee);
            fprintf(out, "(");
            for (int i = 0; i < node->call.args.count; i++) {
                if (i > 0) fprintf(out, ", ");
                gen_node(out, node->call.args.items[i]);
            }
            fprintf(out, ")");
            break;

        case NODE_RETURN:
            fprintf(out, "return ");
            gen_node(out, node->retnode.value);
            break;

        default:
            fprintf(out, "/* TODO: node type %d */", node->type);
    }
}

void pico_codegen_c(AstNode *prog, const char *out_path) {
    FILE *out = fopen(out_path, "w");
    fprintf(out, "#include \"pico_runtime.h\"\n\n");
    
    if (prog->type == NODE_PROGRAM) {
        for (int i = 0; i < prog->program.stmts.count; i++) {
            gen_node(out, prog->program.stmts.items[i]);
            fprintf(out, ";\n");
        }
    }
    
    fclose(out);
}
