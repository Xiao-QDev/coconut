#define _POSIX_C_SOURCE 200809L
#include "vm.h"
#include "interpreter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    Chunk   *chunk;
    int      local_count;
    char    *locals[64];
    VM      *vm;
} Compiler;

static void emit(Compiler *c, uint8_t byte) { chunk_write(c->chunk, byte, 0); }
static void emit2(Compiler *c, uint8_t a, uint8_t b) { emit(c,a); emit(c,b); }

static int add_const(Compiler *c, Value v) { return chunk_add_const(c->chunk, v); }

static int resolve_local(Compiler *c, const char *name) {
    for (int i = c->local_count-1; i >= 0; i--)
        if (strcmp(c->locals[i], name) == 0) return i;
    return -1;
}

static int resolve_global(Compiler *c, const char *name) {
    return global_idx(c->vm, name);
}

static void compile_node(Compiler *c, AstNode *node);

static void compile_node(Compiler *c, AstNode *node) {
    if (!node) return;
    switch (node->type) {
    case NODE_INT:    emit2(c, OP_CONST, add_const(c, VAL_INT_V(node->ival))); break;
    case NODE_FLOAT:  emit2(c, OP_CONST, add_const(c, VAL_FLOAT_V(node->fval))); break;
    case NODE_STRING: emit2(c, OP_CONST, add_const(c, VAL_STR_V(str_intern(node->sval.s, node->sval.len)))); break;
    case NODE_BOOL:   emit(c, node->bval ? OP_TRUE : OP_FALSE); break;
    case NODE_NIL:    emit(c, OP_NIL); break;

    case NODE_IDENT: {
        int loc = resolve_local(c, node->sval.s);
        if (loc >= 0) emit2(c, OP_GET_LOCAL, loc);
        else emit2(c, OP_GET_GLOBAL, resolve_global(c, node->sval.s));
        break;
    }
    case NODE_LET: {
        compile_node(c, node->let.value);
        int loc = c->local_count;
        c->locals[c->local_count++] = node->let.name;
        emit2(c, OP_SET_LOCAL, loc);
        break;
    }
    case NODE_ASSIGN: {
        compile_node(c, node->assign.value);
        if (node->assign.name) {
            int loc = resolve_local(c, node->assign.name);
            if (loc >= 0) emit2(c, OP_SET_LOCAL, loc);
            else emit2(c, OP_SET_GLOBAL, resolve_global(c, node->assign.name));
        }
        break;
    }
    case NODE_BINOP: {
        compile_node(c, node->binop.left);
        compile_node(c, node->binop.right);
        switch (node->binop.op) {
        case TOK_PLUS:    emit(c, OP_ADD); break;
        case TOK_MINUS:   emit(c, OP_SUB); break;
        case TOK_STAR:    emit(c, OP_MUL); break;
        case TOK_SLASH:   emit(c, OP_DIV); break;
        case TOK_PERCENT: emit(c, OP_MOD); break;
        case TOK_EQ:      emit(c, OP_EQ);  break;
        case TOK_NEQ:     emit(c, OP_NEQ); break;
        case TOK_LT:      emit(c, OP_LT);  break;
        case TOK_LE:      emit(c, OP_LE);  break;
        case TOK_GT:      emit(c, OP_GT);  break;
        case TOK_GE:      emit(c, OP_GE);  break;
        default: break;
        }
        break;
    }
    case NODE_UNOP:
        compile_node(c, node->unop.operand);
        if (node->unop.op == TOK_MINUS) emit(c, OP_NEG);
        else if (node->unop.op == TOK_NOT) emit(c, OP_NOT);
        break;

    case NODE_IF: {
        compile_node(c, node->ifnode.cond);
        // emit JUMP_IF_FALSE placeholder
        emit(c, OP_JUMP_IF_FALSE);
        int patch = c->chunk->count;
        emit2(c, 0, 0);
        emit(c, OP_POP);
        compile_node(c, node->ifnode.then);
        int else_patch = -1;
        if (node->ifnode.els) {
            emit(c, OP_JUMP);
            else_patch = c->chunk->count;
            emit2(c, 0, 0);
        }
        int off = c->chunk->count - patch - 2;
        c->chunk->code[patch]   = (off >> 8) & 0xff;
        c->chunk->code[patch+1] = off & 0xff;
        emit(c, OP_POP);
        if (node->ifnode.els) {
            compile_node(c, node->ifnode.els);
            int off2 = c->chunk->count - else_patch - 2;
            c->chunk->code[else_patch]   = (off2 >> 8) & 0xff;
            c->chunk->code[else_patch+1] = off2 & 0xff;
        }
        break;
    }
    case NODE_WHILE: {
        int loop_start = c->chunk->count;
        compile_node(c, node->whilenode.cond);
        emit(c, OP_JUMP_IF_FALSE);
        int patch = c->chunk->count; emit2(c, 0, 0);
        emit(c, OP_POP);
        compile_node(c, node->whilenode.body);
        // loop back
        emit(c, OP_LOOP);
        int back = c->chunk->count - loop_start + 2;
        emit2(c, (back>>8)&0xff, back&0xff);
        int off = c->chunk->count - patch - 2;
        c->chunk->code[patch]   = (off>>8)&0xff;
        c->chunk->code[patch+1] = off&0xff;
        emit(c, OP_POP);
        break;
    }
    case NODE_BLOCK:
        for (int i = 0; i < node->block.stmts.count; i++)
            compile_node(c, node->block.stmts.items[i]);
        break;
    case NODE_PROGRAM:
        for (int i = 0; i < node->program.stmts.count; i++)
            compile_node(c, node->program.stmts.items[i]);
        break;
    case NODE_EXPR_STMT:
        compile_node(c, node->exprstmt.expr);
        emit(c, OP_POP);
        break;
    case NODE_RETURN:
        if (node->retnode.value) compile_node(c, node->retnode.value);
        else emit(c, OP_NIL);
        emit(c, OP_RETURN);
        break;
    case NODE_LIST: {
        for (int i = 0; i < node->list.elements.count; i++)
            compile_node(c, node->list.elements.items[i]);
        emit2(c, OP_MAKE_LIST, node->list.elements.count);
        break;
    }
    case NODE_MAP: {
        for (int i = 0; i < node->map.keys.count; i++) {
            compile_node(c, node->map.keys.items[i]);
            compile_node(c, node->map.vals.items[i]);
        }
        emit2(c, OP_MAKE_MAP, node->map.keys.count);
        break;
    }
    case NODE_INDEX:
        compile_node(c, node->index.obj);
        compile_node(c, node->index.idx);
        emit(c, OP_INDEX);
        break;
    case NODE_FIELD_ACCESS: {
        compile_node(c, node->field.obj);
        int ci = add_const(c, VAL_STR_V(str_intern(node->field.field, (int)strlen(node->field.field))));
        emit2(c, OP_GET_FIELD, ci);
        break;
    }
    case NODE_CALL: {
        compile_node(c, node->call.callee);
        for (int i = 0; i < node->call.args.count; i++)
            compile_node(c, node->call.args.items[i]);
        emit2(c, OP_CALL, node->call.args.count);
        break;
    }
    default: break;
    }
}

ObjChunk *compile(AstNode *prog) {
    ObjChunk *oc = gc_alloc(sizeof(ObjChunk));
    oc->hdr.type = OBJ_FN;
    oc->hdr.marked = false;
    oc->hdr.next = gc_objects;
    gc_objects = (Obj*)oc;
    chunk_init(&oc->chunk);
    oc->arity = 0;
    oc->name  = "main";

    VM dummy_vm; vm_init(&dummy_vm);
    Compiler c = { .chunk = &oc->chunk, .local_count = 0, .vm = &dummy_vm };
    compile_node(&c, prog);
    emit(&c, OP_RETURN);
    return oc;
}
