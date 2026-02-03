#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "arena.h"

int main() {
    enum { KB = 1u << 10, MB = 1u << 20 };
    void* buffer = malloc(1 * MB);  // 1MB

    Arena arena = {0};
    ArenaMark mark[10] = {0};
    ARENA_ERR err;

    err = arena_init_with_buffer(&arena, buffer, 1 * MB);
    assert(err == EARENA_SUCCESS);
    assert(arena.cap == 1 * MB);
    assert(arena.offset == 0);

    err = arena_malloc(&arena, &mark[0], 0);
    assert(err == EARENA_SUCCESS && mark[0] == 0);
    assert(arena.offset == 0);

    err = arena_malloc(&arena, &mark[1], 1 * KB);
    assert(err == EARENA_SUCCESS && mark[1] == 0);
    assert(arena.offset == 1 * KB);

    err = arena_malloc(&arena, &mark[2], 1 * KB);
    assert(err == EARENA_SUCCESS && mark[2] == 1 * KB);
    assert(arena.offset == 2 * KB);

    err = arena_malloc(&arena, &mark[3], 1 << 20);
    assert(err == EARENA_OOM && mark[3] == 0);
    assert(arena.offset == 2 * KB);

    err = arena_reset_to(&arena, mark[2]);
    assert(err == EARENA_SUCCESS);
    assert(arena.offset == 1 * KB);

    err = arena_reset_to(&arena, mark[1]);
    assert(err == EARENA_SUCCESS);
    assert(arena.offset == 0);

    err = arena_reset_to(&arena, mark[0]);
    assert(err == EARENA_SUCCESS);
    assert(arena.offset == 0);

    printf("Success! All tests have been passed!\n");

    free(buffer);
    return 0;
}
