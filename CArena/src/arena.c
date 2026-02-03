#include "arena.h"
#include <stdlib.h>

static const char* arena__get_err_str(ARENA_ERR err) {
    const char* err_msg;

    switch(err) {
    case EARENA_SUCCESS:
        err_msg = "Success";
        break;
    case EARENA_FAILED:
        err_msg = "Failed";
        break;
    case EARENA_OOM:
        err_msg = "Failed because of not enough memory";
        break;
    case EARENA_INVALID_PARAM:
        err_msg = "Failed because of invalid parameters";
        break;
    case EARENA_INVARIANT_VIOLATION:
        err_msg = "Failed because of arena state corrupted";
        break;
    default:
        err_msg = "Unknown error";
    }

    return err_msg;
}

void arena_err_fprint(FILE* stream, ARENA_ERR err) {
    fprintf(stream, "[Arena] %s", arena__get_err_str(err));
}

ARENA_ERR arena_init_with_buffer(Arena* arena, void* buf, size_t cap) {
    if (!arena
     || !buf
     || cap <= 0) return EARENA_INVALID_PARAM;

    arena->base = buf;
    arena->offset = 0;
    arena->cap = cap;

    arena->flags = 0;
    arena->high_water = 0;

    return EARENA_SUCCESS;
}

ARENA_ERR arena_malloc(Arena* arena, ArenaMark* mark, size_t size) {
    if (!arena || !mark) return EARENA_INVALID_PARAM;
#ifndef NDEBUG
    if (arena->cap < arena->offset) return EARENA_INVARIANT_VIOLATION;
#endif

    // overflow check
    if (size > arena->cap - arena->offset) return EARENA_OOM;

    // allocate
    *mark = arena->offset;
    arena->offset += size;

    return EARENA_SUCCESS;
}

ARENA_ERR arena_reset_to(Arena* arena, ArenaMark mark) {
    if (!arena) return EARENA_INVALID_PARAM;
#ifndef NDEBUG
    if (arena->offset > arena->cap) return EARENA_INVARIANT_VIOLATION;
#endif

    if (mark > arena->offset) return EARENA_FAILED;

    arena->offset = mark;

    return EARENA_SUCCESS;
}

