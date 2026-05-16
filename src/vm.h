#pragma once
#include "value.h"
#include "ast.h"
#include <stdint.h>

typedef enum {
    OP_CONST,      // push constants[arg]
    OP_NIL,        // push nil
    OP_TRUE,       // push true
    OP_FALSE,      // push false
    OP_POP,        // pop top
    OP_GET_LOCAL,  // push locals[arg]
    OP_SET_LOCAL,  // locals[arg] = top
    OP_GET_GLOBAL, // push globals[arg]
    OP_SET_GLOBAL, // globals[arg] = top
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_NEG,
    OP_EQ, OP_NEQ, OP_LT, OP_LE, OP_GT, OP_GE,
    OP_AND, OP_OR, OP_NOT,
    OP_CONCAT,     // string +
    OP_JUMP,       // ip += arg
    OP_JUMP_IF_FALSE, // if !top: ip += arg
    OP_LOOP,       // ip -= arg
    OP_CALL,       // call top with arg args
    OP_RETURN,
    OP_MAKE_LIST,  // pop arg items, push list
    OP_MAKE_MAP,   // pop arg*2 items, push map
    OP_INDEX,      // obj[idx]
    OP_SET_INDEX,  // obj[idx] = val
    OP_GET_FIELD,  // obj.field (field name in constants[arg])
    OP_SET_FIELD,
    OP_MAKE_FN,    // push closure wrapping chunk[arg]
    OP_PRINT,      // print top
} OpCode;

typedef struct {
    uint8_t  *code;
    int       count;
    int       cap;
    Value    *constants;
    int       const_count;
    int       const_cap;
    int      *lines;
    char    **local_names;
    int       local_count;
} Chunk;

typedef struct ObjChunk {
    Obj    hdr;
    Chunk  chunk;
    int    arity;
    char  *name;
} ObjChunk;

void   chunk_init(Chunk *c);
void   chunk_free(Chunk *c);
void   chunk_write(Chunk *c, uint8_t byte, int line);
int    chunk_add_const(Chunk *c, Value v);

// Compiler: AST -> Chunk
ObjChunk *compile(AstNode *prog);

// VM
typedef struct {
    ObjChunk  *chunk;
    uint8_t   *ip;
    Value      stack[256];
    int        sp;
    Value     *globals;
    int        global_count;
    char     **global_names;
    int        global_cap;
} VM;

void  vm_init(VM *vm);
int   global_idx(VM *vm, const char *name);
Value vm_run(VM *vm, ObjChunk *chunk);
