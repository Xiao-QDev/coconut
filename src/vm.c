#define _POSIX_C_SOURCE 200809L
#include "vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

void chunk_init(Chunk *c) { memset(c, 0, sizeof(Chunk)); }

void chunk_free(Chunk *c) {
    free(c->code); free(c->constants); free(c->lines);
    free(c->local_names);
}

void chunk_write(Chunk *c, uint8_t byte, int line) {
    if (c->count >= c->cap) {
        c->cap = c->cap ? c->cap * 2 : 8;
        c->code  = realloc(c->code,  c->cap);
        c->lines = realloc(c->lines, c->cap * sizeof(int));
    }
    c->code[c->count]  = byte;
    c->lines[c->count] = line;
    c->count++;
}

int chunk_add_const(Chunk *c, Value v) {
    if (c->const_count >= c->const_cap) {
        c->const_cap = c->const_cap ? c->const_cap * 2 : 8;
        c->constants = realloc(c->constants, c->const_cap * sizeof(Value));
    }
    c->constants[c->const_count++] = v;
    return c->const_count - 1;
}

// ── VM ───────────────────────────────────────────────────────

void vm_init(VM *vm) {
    memset(vm, 0, sizeof(VM));
    vm->global_cap = 64;
    vm->globals      = malloc(vm->global_cap * sizeof(Value));
    vm->global_names = malloc(vm->global_cap * sizeof(char*));
}

static void push(VM *vm, Value v) { vm->stack[vm->sp++] = v; }
static Value pop(VM *vm)          { return vm->stack[--vm->sp]; }
static Value peek(VM *vm, int d)  { return vm->stack[vm->sp - 1 - d]; }

int global_idx(VM *vm, const char *name) {
    for (int i = 0; i < vm->global_count; i++)
        if (strcmp(vm->global_names[i], name) == 0) return i;
    if (vm->global_count >= vm->global_cap) {
        vm->global_cap *= 2;
        vm->globals      = realloc(vm->globals,      vm->global_cap * sizeof(Value));
        vm->global_names = realloc(vm->global_names, vm->global_cap * sizeof(char*));
    }
    vm->global_names[vm->global_count] = strdup(name);
    vm->globals[vm->global_count]      = VAL_NIL_V;
    return vm->global_count++;
}

#define FRAME()      (&vm->frames[vm->frame_count - 1])
#define READ_BYTE()  (*FRAME()->ip++)
#define READ_SHORT() (FRAME()->ip += 2, (uint16_t)((FRAME()->ip[-2] << 8) | FRAME()->ip[-1]))
#define READ_CONST() (FRAME()->chunk->chunk.constants[READ_BYTE()])

static void push_frame(VM *vm, ObjChunk *chunk, int slot_base) {
    CallFrame *f = &vm->frames[vm->frame_count++];
    f->chunk      = chunk;
    f->ip         = chunk->chunk.code;
    f->slots      = vm->stack + slot_base;
    f->slot_base  = slot_base;
}

