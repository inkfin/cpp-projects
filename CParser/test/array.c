#include <assert.h>
#include <stdio.h>
#define TLDS_ABBR
#define TLDS_DEBUG
#define TLDS_DEBUG_PRINT
#include "../tl_ds.h"

int main(void) {
    float* nl = NULL;
    arr_init(nl);
    assert(ARR_SIZE(nl) == 0);
    assert(ARR_CAP(nl) == TL_ARR_INITIAL_CAPACITY);

    arr_reserve(nl, 10);
    arr_pushv(nl, 3.14f);
    assert(ARR_SIZE(nl) == 1);
    assert(ARR_CAP(nl) == TL_ARR_INITIAL_CAPACITY);

    for (size_t i = 0; i < 20; ++i) {
        arr_pushv(nl, 3.14f);
    }

    arr_free(nl);
}
