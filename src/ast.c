#include "ast.h"
#include <stdlib.h>
#include <string.h>

#define ARENA_BLOCK_SIZE (64 * 1024)

void arena_init(Arena *a) { a->head = NULL; }

void *arena_alloc(Arena *a, size_t size) {
    size = (size + 7) & ~7;  // 8字节对齐
    if (!a->head || a->head->used + size > a->head->cap) {
        size_t cap = size > ARENA_BLOCK_SIZE ? size : ARENA_BLOCK_SIZE;
        ArenaBlock *b = malloc(sizeof(ArenaBlock) + cap);
        b->data = (char*)(b + 1);
        b->used = 0;
        b->cap  = cap;
        b->next = a->head;
        a->head = b;
    }
    void *p = a->head->data + a->head->used;
    a->head->used += size;
    return p;
}

char *arena_strdup(Arena *a, const char *s, int len) {
    char *p = arena_alloc(a, len + 1);
    memcpy(p, s, len);
    p[len] = '\0';
    return p;
}

void arena_free(Arena *a) {
    ArenaBlock *b = a->head;
    while (b) { ArenaBlock *n = b->next; free(b); b = n; }
    a->head = NULL;
}

AstNode *node_new(Arena *a, NodeType type, int line, int col) {
    AstNode *n = arena_alloc(a, sizeof(AstNode));
    memset(n, 0, sizeof(AstNode));
    n->type = type;
    n->line = line;
    n->col  = col;
    return n;
}

void nodelist_push(Arena *a, NodeList *nl, AstNode *n) {
    if (nl->count >= nl->cap) {
        int newcap = nl->cap ? nl->cap * 2 : 4;
        AstNode **items = arena_alloc(a, newcap * sizeof(AstNode*));
        if (nl->items) memcpy(items, nl->items, nl->count * sizeof(AstNode*));
        nl->items = items;
        nl->cap   = newcap;
    }
    nl->items[nl->count++] = n;
}