Value vm_run(VM *vm, ObjChunk *chunk) {
    vm->chunk = chunk;
    vm->ip    = chunk->chunk.code;
    push_frame(vm, chunk, 0);

    for (;;) {
        uint8_t op = READ_BYTE();
        switch (op) {
        case OP_CONST:   push(vm, READ_CONST()); break;
        case OP_NIL:     push(vm, VAL_NIL_V); break;
        case OP_TRUE:    push(vm, VAL_BOOL_V(true)); break;
        case OP_FALSE:   push(vm, VAL_BOOL_V(false)); break;
        case OP_POP:     pop(vm); break;

        case OP_GET_LOCAL:  push(vm, FRAME()->slots[READ_BYTE()]); break;
        case OP_SET_LOCAL:  FRAME()->slots[READ_BYTE()] = peek(vm, 0); break;
        case OP_GET_GLOBAL: { int i = READ_BYTE(); push(vm, vm->globals[i]); break; }
        case OP_SET_GLOBAL: { int i = READ_BYTE(); vm->globals[i] = peek(vm, 0); break; }

        case OP_ADD: {
            Value b = pop(vm), a = pop(vm);
            if (IS_STR(a) && IS_STR(b)) push(vm, VAL_STR_V(str_concat(a.string, b.string)));
            else if (IS_INT(a) && IS_INT(b)) push(vm, VAL_INT_V(a.integer + b.integer));
            else push(vm, VAL_FLOAT_V((IS_INT(a)?a.integer:a.floating)+(IS_INT(b)?b.integer:b.floating)));
            break;
        }
        case OP_SUB: { Value b=pop(vm),a=pop(vm); push(vm,IS_INT(a)&&IS_INT(b)?VAL_INT_V(a.integer-b.integer):VAL_FLOAT_V((IS_INT(a)?a.integer:a.floating)-(IS_INT(b)?b.integer:b.floating))); break; }
        case OP_MUL: { Value b=pop(vm),a=pop(vm); push(vm,IS_INT(a)&&IS_INT(b)?VAL_INT_V(a.integer*b.integer):VAL_FLOAT_V((IS_INT(a)?a.integer:a.floating)*(IS_INT(b)?b.integer:b.floating))); break; }
        case OP_DIV: { Value b=pop(vm),a=pop(vm); push(vm,VAL_FLOAT_V((IS_INT(a)?a.integer:a.floating)/(IS_INT(b)?b.integer:b.floating))); break; }
        case OP_MOD: { Value b=pop(vm),a=pop(vm); push(vm,IS_INT(a)&&IS_INT(b)?VAL_INT_V(a.integer%b.integer):VAL_FLOAT_V(fmod(IS_INT(a)?a.integer:a.floating,IS_INT(b)?b.integer:b.floating))); break; }
        case OP_NEG: { Value a=pop(vm); push(vm,IS_INT(a)?VAL_INT_V(-a.integer):VAL_FLOAT_V(-a.floating)); break; }

        case OP_EQ:  { Value b=pop(vm),a=pop(vm); push(vm,VAL_BOOL_V(value_equal(a,b))); break; }
        case OP_NEQ: { Value b=pop(vm),a=pop(vm); push(vm,VAL_BOOL_V(!value_equal(a,b))); break; }
        case OP_LT:  { Value b=pop(vm),a=pop(vm); push(vm,VAL_BOOL_V((IS_INT(a)?a.integer:a.floating)<(IS_INT(b)?b.integer:b.floating))); break; }
        case OP_LE:  { Value b=pop(vm),a=pop(vm); push(vm,VAL_BOOL_V((IS_INT(a)?a.integer:a.floating)<=(IS_INT(b)?b.integer:b.floating))); break; }
        case OP_GT:  { Value b=pop(vm),a=pop(vm); push(vm,VAL_BOOL_V((IS_INT(a)?a.integer:a.floating)>(IS_INT(b)?b.integer:b.floating))); break; }
        case OP_GE:  { Value b=pop(vm),a=pop(vm); push(vm,VAL_BOOL_V((IS_INT(a)?a.integer:a.floating)>=(IS_INT(b)?b.integer:b.floating))); break; }
        case OP_NOT: { Value a=pop(vm); push(vm,VAL_BOOL_V(!IS_TRUTHY(a))); break; }

        case OP_JUMP: { uint16_t off=READ_SHORT(); vm->ip+=off; break; }
        case OP_JUMP_IF_FALSE: { uint16_t off=READ_SHORT(); if(!IS_TRUTHY(peek(vm,0))) vm->ip+=off; break; }
        case OP_LOOP: { uint16_t off=READ_SHORT(); vm->ip-=off; break; }

        case OP_MAKE_LIST: {
            int n = READ_BYTE();
            ObjList *l = list_new();
            for (int i = n-1; i >= 0; i--) l->items[i] = pop(vm); // pre-alloc needed
            // simpler: push in order
            l->len = 0;
            // redo: collect from stack
            Value tmp[64]; for(int i=0;i<n;i++) tmp[n-1-i]=pop(vm);
            for(int i=0;i<n;i++) list_push(l, tmp[i]);
            push(vm, VAL_LIST_V(l));
            break;
        }
        case OP_MAKE_MAP: {
            int n = READ_BYTE(); // n key-value pairs
            ObjMap *m = map_new();
            Value tmp[128]; for(int i=0;i<n*2;i++) tmp[n*2-1-i]=pop(vm);
            for(int i=0;i<n;i++) map_set(m, tmp[i*2].string, tmp[i*2+1]);
            push(vm, VAL_MAP_V(m));
            break;
        }
        case OP_INDEX: {
            Value idx=pop(vm), obj=pop(vm);
            if(IS_LIST(obj)&&IS_INT(idx)) push(vm,list_get(obj.list,(int)idx.integer));
            else if(IS_MAP(obj)&&IS_STR(idx)){ Value v; map_get(obj.map,idx.string,&v); push(vm,v); }
            else push(vm,VAL_NIL_V);
            break;
        }
        case OP_GET_FIELD: {
            Value key=READ_CONST(), obj=pop(vm);
            if(obj.type==VAL_INSTANCE){
                for(int i=0;i<obj.instance->def->field_count;i++)
                    if(obj.instance->def->fields[i].name==key.string){ push(vm,obj.instance->fields[i]); goto next; }
                Value m; if(map_get(obj.instance->def->methods,key.string,&m)){ push(vm,m); goto next; }
            } else if(IS_MAP(obj)){ Value v; map_get(obj.map,key.string,&v); push(vm,v); goto next; }
            push(vm,VAL_NIL_V);
            goto next;
        }
        case OP_PRINT: { value_println(pop(vm)); break; }

        case OP_CALL: {
            int argc = READ_BYTE();
            Value callee = vm->stack[vm->sp - argc - 1];
            if (callee.type == VAL_NATIVE) {
                Value *argv = &vm->stack[vm->sp - argc];
                Value res = callee.native(argc, argv);
                vm->sp -= argc + 1;
                push(vm, res);
            } else if (callee.type == VAL_FN) {
                ObjFn *fn = callee.fn;
                if (fn->vm_chunk && vm->frame_count < 64) {
                    int slot_base = vm->sp - argc;
                    push_frame(vm, (ObjChunk*)fn->vm_chunk, slot_base);
                } else {
                    push(vm, VAL_NIL_V);
                }
            }
            break;
        }
        case OP_RETURN: {
            Value ret = vm->sp > 0 ? pop(vm) : VAL_NIL_V;
            vm->frame_count--;
            if (vm->frame_count == 0) return ret;
            // restore sp to before callee + args
            vm->sp = FRAME()->slot_base - 1; // -1 for the callee slot
            push(vm, ret);
            break;
        }
        default: break;
        }
        next:;
    }
}
