#include "../parser.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    float* nl = NULL;
    ARR_INIT(nl);
    assert(ARR_SIZE(nl) == 0);
    assert(ARR_CAP(nl) == ARR_INITIAL_CAPACITY);

    ARR_RESERVE(nl, 10);
    ARR_PUSH(nl, 3.14f);
    assert(ARR_SIZE(nl) == 1);
    assert(ARR_CAP(nl) == ARR_INITIAL_CAPACITY);

    for (size_t i = 0; i < 20; ++i) {
        ARR_PUSH(nl, 3.14f);
    }

    ARR_FREE(nl);
}
