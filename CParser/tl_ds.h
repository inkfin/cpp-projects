/* vim: set ft=c : -*- mode: c -*-
 * tl_ds.h
 *   Common data structures for C projects
 *
 *   Configurations:
 *
 *     TLDS_DISABLE_PRINT:
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
#include "tl_def.h"


#if defined(__cplusplus)
#error "Bro just use STL lol"
#endif
#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L) /* C99 */
#error "This library requires C99 or later."
#endif

#if !TLDS_DISABLE_PRINT
#include <stdio.h>
#  if TLDS_DBG
#define TLDS__PRINT_DBG(fmt, ...) fprintf(stdout, "%s: " fmt "\n", __func__, ##__VA_ARGS__)
#  else
#define TLDS__PRINT_DBG(fmt, ...)
#  endif
#define TLDS__PRINT_ERR(fmt, ...) fprintf(stderr, "[FATAL] %s: " fmt "\n", __func__, ##__VA_ARGS__)
#else
#define TLDS__PRINT_DBG(fmt, ...)
#define TLDS__PRINT_ERR(fmt, ...)
#endif

#ifndef TLDS_ASSERT
#include <assert.h>
#define TLDS_ASSERT(expr, msg)                                         \
    do {                                                               \
        if (!(expr)) {                                                 \
            TLDS__PRINT_ERR("Assertion failed: %s, %s", #expr, msg); \
            assert(expr);                                              \
        }                                                              \
    } while (0)
#endif

#ifndef TLDS_TYPEOF
#define TLDS_TYPEOF __typeof__
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
 *   - `max_align_t` guarantee in-struct alignment, not the alignment of the data block, which is defined by the element
 *      type.
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
/* Maximum number of elements, considering header size */
#define TL_ARR_DATA_MAX_SIZE (SIZE_MAX - TL_ARR_HEADER_SIZE)

/** Handy getters that are not NULL-safe **/
#define TL_ARR_HEADER(p) ((struct DynArrayHeader *)((byte_t *)(p) - TL_ARR_HEADER_SIZE))
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

/**
 * tl_arr_init_reserve (p, n)
 *
 *   initialize the array with initial capacity of n, if n is 0, use default initial capacity.
 *   The array is empty after initialization.
 *
 *   return true if initialization is successful, false otherwise (e.g. invalid parameters or memory allocation
 *   failure).
 */
static inline b32_t
tl_arr_init_reserve(void **pp,
                    size_t initial_cap,
                    size_t elem_size) {
    if (pp == NULL) return false;
    if (*pp != NULL) {
        TLDS__PRINT_DBG("Array already initialized, skip initialization");
        return true;
    }
    if (elem_size == 0) {
        TLDS__PRINT_ERR("Element size must be greater than 0");
        return false;
    }
    if (initial_cap < TL_ARR_INITIAL_CAPACITY) {
        initial_cap = TL_ARR_INITIAL_CAPACITY;
    }
    if (initial_cap > TL_ARR_DATA_MAX_SIZE / elem_size) {
        TLDS__PRINT_ERR("Initial capacity too large: %zu", initial_cap);
        return false;
    }

    struct DynArrayHeader *h = (struct DynArrayHeader *)malloc(TL_ARR_HEADER_SIZE + initial_cap * elem_size);
    if (h == NULL) {
        TLDS__PRINT_ERR("Failed to allocate memory for dynamic array");
        return false;
    }

    h->cap = initial_cap;
    h->size = 0;
#ifdef TLDS_DBG
    h->magic = TL_ARR_MAGIC_NUM;
#endif

    *pp = (void *)h->data;
    return true;
}
#define tl_arr_init_reserve(p, n) tl_arr_init_reserve((void **)&(p), n, sizeof((p)[0]))

/**
 * tl_arr_init (p)
 *
 *   initialize the array with default initial capacity, the array is empty after initialization.
 *
 *   return true if initialization is successful, false otherwise (e.g. invalid parameters or memory allocation
 *   failure).
 */
#define tl_arr_init(p) tl_arr_init_reserve((void **)&(p), 0, sizeof((p)[0]))

/**
 * tl_arr_free (p)
 *
 *   free allocated memory and reassign p to `NULL`
 */
static inline void
tl__arr_free(void **pp) {
    if (pp == NULL) return;
    if (!TL_ARR_VALID(*pp)) {
        TLDS__PRINT_DBG("Array not initialized, skip free");
        return;
    }
    free(TL_ARR_HEADER(*pp));
    *pp = NULL;
}
#define tl_arr_free(p) tl__arr_free((void **)&(p))

static inline void
tl_arr_clear(void *p) {
    if (!TL_ARR_VALID(p)) {
        TLDS__PRINT_DBG("Array not initialized, skip clear");
        return;
    }
    TL_ARR_SIZE(p) = 0;
}

/** [Internal] Wrapper of realloc API, update cap & pp, doesn't check for pointer validity */
static inline b32_t
tl__arr_realloc(void **pp, size_t new_cap, size_t elem_size) {
    struct DynArrayHeader *h =
        (struct DynArrayHeader *)realloc(TL_ARR_HEADER(*pp), TL_ARR_HEADER_SIZE + new_cap * elem_size);
    if (h == NULL) {
        TLDS__PRINT_ERR("Failed to reallocate memory for dynamic array");
        return false;
    }

    h->cap = new_cap;

    *pp = (void *)h->data;
    return true;
}

/**
 * tl_arr_reserve (p, n)
 *
 *   Ensure the array has at least capacity for n elements, if not, reallocate the array to have capacity of n.
 *
 *   return true if the array has enough capacity after the call, false otherwise (e.g. invalid parameters or memory
 *   allocation failure).
 */
static inline b32_t
tl__arr_reserve(void **pp, size_t new_cap, size_t elem_size) {
    if (pp == NULL) return false;
    if (!TL_ARR_VALID(*pp)) {
        TLDS__PRINT_DBG("Array not initialized, cannot reserve capacity");
        return false;
    }
    if (elem_size == 0) {
        TLDS__PRINT_ERR("Element size must be greater than 0");
        return false;
    }
    if (new_cap > TL_ARR_DATA_MAX_SIZE / elem_size) {
        TLDS__PRINT_ERR("Requested capacity too large: %zu", new_cap);
        return false;
    }
    if (new_cap > TL_ARR_CAP(*pp)) {
        if (!tl__arr_realloc(pp, new_cap, elem_size)) {
            return false;
        }
    }
    return true;
}
#define tl_arr_reserve(p, n) tl__arr_reserve((void **)&(p), n, sizeof((p)[0]))

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
            return TL_ARR_DATA_MAX_SIZE;
        }
    }
    return new_sz;
}

