#ifndef ARENA_H
#define ARENA_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

// ==== MEMORY MANAGEMENT
// Here are the memory macro definitions, which users can override with their
// own implementions

typedef unsigned char byte_t;

typedef enum {
    EARENA_SUCCESS = 0,
    EARENA_FAILED,
    EARENA_OOM,
    EARENA_INVALID_PARAM,
    EARENA_INVARIANT_VIOLATION,
    EARENA_MAXNUM,
} ARENA_ERR;

#define ARENA_ALLOC_MEMORY_BIT (1u<<1)

#define ALIGN_UP(offset, align) (offset + align)

typedef struct {
    byte_t* base;
    size_t cap;
    size_t offset;

    uint32_t flags;
    uint32_t high_water; ///< peak
} Arena;

typedef size_t ArenaMark;

static inline ArenaMark arena_mark(Arena* a) { return a->offset; }

// debugging tools
void      arena_err_fprint(FILE* stream, ARENA_ERR);

ARENA_ERR arena_init_with_buffer(Arena* arena, void* buf, size_t cap);
ARENA_ERR arena_malloc(Arena* arena, ArenaMark* mark, size_t size);
ARENA_ERR arena_reset_to(Arena* arena, ArenaMark mark);

#endif // ARENA_H
