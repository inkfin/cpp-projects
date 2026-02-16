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
#define TLDS__PRINT(fmt, ...) fprintf(stderr, "[FATAL] %s: " fmt "\n", __func__, ##__VA_ARGS__)
#else
#define TLDS__PRINT(fmt, ...)
#endif

#ifndef TLDS_ASSERT
#include <assert.h>
#define TLDS_ASSERT(expr, msg, ...)                                              \
    do {                                                                         \
        if (!(expr)) {                                                           \
            TLDS__PRINT("Assertion failed (" #expr "): " msg, ##__VA_ARGS__); \
            assert(expr);                                                        \
        }                                                                        \
    } while (0)
#endif


typedef union tl_max_align {
    long double ld;
    void *p;
    long long ll;
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
 *   - `max_align_t` guarantee the data pointer is properly aligned for any type, but it also means the header size is
 *      at least 16 bytes on 64-bit platform, which is a trade-off for simplicity and safety.
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

#ifndef TL_ARR_INITIAL_CAPACITY
#  define TL_ARR_INITIAL_CAPACITY 4
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
#define TL_ARR_VALID(p) ((p) != NULL && TL_ARR_HEADER(p)->magic == TL_ARR_MAGIC_NUM)
#else
#define TL_ARR_VALID(p) ((p) != NULL)
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
#define arr_reserve      tl_arr_reserve
#define arr_new_item     tl_arr_new_item

#endif

static inline void *
tl__arr_init_reserve(size_t initial_cap,
                    size_t elem_size) {
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
        TLDS_ASSERT(0, "Failed to allocate memory for dynamic array");
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
#define tl_arr_init_reserve(ty, n) tl__arr_init_reserve(n, sizeof(ty))

/**
 * tl_arr_init (<ty>, p)
 *
 *   initialize the array with default initial capacity, the array is empty after initialization. The capacity will be
 *   set to `TL_ARR_INITIAL_CAPACITY`, which guarantee a non-NULL data pointer.
 *
 *   return pointer to the array if initialization is successful, NULL otherwise.
 */
#define tl_arr_init(ty) tl__arr_init_reserve(0, sizeof(ty))

static inline void
tl__arr_free(void *p) {
    if (!TL_ARR_VALID(p)) {
        // NULL-safe
        TLDS__PRINT("Array not initialized, skip free");
        return;
    }
    free(TL_ARR_HEADER(p));
}

/**
 * tl_arr_free (p)
 *
 *   free allocated memory and reassign p to `NULL`
 */
#define tl_arr_free(p) (tl__arr_free(p), (p) = NULL)

/**
 * tl_arr_clear (p)
 *
 *   clear the array by setting size to 0, the capacity remains unchanged. The caller can reuse the array after clear
 *   without reinitialization.
 */
static inline void
tl_arr_clear(void *p) {
    if (!TL_ARR_VALID(p)) {
        TLDS_ASSERT(0, "Array not initialized, skip clear");
        return;
    }
    TL_ARR_SIZE(p) = 0;
}

/** [Internal] Wrapper of realloc API, update cap & pp, doesn't check for pointer validity, return NULL if failed */
static inline void *
tl__arr_realloc(void *p, size_t new_cap, size_t elem_size) {
    TLDS_ASSERT(elem_size == 0, "Element size must be greater than 0");
    if (new_cap > TL_ARR_MAX_DATA_BYTES / elem_size) {
        TLDS_ASSERT(0, "Requested capacity too large: %zu", new_cap);
        return NULL;
    }
    struct DynArrayHeader *h =
        (struct DynArrayHeader *)realloc(TL_ARR_HEADER(p), TL_ARR_HEADER_SIZE + new_cap * elem_size);
    if (h == NULL) {
        TLDS_ASSERT(0, "Failed to reallocate memory for dynamic array with new capacity: %zu and element size %zu",
                    new_cap, elem_size);
        return NULL;
    }

    h->cap = new_cap;

    return (void *)h->data;
}

static inline void
tl__arr_shrink(void *p, size_t elem_size) {
    if (!TL_ARR_VALID(p)) {
        TLDS_ASSERT(0, "Array not initialized, skip shrink");
        return;
    }
    size_t sz = TL_ARR_SIZE(p);
    if (sz < TL_ARR_INITIAL_CAPACITY) {
        // free the data and reset to initial capacity
        tl__arr_realloc(p, TL_ARR_INITIAL_CAPACITY, elem_size);
        TL_ARR_SIZE(p) = sz;
    } else if (sz < TL_ARR_CAP(p)) {
        // shrink to fit the current size
        tl__arr_realloc(p, sz, elem_size);
    }
}

/**
 * tl_arr_shrink (p)
 *
 *   Shrink the capacity of the array to fit its current size, if the size is smaller than `TL_ARR_INITIAL_CAPACITY`,
 *   shrink to `TL_ARR_INITIAL_CAPACITY` instead to guarantee a non-NULL data pointer.
 */
#define tl_arr_shrink(ty, p) tl__arr_shrink(p, sizeof(ty))

static inline void *
tl__arr_reserve(void *p, size_t new_cap, size_t elem_size) {
    if (!TL_ARR_VALID(p)) {
        TLDS_ASSERT(0, "Array not initialized");
        return NULL;
    }
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
#define tl_arr_reserve(ty, p, n) ((p) = tl__arr_reserve(p, n, sizeof(ty)))

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
    if (new_sz < required) {
        new_sz = tl__ceil_pow2_size(required);
        if (new_sz == 0) {
            // overflow, return max size
            return TL_ARR_MAX_DATA_BYTES;
        }
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
        p = tl__arr_realloc(p, grow_cap, elem_size);
    }
    return p;
}

/** Add Items **/

static inline void *
tl__arr_new_item(void *p, size_t elem_size, void **new_item) {
    if (new_item == NULL) {
        TLDS_ASSERT(0, "pointer to new_item is NULL");
        return p;
    }
    *new_item = NULL;
    if (!TL_ARR_VALID(p)) {
        TLDS_ASSERT(0, "Array not initialized");
        return p;
    }
    size_t new_sz = TL_ARR_SIZE(p) + 1;
    void  *old = p;
    p = tl__arr_ensure(p, new_sz, elem_size);
    if (p == NULL) return old;

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
 *   out is the pointer to the new item if successful, NULL otherwise.
 *
 *   return pointer to the array if successful, NULL otherwise.
 */
#define tl_arr_new_item(ty, p, out) ((p) = (ty *)tl__arr_new_item(p, sizeof(ty), &(out)))

static inline void *
tl__arr_push(void *p, const void *v, size_t elem_size) {
    if (!TL_ARR_VALID(p)) {
        TLDS_ASSERT(0, "Array not initialized");
        return p;
    }
    size_t new_sz = TL_ARR_SIZE(p) + 1;
    void *old = p;
    p = tl__arr_ensure(p, new_sz, elem_size);
    if (p == NULL) return old;

    memcpy((char *)(p) + (new_sz - 1) * elem_size, v, elem_size);
    TL_ARR_SIZE(p) = new_sz;
    return p;
}

/**
 * tl_arr_push (<ty>, p, v)
 *
 *   Add a new item with value v to the end of the array.
 *
 *   return pointer to the array if successful, NULL otherwise.
 */
#define tl_arr_push(ty, p, v) ((p) = (ty *)tl__arr_push(p, v, sizeof(ty)))

static inline void *
tl__arr_pop_back(void *p, size_t elem_size) {
    if (!TL_ARR_VALID(p)) {
        TLDS_ASSERT(0, "Array not initialized");
        return NULL;
    }
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
 *   item through the returned pointer before the next modification to the array.
 *
 *   return pointer to the removed item if successful, NULL otherwise.
 */
#define tl_arr_pop_back(ty, p) ((ty *)tl__arr_pop_back(p, sizeof(ty)))

static inline void
tl__arr_del_fast(void *p, size_t idx, size_t elem_size) {
    if (!TL_ARR_VALID(p)) {
        TLDS_ASSERT(0, "Array not initialized");
        return;
    }
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
#define tl_arr_del_fast(ty, p, idx) tl__arr_del_fast(p, idx, sizeof(ty))


#endif  /* TINYLIB_DS_H */
