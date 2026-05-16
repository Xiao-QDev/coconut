#pragma once
#include "lexer.h"
#include "value.h"

typedef enum {
    // 字面量
    NODE_INT, NODE_FLOAT, NODE_STRING, NODE_BOOL, NODE_NIL, NODE_FSTRING,
    // 变量
    NODE_IDENT, NODE_ASSIGN,
    // 运算
    NODE_BINOP, NODE_UNOP,
    // 集合
    NODE_LIST, NODE_MAP, NODE_INDEX, NODE_SLICE,
    // 控制流
    NODE_IF, NODE_WHILE, NODE_FOR, NODE_MATCH,
    NODE_RETURN, NODE_YIELD, NODE_BREAK, NODE_CONTINUE,
    // 函数
    NODE_FN, NODE_CALL, NODE_METHOD_CALL,
    // 结构体
    NODE_STRUCT_DEF, NODE_STRUCT_LIT, NODE_FIELD_ACCESS,
    // 模块
    NODE_IMPORT,
    // 异步/线程
    NODE_ASYNC_FN, NODE_AWAIT, NODE_SPAWN,
    // 错误处理
    NODE_TRY,
    // 语句块
    NODE_BLOCK, NODE_EXPR_STMT, NODE_LET,
    // 程序
    NODE_PROGRAM
} NodeType;

typedef struct AstNode AstNode;

typedef struct {
    AstNode **items;
    int       count;
    int       cap;
} NodeList;

struct AstNode {
    NodeType type;
    int      line;
    int      col;
    union {
        int64_t  ival;
        double   fval;
        bool     bval;
        struct { char *s; int len; }  sval;   // STRING / IDENT
        struct { NodeList parts; }    fstr;   // FSTRING: alternating str/expr
        struct { AstNode *left; AstNode *right; PicoTokenType op; } binop;
        struct { AstNode *operand; PicoTokenType op; }              unop;
        struct { char *name; AstNode *value; }                  let;
        struct { char *name; AstNode *value; PicoTokenType op; }    assign;
        struct { NodeList elements; }                           list;
        struct { NodeList keys; NodeList vals; }                map;
        struct { AstNode *obj; AstNode *idx; }                  index;
        struct { AstNode *obj; AstNode *lo; AstNode *hi; }      slice;
        struct { AstNode *cond; AstNode *then; AstNode *els; }  ifnode;
        struct { AstNode *cond; AstNode *body; }                whilenode;
        struct { char *var; AstNode *iter; AstNode *body; }     fornode;
        struct {
            AstNode *subject;
            NodeList patterns;
            NodeList bodies;
            AstNode *default_body;
        } matchnode;
        struct { AstNode *value; }  retnode;
        struct { AstNode *value; }  yieldnode;
        struct { AstNode *value; }  awaitnode;
        struct { AstNode *expr; }   spawnnode;
        struct {
            char    *name;
            char   **params;
            int      param_count;
            AstNode *body;
            bool     is_generator;
            bool     is_async;
        } fn;
        struct { AstNode *callee; NodeList args; }              call;
        struct { AstNode *obj; char *method; NodeList args; }   mcall;
        struct { AstNode *obj; char *field; }                   field;
        struct {
            char    *name;
            char    *parent_name;  // 继承：struct 子(父):
            char   **field_names;
            int      field_count;
            NodeList methods;
        } structdef;
        struct { char *name; NodeList keys; NodeList vals; }    structlit;
        struct { char **names; int count; }                     import;
        struct {
            AstNode *body;
            AstNode *catch_var;
            AstNode *catch_body;
        } trynode;
        struct { NodeList stmts; }  block;
        struct { AstNode *expr; }   exprstmt;
        struct { NodeList stmts; }  program;
    };
};

// ── Arena 分配器 ─────────────────────────────────────────────
typedef struct ArenaBlock {
    char             *data;
    size_t            used;
    size_t            cap;
    struct ArenaBlock *next;
} ArenaBlock;

typedef struct {
    ArenaBlock *head;
} Arena;

void     arena_init(Arena *a);
void    *arena_alloc(Arena *a, size_t size);
char    *arena_strdup(Arena *a, const char *s, int len);
void     arena_free(Arena *a);

AstNode *node_new(Arena *a, NodeType type, int line, int col);
void     nodelist_push(Arena *a, NodeList *nl, AstNode *n);
