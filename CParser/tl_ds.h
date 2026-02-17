/* vim: set ft=c : -*- mode: c -*-
 * tl_ds.h
 *   Common data structures for C projects
 *
 *   Configurations:
 *
 *     TLDS_DEBUG_PRINT:
 *       disable print warnings and errors to stderr
 *
 *     TLDS_ABBR:
 *       enable macro abbreviations without TL prefix
 *
 *     TLDS_DBG:
 *       enable debug checks and prints
 *
 *     TL_ARR_INITIAL_CAPACITY:
 *       initial capacity for dynamic array, default to 4
 */
#ifndef TINYLIB_DS_H
#define TINYLIB_DS_H

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#if defined(__cplusplus)
#error "Bro just use STL lol"
#endif
#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L) /* C99 */
#error "This library requires C99 or later."
#endif

#if TLDS_DEBUG_PRINT
#include <stdio.h>
#define TLDS__PRINT(fmt, ...) fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__)
#else
#define TLDS__PRINT(fmt, ...)
#endif

#ifndef TLDS_ASSERT
#include <assert.h>
#define TLDS_ASSERT(expr, msg, ...)                                                   \
    do {                                                                              \
        if (!(expr)) {                                                                \
            TLDS__PRINT("[ERROR] Assertion failed (" #expr "): " msg, ##__VA_ARGS__); \
            assert(expr);                                                             \
        }                                                                             \
    } while (0)
#endif

typedef union tl_max_align {
    long double ld;
    void       *p;
    long long   ll;
} tl_max_align_t;

/////////////////////
/** Dynamic Array **/
/////////////////////

/**
 * The dynamic array is implemented as a flexible header struct followed by a contiguous block of memory for the
 * elements. The user only holds a pointer to the elements, and the header is hidden just before the data. This allows
 * for efficient access to the size and capacity of the array while keeping the API simple.
 *
 * NOTE:
 *   - `magic` field only checks struct returned by library api, not arbitrary pointer.
 *   - `max_align_t` guarantee the data pointer is aligned at least to alignof(tl_max_align_t) (sufficient for most
 *     object types on this platform), but it also means the header size is at least 16 bytes on 64-bit platform, which
 *     is a trade-off for simplicity and safety.
 *   - data pointer is always non-NULL when the array is initialized, but the size can be 0
 */
struct DynArrayHeader {
    size_t size;
    size_t cap;
#ifdef TLDS_DBG
    uint32_t magic;
#endif

    tl_max_align_t data[];
};

typedef struct DynArrayRange {
    size_t start;
    size_t end;  // exclusive
} DynArrayRange;

#ifndef TL_ARR_INITIAL_CAPACITY
#define TL_ARR_INITIAL_CAPACITY 4
#endif

#define TL_ARR_MAGIC_NUM   UINT32_C(0xDEADBEEF)
#define TL_ARR_HEADER_SIZE ((size_t)offsetof(struct DynArrayHeader, data))
/* Maximum bytes for data, considering the header size and potential overflow */
#define TL_ARR_MAX_DATA_BYTES (SIZE_MAX - TL_ARR_HEADER_SIZE)

/** Handy getters that are not NULL-safe **/
#define TL_ARR_HEADER(p) ((struct DynArrayHeader *)((char *)(p) - TL_ARR_HEADER_SIZE))
#define TL_ARR_SIZE(p)   (TL_ARR_HEADER(p)->size)
#define TL_ARR_CAP(p)    (TL_ARR_HEADER(p)->cap)
#define TL_ARR_BACK(p)   (TL_ARR_SIZE(p) > 0 ? &(p)[TL_ARR_SIZE(p) - 1] : NULL)
#define TL_ARR_FRONT(p)  (TL_ARR_SIZE(p) > 0 ? &(p)[0] : NULL)
#define TL_ARR_EMPTY(p)  (TL_ARR_SIZE(p) == 0)

/** check array pointer managed by this library */
#if TLDS_DBG
#define TL_ARR_CHECK_VALID(p)                                                                                     \
    do {                                                                                                          \
        TLDS_ASSERT((p) != NULL, "Array not initialized: NULL pointer");                                          \
        TLDS_ASSERT(TL_ARR_HEADER(p)->magic == TL_ARR_MAGIC_NUM, "Invalid array pointer: magic number mismatch"); \
    } while (0)
#else
#define TL_ARR_CHECK_VALID(p)                                            \
    do {                                                                 \
        TLDS_ASSERT((p) != NULL, "Array not initialized: NULL pointer"); \
    } while (0)
#endif

#if TLDS_ABBR

#define ARR_HEADER TL_ARR_HEADER
#define ARR_SIZE   TL_ARR_SIZE
#define ARR_CAP    TL_ARR_CAP
#define ARR_BACK   TL_ARR_BACK
#define ARR_FRONT  TL_ARR_FRONT
#define ARR_EMPTY  TL_ARR_EMPTY

#define arr_init         tl_arr_init
#define arr_init_reserve tl_arr_init_reserve
#define arr_free         tl_arr_free
#define arr_clear        tl_arr_clear
#define arr_shrink       tl_arr_shrink
#define arr_reserve      tl_arr_reserve
#define arr_new_item     tl_arr_new_item
#define arr_pushp        tl_arr_pushp
#define arr_pushv        tl_arr_pushv
#define arr_pop_back     tl_arr_pop_back
#define arr_del_fast     tl_arr_del_fast
#define arr_del_stable   tl_arr_del_stable
#define arr_del          tl_arr_del

#endif

static inline void *
tl__arr_init_reserve(size_t initial_cap, size_t elem_size) {
    TLDS_ASSERT(elem_size > 0, "Element size must be greater than 0");
    if (initial_cap < TL_ARR_INITIAL_CAPACITY) {
        initial_cap = TL_ARR_INITIAL_CAPACITY;
    }
    if (initial_cap > TL_ARR_MAX_DATA_BYTES / elem_size) {
        TLDS_ASSERT(0, "Initial capacity too large: %zu", initial_cap);
        return NULL;
    }

    struct DynArrayHeader *h = (struct DynArrayHeader *)malloc(TL_ARR_HEADER_SIZE + initial_cap * elem_size);
    if (h == NULL) {
        TLDS_ASSERT(0, "Failed to allocate memory for dynamic array, size: %zu",
                    TL_ARR_HEADER_SIZE + initial_cap * elem_size);
        return NULL;
    }

    h->cap = initial_cap;
    h->size = 0;
#ifdef TLDS_DBG
    h->magic = TL_ARR_MAGIC_NUM;
#endif

    return (void *)h->data;
}

/**
 * tl_arr_init_reserve (<ty>, n)
 *
 *   Initialize the array with initial capacity of n, the array is empty after initialization. A minimal initial
 *   capacity is defined by `TL_ARR_INITIAL_CAPACITY`, which guarantee a non-NULL data pointer.
 *
 *   return pointer to the array if initialization is successful, NULL otherwise.
 *
 */
#define tl_arr_init_reserve(ty, n) ((ty *)tl__arr_init_reserve((n), sizeof(ty)))

/**
 * tl_arr_init (<ty>, p)
 *
 *   initialize the array with default initial capacity, the array is empty after initialization. The capacity will be
 *   set to `TL_ARR_INITIAL_CAPACITY`, which guarantee a non-NULL data pointer.
 *
 *   return pointer to the array if initialization is successful, NULL otherwise.
 */
#define tl_arr_init(ty) ((ty *)tl__arr_init_reserve(0, sizeof(ty)))

static inline void
tl__arr_free(void *p) {
    // NULL-safe
    if (p == NULL) return;
    TL_ARR_CHECK_VALID(p);
    free(TL_ARR_HEADER(p));
}

/**
 * tl_arr_free (p)
 *
 *   free allocated memory and reassign p to `NULL`
 */
#define tl_arr_free(p)   \
    do {                 \
        tl__arr_free(p); \
        (p) = NULL;      \
    } while (0)

/**
 * tl_arr_clear (p)
 *
 *   clear the array by setting size to 0, the capacity remains unchanged. The caller can reuse the array after clear
 *   without reinitialization.
 */
static inline void
tl_arr_clear(void *p) {
    TL_ARR_CHECK_VALID(p);
    TL_ARR_SIZE(p) = 0;
}

/** [Internal] Wrapper of realloc API, update cap & pp, doesn't check for pointer validity, return NULL if failed */
static inline void *
tl__arr_realloc(void *p, size_t new_cap, size_t elem_size) {
    TLDS_ASSERT(elem_size > 0, "Element size must be greater than 0");
    if (new_cap > TL_ARR_MAX_DATA_BYTES / elem_size) {
        TLDS__PRINT("[ERROR] Requested capacity too large: %zu > %zu", new_cap, TL_ARR_MAX_DATA_BYTES / elem_size);
        return NULL;
    }
    struct DynArrayHeader *h =
        (struct DynArrayHeader *)realloc(TL_ARR_HEADER(p), TL_ARR_HEADER_SIZE + new_cap * elem_size);
    if (h == NULL) {
        TLDS__PRINT("[ERROR] Failed to reallocate memory for dynamic array of size: %zu",
                    TL_ARR_HEADER_SIZE + new_cap * elem_size);
        return NULL;
    }

    h->cap = new_cap;

    return (void *)h->data;
}

static inline void *
tl__arr_shrink(void *p, size_t elem_size) {
    TL_ARR_CHECK_VALID(p);

    size_t sz = TL_ARR_SIZE(p);
    size_t target = (sz < TL_ARR_INITIAL_CAPACITY) ? TL_ARR_INITIAL_CAPACITY : sz;
    if (TL_ARR_CAP(p) > target) {
        p = tl__arr_realloc(p, target, elem_size);
    }
    return p;
}

/**
 * tl_arr_shrink (<ty>, p)
 *
 *   Shrink the capacity of the array to fit its current size, has a minimum capacity defined by
 *   `TL_ARR_INITIAL_CAPACITY` to guarantee a non-NULL data pointer.
 *
 *   return pointer to the array if successful, NULL otherwise.
 */
#define tl_arr_shrink(ty, p)                            \
    do {                                                \
        void *tl__np = tl__arr_shrink((p), sizeof(ty)); \
        if (tl__np) (p) = (ty *)tl__np;                 \
    } while (0)
#define tl_arr_shrink_try(ty, p, ok_out)                \
    do {                                                \
        void *tl__np = tl__arr_shrink((p), sizeof(ty)); \
        if (tl__np) {                                   \
            (p) = (ty *)tl__np;                         \
            (ok_out) = 1;                               \
        } else {                                        \
            (ok_out) = 0;                               \
        }                                               \
    } while (0)

static inline void *
tl__arr_reserve(void *p, size_t new_cap, size_t elem_size) {
    TL_ARR_CHECK_VALID(p);

    if (new_cap > TL_ARR_CAP(p)) {
        p = tl__arr_realloc(p, new_cap, elem_size);
    }
    return p;
}

/**
 * tl_arr_reserve (<ty>, p, n)
 *
 *   Ensure the array has at least capacity for n elements, if not, reallocate the array to have capacity of n.
 *
 *   return pointer to the array if successful, NULL otherwise.
 */
#define tl_arr_reserve(ty, p, n)                              \
    do {                                                      \
        void *tl__np = tl__arr_reserve((p), (n), sizeof(ty)); \
        if (tl__np) (p) = (ty *)tl__np;                       \
    } while (0)
#define tl_arr_reserve_try(ty, p, n, ok_out)                  \
    do {                                                      \
        void *tl__np = tl__arr_reserve((p), (n), sizeof(ty)); \
        if (tl__np) {                                         \
            (p) = (ty *)tl__np;                               \
            (ok_out) = 1;                                     \
        } else {                                              \
            (ok_out) = 0;                                     \
        }                                                     \
    } while (0)

/** [Internal] Align size to the next power of 2 */
static inline size_t
tl__ceil_pow2_size(size_t x) {
    if (x == 0) return 1;
    // x already power of 2
    if ((x & (x - 1)) == 0) return x;

    size_t v = x - 1;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
#if SIZE_MAX > 0xFFFFFFFFUL
    v |= v >> 32;
#endif
    v++;

    // NOTE: potential overflow if x is close to SIZE_MAX
    if (v < x) return 0;
    return v;
}

static inline size_t
tl__arr_grow_size(size_t old, size_t required) {
    if (old >= required) {
        return old;
    }
    if (required <= TL_ARR_INITIAL_CAPACITY) {
        return TL_ARR_INITIAL_CAPACITY;
    }

    size_t new_sz = old * 2;
    if (old > SIZE_MAX / 2 || new_sz < required) {
        new_sz = tl__ceil_pow2_size(required);
        // overflow, propagate failure
        if (new_sz == 0) return 0;
    }
    return new_sz;
}

/**
 * [Internal] Ensure the array has enough capacity to hold new_cap elements, if not, grow the capacity
 * to at least new_cap. The growth strategy is to double the capacity until it can hold new_cap.
 */
static inline void *
tl__arr_ensure(void *p, size_t new_cap, size_t elem_size) {
    if (new_cap > TL_ARR_CAP(p)) {
        size_t grow_cap = tl__arr_grow_size(TL_ARR_CAP(p), new_cap);
        // overflow, failed to grow
        if (grow_cap == 0) return NULL;
        p = tl__arr_realloc(p, grow_cap, elem_size);
    }
    return p;
}

/** Add Items **/

static inline void *
tl__arr_new_item(void *p, size_t elem_size, void **new_item) {
    if (new_item == NULL) {
        TLDS_ASSERT(0, "pointer to new_item is NULL");
        return NULL;
    }
    *new_item = NULL;
    TL_ARR_CHECK_VALID(p);

    size_t sz = TL_ARR_SIZE(p);
    if (sz == SIZE_MAX) {
        TLDS_ASSERT(0, "Array size overflow");
        return NULL;
    }
    size_t new_sz = sz + 1;
    p = tl__arr_ensure(p, new_sz, elem_size);
    if (p == NULL) return NULL;

    TL_ARR_SIZE(p) = new_sz;
    *new_item = (void *)((char *)p + (new_sz - 1) * elem_size);
    return p;
}

/**
 * tl_arr_new_item (<ty>, p, out)
 *
 *   Add a new item to the end of the array and return a pointer to the new item, the caller can then assign value to
 *   the new item through the returned pointer.
 *
 *   pout is the pointer to the new item if successful, NULL otherwise. NOTE: Must be a ty* lvalue variable
 *
 *   return pointer to the array to keep it updated if reallocation happens, NULL if failed to add new item.
 */
#define tl_arr_new_item(ty, p, pout)                                        \
    do {                                                                    \
        void *tl__np = tl__arr_new_item((p), sizeof(ty), (void **)&(pout)); \
        if (tl__np) (p) = (ty *)tl__np;                                     \
    } while (0)
#define tl_arr_new_item_try(ty, p, pout, ok_out)                            \
    do {                                                                    \
        void *tl__np = tl__arr_new_item((p), sizeof(ty), (void **)&(pout)); \
        if (tl__np) {                                                       \
            (p) = (ty *)tl__np;                                             \
            (ok_out) = 1;                                                   \
        } else {                                                            \
            (ok_out) = 0;                                                   \
        }                                                                   \
    } while (0)

static inline void *
tl__arr_push(void *p, const void *v, size_t elem_size) {
    TL_ARR_CHECK_VALID(p);

    size_t sz = TL_ARR_SIZE(p);
    if (sz == SIZE_MAX) {
        TLDS_ASSERT(0, "Array size overflow");
        return NULL;
    }
    size_t new_sz = sz + 1;
    p = tl__arr_ensure(p, new_sz, elem_size);
    if (p == NULL) return NULL;

    memcpy((char *)(p) + (new_sz - 1) * elem_size, v, elem_size);
    TL_ARR_SIZE(p) = new_sz;
    return p;
}

/**
 * tl_arr_push[p|v] (<ty>, p, v)
 *
 *   Add a new item with value v to the end of the array.
 *   There are two variants of this macro, tl_arr_pushp takes v as an pointer, while tl_arr_pushv takes v as a value and
 *   use compound literal to pass its address to the internal function, which is more convenient for primitive types and
 *   small structs.
 *
 *   return pointer to the array to keep it updated if reallocation happens, NULL if failed to add new item.
 */
#define tl_arr_pushp(ty, p, pv)                             \
    do {                                                    \
        void *tl__np = tl__arr_push((p), (pv), sizeof(ty)); \
        if (tl__np) {                                       \
            (p) = (ty *)tl__np;                             \
        }                                                   \
    } while (0)
#define tl_arr_pushp_try(ty, p, pv, ok_out)                 \
    do {                                                    \
        void *tl__np = tl__arr_push((p), (pv), sizeof(ty)); \
        if (tl__np) {                                       \
            (p) = (ty *)tl__np;                             \
            (ok_out) = 1;                                   \
        } else {                                            \
            (ok_out) = 0;                                   \
        }                                                   \
    } while (0)
#define tl_arr_pushv(ty, p, vv)                                    \
    do {                                                           \
        void *tl__np = tl__arr_push((p), &(ty){(vv)}, sizeof(ty)); \
        if (tl__np) {                                              \
            (p) = (ty *)tl__np;                                    \
        }                                                          \
    } while (0)
#define tl_arr_pushv_try(ty, p, vv, ok_out)                        \
    do {                                                           \
        void *tl__np = tl__arr_push((p), &(ty){(vv)}, sizeof(ty)); \
        if (tl__np) {                                              \
            (p) = (ty *)tl__np;                                    \
            (ok_out) = 1;                                          \
        } else {                                                   \
            (ok_out) = 0;                                          \
        }                                                          \
    } while (0)

static inline void *
tl__arr_pop_back(void *p, size_t elem_size) {
    TL_ARR_CHECK_VALID(p);
    if (TL_ARR_SIZE(p) == 0) {
        TLDS_ASSERT(0, "Array is empty, cannot pop");
        return NULL;
    }
    TL_ARR_SIZE(p)--;
    return (char *)p + TL_ARR_SIZE(p) * elem_size;
}

/**
 * tl_arr_pop_back (<ty>, p)
 *
 *   Remove the last item from the array and return a pointer to the removed item, the caller can access the removed
 *   item through the returned pointer before the next modification to the array, e.g. push/reserve/ensure/shrink.
 *
 *   return pointer to the removed item if successful, NULL otherwise.
 */
#define tl_arr_pop_back(ty, p) ((ty *)tl__arr_pop_back((p), sizeof(ty)))

static inline void
tl__arr_del_fast(void *p, size_t idx, size_t elem_size) {
    TL_ARR_CHECK_VALID(p);

    size_t sz = TL_ARR_SIZE(p);
    if (idx >= sz) {
        TLDS_ASSERT(0, "Index out of bounds: %zu", idx);
        return;
    }
    if (idx != sz - 1) {
        // move the last item to the deleted position
        memcpy((char *)p + idx * elem_size, (char *)p + (sz - 1) * elem_size, elem_size);
    }
    TL_ARR_SIZE(p)--;
}

/**
 * tl_arr_del_fast (<ty>, p, idx)
 *
 *   Remove the item at index idx from the array by moving the last item to the deleted position, this operation does
 *   not preserve the order of the array but is O(1).
 */
#define tl_arr_del_fast(ty, p, idx) tl__arr_del_fast((p), idx, sizeof(ty))

static inline void
tl__arr_del_stable(void *p, size_t idx, size_t elem_size) {
    TL_ARR_CHECK_VALID(p);

    size_t sz = TL_ARR_SIZE(p);
    if (idx >= sz) {
        TLDS_ASSERT(0, "Index out of bounds: %zu", idx);
        return;
    }
    if (idx != sz - 1) {
        // move the items after idx to the left by one position
        memmove((char *)p + idx * elem_size, (char *)p + (idx + 1) * elem_size, (sz - idx - 1) * elem_size);
    }
    TL_ARR_SIZE(p)--;
}

/**
 * tl_arr_del_stable (<ty>, p, idx)
 *
 *   Remove the item at index idx from the array by moving the items after idx to the left by one position, this
 *   operation preserves the order of the array but is O(n).
 */
#define tl_arr_del_stable(ty, p, idx) tl__arr_del_stable((p), idx, sizeof(ty))

#define tl_arr_del tl_arr_del_stable

#endif /* TINYLIB_DS_H */
