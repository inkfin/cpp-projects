#ifndef ARENA_H
#define ARENA_H

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
    MAXNUM,
} EARENA_ERR;

#define ARENA_ALLOC_MEMORY_BIT (1u<<1)

typedef struct {
    byte_t* base;
    size_t cap;
    size_t offset;

    uint32_t flags;
    uint32_t high_water; //< peak
} Arena;

typedef size_t ArenaMark;

#define ALIGN_UP(offset, align) (offset + align)

EARENA_ERR arena_init_with_buffer(Arena* arena, void* buf, size_t cap);
EARENA_ERR arena_malloc(size_t size);
EARENA_ERR arena_destroy(size_t size);

#endif // ARENA_H
