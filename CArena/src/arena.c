#include "arena.h"
#include <stdlib.h>

EARENA_ERR arena_init_with_buffer(Arena* arena, void* buf, size_t cap) {
    if (!arena || !arena->base) return EARENA_FAILED;

    // set flags
    arena->flags = 0;
    arena->base = NULL;

    return EARENA_SUCCESS;
}