/**
 * [Internal] Ensure the array has enough capacity to hold new_cap elements, if not, grow the capacity
 * to at least new_cap. The growth strategy is to double the capacity until it can hold new_cap.
 */
static inline b32_t
tl__arr_ensure(void **pp, size_t new_cap, size_t elem_size) {
    if (pp == NULL || *pp == NULL) return false;
    if (new_cap > TL_ARR_CAP(*pp)) {
        size_t grow_cap = tl__arr_grow_size(TL_ARR_CAP(*pp), new_cap);
        if (!tl__arr_reserve(pp, grow_cap, elem_size)) {
            return false;
        }
    }
    return true;
}

/** Add Items **/

/**
 * tl_arr_new_item (p)
 *
 *   Add a new item to the end of the array and return a pointer to the new item, the caller can then assign value to
 *   the new item through the returned pointer.
 *
 *   return pointer to the new item if successful, NULL otherwise (e.g. invalid parameters or memory allocation
 *   failure).
 */
static inline void *
tl__arr_new_item(void **pp, size_t elem_size) {
    if (pp == NULL) return NULL;
    if (*pp == NULL) {
        TLDS__PRINT_DBG("Array not initialized, cannot add new item");
        return NULL;
    }
    size_t new_sz = TL_ARR_SIZE(*pp) + 1;
    if (!tl__arr_ensure(pp, new_sz, elem_size)) {
        return NULL;
    }
    TL_ARR_SIZE(*pp) = new_sz;
    return (char *)(*pp) + new_sz - 1;
}
#define tl_arr_new_item(p) (TLDS_TYPEOF((p)[0]) tl__arr_new_item(&(p), sizeof((p)[0])))

static inline void
tl__arr_push(void **pp, const void *v, size_t elem_size) {
    if (pp == NULL || *pp == NULL) return;
    TL_ARR_SIZE(*pp)++;
    if (!tl__arr_ensure(pp, TL_ARR_SIZE(*pp), elem_size)) {
        return;
    }
    memcpy((char *)(*pp) + (TL_ARR_SIZE(*pp) - 1), v, elem_size);
}
#define tl_arr_push(p, v) tl__arr_push(&(p), v, sizeof((p)[0]))

#define ARR_PUSH(p, v)                     \
    do {                                   \
        assert(p);                         \
        ARR__ENSURE((p), ARR_SIZE(p) + 1); \
        p[ARR_SIZE(p)] = v;                \
        ARR_SIZE(p) += 1;                  \
    } while (0)
#define ARR_POP(p) (assert(p), ARR_SIZE(p) > 0 ? &p[--ARR_SIZE(p)] : NULL)
#define ARR_DEL_FST(p, i)          \
    do {                           \
        assert(p);                 \
        (p)[i] = p[--ARR_SIZE(p)]; \
    } while (0)
#define ARR_SHRINK(p)                 \
    do {                              \
        assert(p);                    \
        ARR__REALLOC(p, ARR_SIZE(p)); \
    } while (0)

#endif  /* TINYLIB_DS_H */
