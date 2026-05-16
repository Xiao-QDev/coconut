#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ast.h"
#include "lexer.h"

static void gen_expr(FILE *out, AstNode *node);
static void gen_stmt(FILE *out, AstNode *node, int indent);

static void ind(FILE *out, int n) { for(int i=0;i<n;i++) fprintf(out,"    "); }

static void gen_expr(FILE *out, AstNode *node) {
    if (!node) { fprintf(out,"PICO_NIL_V"); return; }
    switch (node->type) {
    case NODE_INT:    fprintf(out,"pico_int(%lldLL)",(long long)node->ival); break;
    case NODE_FLOAT:  fprintf(out,"pico_float(%g)",node->fval); break;
    case NODE_BOOL:   fprintf(out,"pico_bool(%s)",node->bval?"true":"false"); break;
    case NODE_NIL:    fprintf(out,"PICO_NIL_V"); break;
    case NODE_STRING: {
        fprintf(out,"pico_str(\"");
        for(int i=0;i<node->sval.len;i++){
            char c=node->sval.s[i];
            if(c=='"') fprintf(out,"\\\"");
            else if(c=='\\') fprintf(out,"\\\\");
            else if(c=='\n') fprintf(out,"\\n");
            else fputc(c,out);
        }
        fprintf(out,"\")");
        break;
    }
    case NODE_IDENT:
        // mangle: prefix with _p_ to avoid C keyword conflicts
        fprintf(out,"_p_%s",node->sval.s);
        break;
    case NODE_BINOP: {
        PicoTokenType op = node->binop.op;
        const char *fn = NULL;
        switch(op){
        case TOK_PLUS:    fn="pico_add"; break;
        case TOK_MINUS:   fn="pico_sub"; break;
        case TOK_STAR:    fn="pico_mul"; break;
        case TOK_SLASH:   fn="pico_div"; break;
        case TOK_PERCENT: fn="pico_mod"; break;
        case TOK_EQ:      fn="pico_eq";  break;
        default: break;
        }
        if (fn) {
            fprintf(out,"%s(",fn);
            gen_expr(out,node->binop.left);
            fprintf(out,",");
            gen_expr(out,node->binop.right);
            fprintf(out,")");
        } else {
            // comparison: emit as pico_bool(...)
            fprintf(out,"pico_bool((");
            gen_expr(out,node->binop.left);
            fprintf(out,").i ");
            switch(op){
            case TOK_LT: fprintf(out,"<"); break;
            case TOK_LE: fprintf(out,"<="); break;
            case TOK_GT: fprintf(out,">"); break;
            case TOK_GE: fprintf(out,">="); break;
            case TOK_NEQ: fprintf(out,"!="); break;
            default: fprintf(out,"=="); break;
            }
            fprintf(out," (");
            gen_expr(out,node->binop.right);
            fprintf(out,").i)");
        }
        break;
    }
    case NODE_UNOP:
        if(node->unop.op==TOK_MINUS){
            fprintf(out,"pico_int(-(");
            gen_expr(out,node->unop.operand);
            fprintf(out,").i)");
        } else {
            fprintf(out,"pico_bool(!pico_is_truthy(");
            gen_expr(out,node->unop.operand);
            fprintf(out,"))");
        }
        break;
    case NODE_CALL:
        gen_expr(out,node->call.callee);
        fprintf(out,"(");
        for(int i=0;i<node->call.args.count;i++){
            if(i) fprintf(out,",");
            gen_expr(out,node->call.args.items[i]);
        }
        fprintf(out,")");
        break;
    case NODE_FIELD_ACCESS:
        gen_expr(out,node->field.obj);
        fprintf(out,"/* .%s */",node->field.field);
        break;
    default:
        fprintf(out,"PICO_NIL_V /* expr %d */",node->type);
    }
}

static void gen_stmt(FILE *out, AstNode *node, int d) {
    if (!node) return;
    switch (node->type) {
    case NODE_LET:
        ind(out,d);
        fprintf(out,"PicoVal _p_%s = ",node->let.name);
        gen_expr(out,node->let.value);
        fprintf(out,";\n");
        break;
    case NODE_ASSIGN:
        ind(out,d);
        if(node->assign.name) fprintf(out,"_p_%s = ",node->assign.name);
        gen_expr(out,node->assign.value);
        fprintf(out,";\n");
        break;
    case NODE_EXPR_STMT:
        ind(out,d);
        gen_expr(out,node->exprstmt.expr);
        fprintf(out,";\n");
        break;
    case NODE_IF:
        ind(out,d);
        fprintf(out,"if(pico_is_truthy(");
        gen_expr(out,node->ifnode.cond);
        fprintf(out,")){\n");
        gen_stmt(out,node->ifnode.then,d+1);
        ind(out,d); fprintf(out,"}");
        if(node->ifnode.els){
            fprintf(out," else {\n");
            gen_stmt(out,node->ifnode.els,d+1);
            ind(out,d); fprintf(out,"}");
        }
        fprintf(out,"\n");
        break;
    case NODE_WHILE:
        ind(out,d);
        fprintf(out,"while(pico_is_truthy(");
        gen_expr(out,node->whilenode.cond);
        fprintf(out,")){\n");
        gen_stmt(out,node->whilenode.body,d+1);
        ind(out,d); fprintf(out,"}\n");
        break;
    case NODE_FOR: {
        // for var in list: emit C for loop over PicoList
        ind(out,d);
        fprintf(out,"{\n");
        ind(out,d+1);
        fprintf(out,"PicoVal _iter_%s = ",node->fornode.var);
        gen_expr(out,node->fornode.iter);
        fprintf(out,";\n");
        ind(out,d+1);
        fprintf(out,"for(int _i_%s=0;_i_%s<_iter_%s.l->len;_i_%s++){\n",
            node->fornode.var,node->fornode.var,node->fornode.var,node->fornode.var);
        ind(out,d+2);
        fprintf(out,"PicoVal _p_%s=_iter_%s.l->items[_i_%s];\n",
            node->fornode.var,node->fornode.var,node->fornode.var);
        gen_stmt(out,node->fornode.body,d+2);
        ind(out,d+1); fprintf(out,"}\n");
        ind(out,d); fprintf(out,"}\n");
        break;
    }
    case NODE_BLOCK:
        for(int i=0;i<node->block.stmts.count;i++)
            gen_stmt(out,node->block.stmts.items[i],d);
        break;
    case NODE_RETURN:
        ind(out,d);
        fprintf(out,"return ");
        if(node->retnode.value) gen_expr(out,node->retnode.value);
        else fprintf(out,"PICO_NIL_V");
        fprintf(out,";\n");
        break;
    case NODE_FN: {
        ind(out,d);
        fprintf(out,"PicoVal _p_%s(",node->fn.name?node->fn.name:"_anon");
        for(int i=0;i<node->fn.param_count;i++){
            if(i) fprintf(out,",");
            fprintf(out,"PicoVal _p_%s",node->fn.params[i]);
        }
        fprintf(out,"){\n");
        gen_stmt(out,node->fn.body,d+1);
        ind(out,d); fprintf(out,"return PICO_NIL_V;\n");
        ind(out,d); fprintf(out,"}\n");
        break;
    }
    case NODE_PROGRAM:
        for(int i=0;i<node->program.stmts.count;i++)
            gen_stmt(out,node->program.stmts.items[i],d);
        break;
    default:
        ind(out,d);
        fprintf(out,"/* stmt %d */\n",node->type);
    }
}

void pico_codegen_c(AstNode *prog, const char *out_path) {
    FILE *out = fopen(out_path,"w");
    if(!out){ fprintf(stderr,"无法写入 %s\n",out_path); return; }
    fprintf(out,"#include \"pico_runtime.h\"\n\n");
    // forward-declare all top-level functions
    if(prog->type==NODE_PROGRAM){
        for(int i=0;i<prog->program.stmts.count;i++){
            AstNode *s=prog->program.stmts.items[i];
            if(s->type==NODE_FN && s->fn.name){
                fprintf(out,"PicoVal _p_%s(",s->fn.name);
                for(int j=0;j<s->fn.param_count;j++){
                    if(j) fprintf(out,",");
                    fprintf(out,"PicoVal _p_%s",s->fn.params[j]);
                }
                fprintf(out,");\n");
            }
        }
        fprintf(out,"\n");
    }
    gen_stmt(out,prog,0);
    fprintf(out,"\nint main(void){\n    _pico_main();\n    return 0;\n}\n");
    fclose(out);
}
