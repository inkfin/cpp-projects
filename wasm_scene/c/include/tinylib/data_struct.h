/* vim: set ft=c : -*- mode: c -*-
 * data_struct.h
 *   Typed dynamic array utilities for GNU11.
 *
 *   Model:
 *     - arrays are explicit heap objects with a visible flexible header:
 *         `TL_ArrInt *arr`
 *     - metadata lives in the object itself (`len`, `cap`, `alloc`)
 *     - there is no hidden stb-style prefix metadata before the returned pointer
 *     - array identity is stable, but the allocation may still move on growth
 *
 *   Initialization:
 *     - arrays are expected to be explicitly initialized with `tl_arr_init`
 *     - an initialized empty array is a valid header-only allocation with:
 *         `len == 0`, `cap == 0`
 *     - a never-initialized `NULL` pointer is treated as a different state
 *       from an initialized empty array by design
 *
 *   API shape:
 *     - read-only accessors return const-qualified views where appropriate
 *     - mutating operations require an lvalue array pointer so reallocation can
 *       update the caller-visible handle
 *     - abbreviated snake_case names are available only under `TLDS_ABBR`
 *
 *   Requirements:
 *     - this header currently relies on GNU statement expressions internally
 *     - generic language/attribute helpers live in `c_ext.h`
 */
#ifndef TINYLIB_DATA_STRUCT_H
#define TINYLIB_DATA_STRUCT_H

#include "defs.h"
#include "c_ext.h"
#include "mem.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef TLDS_DEBUG_PRINT
#include <stdio.h>
#define TLDS__PRINT(fmt, ...) fprintf(stderr, "[TLDS] %s:%d %s " fmt "\n", __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define TLDS__PRINT(fmt, ...)
#endif

#ifndef TLDS_ASSERT
#define TLDS_ASSERT(expr, msg, ...)                                                   \
    do {                                                                              \
        if (!(expr)) {                                                                \
            TLDS__PRINT("[ERROR] Assertion failed (" #expr "): " msg, ##__VA_ARGS__); \
            TL_BREAKPOINT();                                                          \
            abort();                                                                  \
        }                                                                             \
    } while (0)
#endif

#if !TL_HAS_STATEMENT_EXPR
#error "data_struct.h currently requires GNU statement expressions."
#endif

#define TLDS__EXPR(...) ({ __VA_ARGS__ })

typedef struct TL__ArrHdr {
    size_t len;
    size_t cap;
    TL_Allocator *alloc;
    byte_t data[];
} TL__ArrHdr;

typedef struct TL__ArrResult {
    TL__ArrHdr *hdr;
    b32_t ok;
} TL__ArrResult;

typedef struct TL__ArrPtrResult {
    TL__ArrHdr *hdr;
    byte_t *ptr;
    b32_t ok;
} TL__ArrPtrResult;

/*
 * Declares a reusable named dynamic-array type.
 *
 * Usage:
 *   TL_DECLARE_ARR_TYPE(TL_ArrInt, int);
 *   TL_ArrInt *arr = NULL;
 */
#define TL_DECLARE_ARR_TYPE(Name, T) \
    typedef struct Name {            \
        size_t len;                  \
        size_t cap;                  \
        const TL_Allocator *alloc;   \
        T data[];                    \
    } Name

#define TL_DEFINE_ARR_TYPE(Name, T) TL_DECLARE_ARR_TYPE(Name, T)
#define TLDS__DECLARE_ARR_TYPE(Name, T) TL_DECLARE_ARR_TYPE(Name, T);

#define TLDS_BASIC_ARR_TYPES(X)          \
    X(TL_ArrBool, bool)                  \
    X(TL_ArrChar, char)                  \
    X(TL_ArrSChar, signed char)          \
    X(TL_ArrUChar, unsigned char)        \
    X(TL_ArrShort, short)                \
    X(TL_ArrUShort, unsigned short)      \
    X(TL_ArrInt, int)                    \
    X(TL_ArrUInt, unsigned int)          \
    X(TL_ArrLong, long)                  \
    X(TL_ArrULong, unsigned long)        \
    X(TL_ArrLLong, long long)            \
    X(TL_ArrULLong, unsigned long long)  \
    X(TL_ArrSize, size_t)                \
    X(TL_ArrPtrdiff, ptrdiff_t)          \
    X(TL_ArrFloat, float)                \
    X(TL_ArrDouble, double)              \
    X(TL_ArrLDouble, long double)        \
    X(TL_ArrByte, byte_t)                \
    X(TL_ArrB8, b8_t)                    \
    X(TL_ArrB32, b32_t)                  \
    X(TL_ArrI8, i8_t)                    \
    X(TL_ArrI16, i16_t)                  \
    X(TL_ArrI32, i32_t)                  \
    X(TL_ArrI64, i64_t)                  \
    X(TL_ArrU8, u8_t)                    \
    X(TL_ArrU16, u16_t)                  \
    X(TL_ArrU32, u32_t)                  \
    X(TL_ArrU64, u64_t)

#ifndef TLDS_NO_BASIC_TYPES
TLDS_BASIC_ARR_TYPES(TLDS__DECLARE_ARR_TYPE)
#endif

/* Reference helpers: `tl_arr_ref` is read-only, `tl_arr_ref_mut` is mutable. */
#define tl_arr_ref(arr)     ((const TL_TYPEOF(arr) *)&(arr))
#define tl_arr_ref_mut(arr) (&(arr))

/* Read-only metadata queries. Safe for NULL pointers. */
#define tl_arr_len(arr)   ((arr) ? (arr)->len : 0U)
#define tl_arr_cap(arr)   ((arr) ? (arr)->cap : 0U)
#define tl_arr_empty(arr) (tl_arr_len(arr) == 0U)

/* Read-only / mutable data views. Safe for NULL pointers (returns NULL). */
#define tl_arr_data(arr) \
    ((arr) ? (const TL_TYPEOF((arr)->data[0]) *)(arr)->data : NULL)
#define tl_arr_data_mut(arr) \
    ((arr) ? (arr)->data : NULL)

/* Indexed element access. Returns NULL on out-of-bounds or NULL array. */
#define tl_arr_at(arr, idx) \
    (((arr) != NULL && (size_t)(idx) < (arr)->len) ? &tl_arr_data(arr)[(idx)] : NULL)
#define tl_arr_at_mut(arr, idx) \
    (((arr) != NULL && (size_t)(idx) < (arr)->len) ? &tl_arr_data_mut(arr)[(idx)] : NULL)

#define tl_arr_front(arr) tl_arr_at((arr), 0)
#define tl_arr_front_mut(arr) tl_arr_at_mut((arr), 0)
#define tl_arr_back(arr) \
    (((arr) != NULL && (arr)->len > 0U) ? &tl_arr_data(arr)[(arr)->len - 1U] : NULL)
#define tl_arr_back_mut(arr) \
    (((arr) != NULL && (arr)->len > 0U) ? &tl_arr_data_mut(arr)[(arr)->len - 1U] : NULL)

#ifdef TLDS_ABBR
#define ArrBool        TL_ArrBool
#define ArrChar        TL_ArrChar
#define ArrSChar       TL_ArrSChar
#define ArrUChar       TL_ArrUChar
#define ArrShort       TL_ArrShort
#define ArrUShort      TL_ArrUShort
#define ArrInt         TL_ArrInt
#define ArrUInt        TL_ArrUInt
#define ArrLong        TL_ArrLong
#define ArrULong       TL_ArrULong
#define ArrLLong       TL_ArrLLong
#define ArrULLong      TL_ArrULLong
#define ArrSize        TL_ArrSize
#define ArrPtrdiff     TL_ArrPtrdiff
#define ArrFloat       TL_ArrFloat
#define ArrDouble      TL_ArrDouble
#define ArrLDouble     TL_ArrLDouble
#define ArrByte        TL_ArrByte
#define ArrB8          TL_ArrB8
#define ArrB32         TL_ArrB32
#define ArrI8          TL_ArrI8
#define ArrI16         TL_ArrI16
#define ArrI32         TL_ArrI32
#define ArrI64         TL_ArrI64
#define ArrU8          TL_ArrU8
#define ArrU16         TL_ArrU16
#define ArrU32         TL_ArrU32
#define ArrU64         TL_ArrU64
#define arr_ref        tl_arr_ref
#define arr_ref_mut    tl_arr_ref_mut
#define arr_len        tl_arr_len
#define arr_cap        tl_arr_cap
#define arr_empty      tl_arr_empty
#define arr_data       tl_arr_data
#define arr_data_mut   tl_arr_data_mut
#define arr_at         tl_arr_at
#define arr_at_mut     tl_arr_at_mut
#define arr_front      tl_arr_front
#define arr_front_mut  tl_arr_front_mut
#define arr_back       tl_arr_back
#define arr_back_mut   tl_arr_back_mut
#define arr_init       tl_arr_init
#define arr_free       tl_arr_free
#define arr_clear      tl_arr_clear
#define arr_reserve    tl_arr_reserve
#define arr_resize     tl_arr_resize
#define arr_set_cap    tl_arr_reserve
#define arr_set_len    tl_arr_resize
#define arr_push       tl_arr_push
#define arr_push_n     tl_arr_push_n
#define arr_pushp      tl_arr_pushp
#define arr_append     tl_arr_append
#define arr_addnptr    tl_arr_addnptr
#define arr_addnidx    tl_arr_addnidx
#define arr_insn       tl_arr_insn
#define arr_ins        tl_arr_ins
#define arr_pop        tl_arr_pop
#define arr_del        tl_arr_del
#define arr_deln       tl_arr_deln
#define arr_del_swap   tl_arr_del_swap
#endif

static inline
TL_Allocator *
tl__arr_allocator(const TL__ArrHdr *arr)
{
    // NOTE: the default allocator won't change any context,
    //       cast here only to satisfy linters
    return (arr && arr->alloc) ? arr->alloc : (TL_Allocator *)&tl_default_allocator;
}

static inline
TL__ArrHdr *
tl__arr_init_impl(TL_Allocator *alloc)
{
    TL__ArrHdr *arr;
    // NOTE: the default allocator won't change any context,
    //       cast here only to satisfy linters
    TL_Allocator *resolved = alloc ? alloc : (TL_Allocator *)&tl_default_allocator;
    size_t total_size = sizeof(TL__ArrHdr);

    arr = (TL__ArrHdr *)tl_allocator_alloc(resolved, total_size);
    if (arr == NULL) {
        TLDS__PRINT("[ERROR] failed to allocate empty array header");
        return NULL;
    }

    arr->len = 0;
    arr->cap = 0;
    arr->alloc = resolved;
    return arr;
}

static inline
size_t
tl__arr_next_cap(size_t old_cap, size_t need_cap)
{
    if (need_cap <= old_cap) return old_cap;
    if (old_cap == 0) return need_cap;
    if (old_cap > SIZE_MAX / 2) return need_cap;
    return TL_MAX(need_cap, old_cap * 2);
}

static inline
TL__ArrResult
tl__arr_reserve_impl(TL__ArrHdr *arr, size_t elem_size, size_t need_cap)
{
    TL__ArrHdr *next;
    TL_Allocator *alloc;
    size_t new_total;
    size_t data_bytes;
    size_t new_cap;
    TL__ArrResult result = {0};

    TLDS_ASSERT(elem_size > 0, "element size must be greater than 0");
    if (arr == NULL) {
        arr = tl__arr_init_impl(NULL);
        if (arr == NULL) return result;
    }
    if (need_cap <= arr->cap) {
        result.hdr = arr;
        result.ok = 1;
        return result;
    }

    new_cap = tl__arr_next_cap(arr->cap, need_cap);
    if (new_cap > SIZE_MAX / elem_size) {
        TLDS_ASSERT(0, "array allocation size overflow");
        return result;
    }
    data_bytes = elem_size * new_cap;
    if (data_bytes > SIZE_MAX - sizeof(TL__ArrHdr)) {
        TLDS_ASSERT(0, "array allocation size overflow");
        return result;
    }
    new_total = sizeof(TL__ArrHdr) + data_bytes;

    alloc = tl__arr_allocator(arr);
    if (arr->cap > SIZE_MAX / elem_size) {
        TLDS_ASSERT(0, "array allocation size overflow");
        return result;
    }
    next = (TL__ArrHdr *)tl_allocator_realloc(alloc, arr, new_total);
    if (next == NULL && new_total != 0U) {
        TLDS__PRINT("[ERROR] failed to grow array to %zu elements", new_cap);
        return result;
    }

    if (next != NULL) {
        next->cap = new_cap;
        next->alloc = alloc;
    }

    result.hdr = next;
    result.ok = 1;
    return result;
}

TL_ATTR_MAYBE_UNUSED
static inline
TL__ArrResult
tl__arr_resize_impl(TL__ArrHdr *arr, size_t elem_size, size_t new_len)
{
    TL__ArrResult result = tl__arr_reserve_impl(arr, elem_size, new_len);

    if (!result.ok) return result;
    result.hdr->len = new_len;
    return result;
}

TL_ATTR_MAYBE_UNUSED
static inline
TL__ArrResult
tl__arr_append_impl(TL__ArrHdr *arr, const void *src, size_t count, size_t elem_size)
{
    size_t old_len;
    TL__ArrResult result = {0};

    TLDS_ASSERT(src != NULL || count == 0, "append source must not be NULL when count > 0");

    if (count == 0) {
        result.hdr = arr;
        result.ok = 1;
        return result;
    }

    old_len = arr ? arr->len : 0U;
    if (old_len > SIZE_MAX - count) {
        TLDS_ASSERT(0, "array length overflow");
        return result;
    }
    result = tl__arr_reserve_impl(arr, elem_size, old_len + count);
    if (!result.ok) return result;

    memcpy(result.hdr->data + old_len * elem_size, src, count * elem_size);
    result.hdr->len = old_len + count;
    return result;
}

static inline
TL__ArrPtrResult
tl__arr_addnptr_impl(TL__ArrHdr *arr, size_t elem_size, size_t count)
{
    size_t old_len;
    TL__ArrPtrResult result = {0};

    if (arr == NULL) {
        arr = tl__arr_init_impl(NULL);
        if (arr == NULL) return result;
    }
    old_len = arr->len;
    if (count == 0) {
        result.hdr = arr;
        result.ptr = arr->data + old_len * elem_size;
        result.ok = 1;
        return result;
    }
    if (old_len > SIZE_MAX - count) {
        TLDS_ASSERT(0, "array length overflow");
        return result;
    }
    {
        TL__ArrResult reserve = tl__arr_reserve_impl(arr, elem_size, old_len + count);
        if (!reserve.ok) return result;
        arr = reserve.hdr;
    }

    arr->len = old_len + count;
    result.hdr = arr;
    result.ptr = arr->data + old_len * elem_size;
    result.ok = 1;
    return result;
}

TL_ATTR_MAYBE_UNUSED
static inline
TL__ArrPtrResult
tl__arr_insn_impl(TL__ArrHdr *arr, size_t elem_size, size_t idx, size_t count)
{
    TL__ArrPtrResult result = {0};

    if (arr == NULL) {
        arr = tl__arr_init_impl(NULL);
        if (arr == NULL) return result;
    }
    TLDS_ASSERT(idx <= arr->len, "insert index out of bounds");

    if (count == 0) {
        result.hdr = arr;
        result.ptr = arr->data + idx * elem_size;
        result.ok = 1;
        return result;
    }

    result = tl__arr_addnptr_impl(arr, elem_size, count);
    if (!result.ok) return result;
    arr = result.hdr;

    memmove(arr->data + (idx + count) * elem_size,
            arr->data + idx * elem_size,
            (arr->len - idx - count) * elem_size);
    result.hdr = arr;
    result.ptr = arr->data + idx * elem_size;
    return result;
}

TL_ATTR_MAYBE_UNUSED
static inline
b32_t
tl__arr_pop_impl(TL__ArrHdr *arr, size_t elem_size, void *out)
{
    TL__ArrHdr *hdr = arr;

    TLDS_ASSERT(hdr != NULL, "array must not be NULL");
    TLDS_ASSERT(out != NULL, "pop destination must not be NULL");
    TLDS_ASSERT(hdr->len > 0, "array is empty");

    hdr->len--;
    memcpy(out, hdr->data + hdr->len * elem_size, elem_size);
    return 1;
}

TL_ATTR_MAYBE_UNUSED
static inline
b32_t
tl__arr_deln_impl(TL__ArrHdr *arr, size_t elem_size, size_t idx, size_t count)
{
    TL__ArrHdr *hdr = arr;

    TLDS_ASSERT(hdr != NULL, "array must not be NULL");
    TLDS_ASSERT(idx <= hdr->len, "delete index out of bounds");
    TLDS_ASSERT(count <= hdr->len - idx, "delete count out of bounds");

    if (count == 0) return 1;

    memmove(hdr->data + idx * elem_size,
            hdr->data + (idx + count) * elem_size,
            (hdr->len - idx - count) * elem_size);
    hdr->len -= count;
    return 1;
}

TL_ATTR_MAYBE_UNUSED
static inline
b32_t
tl__arr_del_swap_impl(TL__ArrHdr *arr, size_t elem_size, size_t idx)
{
    TL__ArrHdr *hdr = arr;

    TLDS_ASSERT(hdr != NULL, "array must not be NULL");
    TLDS_ASSERT(idx < hdr->len, "delete index out of bounds");

    if (idx != hdr->len - 1U) {
        memcpy(hdr->data + idx * elem_size,
               hdr->data + (hdr->len - 1U) * elem_size,
               elem_size);
    }
    hdr->len--;
    return 1;
}

TL_ATTR_MAYBE_UNUSED
static inline
void
tl__arr_clear_impl(TL__ArrHdr *arr)
{
    TL__ArrHdr *hdr = arr;
    if (hdr != NULL) hdr->len = 0;
}

TL_ATTR_MAYBE_UNUSED
static inline
void
tl__arr_free_impl(TL__ArrHdr *arr, size_t elem_size)
{
    TL_Allocator *alloc;
    if (arr == NULL) return;

    alloc = tl__arr_allocator(arr);
    TLDS_ASSERT(arr->cap <= SIZE_MAX / elem_size, "array allocation size overflow");
    TLDS_ASSERT(elem_size * arr->cap <= SIZE_MAX - sizeof(TL__ArrHdr), "array allocation size overflow");
    tl_allocator_free(alloc, arr);
}

/*
 * tl_arr_init(arr, allocator)
 *   Initializes `arr` as an empty header-only array.
 *   `arr` must be an lvalue of some declared array pointer type,
 *   for example `TL_ArrInt *`.
 *   `allocator == NULL` selects the default stdmalloc/stdfree allocator.
 */
#define tl_arr_init(arr, allocator) \
    do {                            \
        TL_REQUIRE_LVALUE(arr);     \
        (arr) = (TL_TYPEOF(arr))tl__arr_init_impl((allocator)); \
    } while (0)

/* Sets length to 0 and keeps capacity/allocation. NULL-safe. */
#define tl_arr_clear(arr) \
    do {                  \
        tl__arr_clear_impl((arr)); \
    } while (0)

/* Frees the allocation (if any) and sets `arr = NULL`. */
#define tl_arr_free(arr) \
    do { \
        TL_REQUIRE_LVALUE(arr); \
        tl__arr_free_impl((TL__ArrHdr *)(arr), sizeof((arr)->data[0])); \
        (arr) = NULL; \
    } while (0)

/*
 * tl_arr_reserve(arr, n) -> b32_t
 *   Ensures capacity >= n.
 *   Returns 1 on success, 0 on allocation/overflow failure.
 *   May reassign `arr`.
 */
#define tl_arr_reserve(arr, n) \
    TLDS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        TL__ArrResult tl__r = tl__arr_reserve_impl((TL__ArrHdr *)(arr), sizeof((arr)->data[0]), (size_t)(n)); \
        if (tl__r.ok) (arr) = (TL_TYPEOF(arr))tl__r.hdr; \
        tl__r.ok; \
    )

/*
 * tl_arr_resize(arr, n) -> b32_t
 *   Sets length to `n`. New slots are uninitialized.
 *   Returns 1 on success, 0 on allocation/overflow failure.
 *   May reassign `arr`.
 */
#define tl_arr_resize(arr, n) \
    TLDS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        TL__ArrResult tl__r = tl__arr_resize_impl((TL__ArrHdr *)(arr), sizeof((arr)->data[0]), (size_t)(n)); \
        if (tl__r.ok) (arr) = (TL_TYPEOF(arr))tl__r.hdr; \
        tl__r.ok; \
    )

/*
 * tl_arr_push(arr, value) -> b32_t
 *   Appends one value.
 */
#define tl_arr_push(arr, value) \
    TLDS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        TL_TYPEOF((arr)->data[0]) tl__value = (value); \
        TL__ArrResult tl__r = tl__arr_append_impl((TL__ArrHdr *)(arr), &tl__value, 1U, sizeof(tl__value)); \
        if (tl__r.ok) (arr) = (TL_TYPEOF(arr))tl__r.hdr; \
        tl__r.ok; \
    )

/*
 * tl_arr_push_n(arr, ...) -> b32_t
 *   Appends multiple values of element type.
 */
#define tl_arr_push_n(arr, ...) \
    TLDS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        TL_TYPEOF((arr)->data[0]) tl__items[] = { __VA_ARGS__ }; \
        TL__ArrResult tl__r = tl__arr_append_impl((TL__ArrHdr *)(arr), tl__items, TL_COUNT_OF(tl__items), sizeof(tl__items[0])); \
        if (tl__r.ok) (arr) = (TL_TYPEOF(arr))tl__r.hdr; \
        tl__r.ok; \
    )

/*
 * tl_arr_append(dst, src) -> b32_t
 *   Appends all items from `src` into `dst`.
 *   Element types must match (checked via typed temporary).
 */
#define tl_arr_append(dst, src) \
    TLDS__EXPR( \
        TL_REQUIRE_LVALUE(dst); \
        const TL_TYPEOF((dst)->data[0]) *tl__src_data = tl_arr_data(src); \
        (void)tl__src_data; \
        TL__ArrResult tl__r = tl__arr_append_impl((TL__ArrHdr *)(dst), tl_arr_data(src), tl_arr_len(src), sizeof((dst)->data[0])); \
        if (tl__r.ok) (dst) = (TL_TYPEOF(dst))tl__r.hdr; \
        tl__r.ok; \
    )

/*
 * tl_arr_addnptr(arr, n) -> T *
 *   Appends `n` uninitialized items.
 *   Returns pointer to first newly appended slot, or NULL on failure.
 *   `n == 0` returns pointer to insertion position at current end.
 */
#define tl_arr_addnptr(arr, n) \
    TLDS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        TL__ArrPtrResult tl__r = tl__arr_addnptr_impl((TL__ArrHdr *)(arr), sizeof((arr)->data[0]), (size_t)(n)); \
        if (tl__r.ok) (arr) = (TL_TYPEOF(arr))tl__r.hdr; \
        (TL_TYPEOF(&(arr)->data[0]))tl__r.ptr; \
    )

/*
 * tl_arr_addnidx(arr, n) -> size_t
 *   Same as tl_arr_addnptr but returns the starting index.
 *   Returns SIZE_MAX on failure.
 */
#define tl_arr_addnidx(arr, n) \
    TLDS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        size_t tl__old_len = tl_arr_len(arr); \
        TL__ArrPtrResult tl__r = tl__arr_addnptr_impl((TL__ArrHdr *)(arr), sizeof((arr)->data[0]), (size_t)(n)); \
        if (tl__r.ok) (arr) = (TL_TYPEOF(arr))tl__r.hdr; \
        tl__r.ok ? tl__old_len : (size_t)-1; \
    )

/*
 * tl_arr_insn(arr, idx, n) -> T *
 *   Inserts `n` uninitialized items at `idx`.
 *   Returns pointer to first inserted slot, or NULL on failure.
 */
#define tl_arr_insn(arr, idx, n) \
    TLDS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        TL__ArrPtrResult tl__r = tl__arr_insn_impl((TL__ArrHdr *)(arr), sizeof((arr)->data[0]), (size_t)(idx), (size_t)(n)); \
        if (tl__r.ok) (arr) = (TL_TYPEOF(arr))tl__r.hdr; \
        (TL_TYPEOF(&(arr)->data[0]))tl__r.ptr; \
    )

/* Inserts one value at `idx`. Returns 1 on success, 0 on failure. */
#define tl_arr_ins(arr, idx, value) \
    TLDS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        TL_TYPEOF((arr)->data[0]) tl__value = (value); \
        TL_TYPEOF(&(arr)->data[0]) tl__slot = tl_arr_insn((arr), (idx), 1U); \
        if (tl__slot != NULL) *tl__slot = tl__value; \
        tl__slot != NULL; \
    )

/* Appends one uninitialized slot; shorthand for tl_arr_addnptr(arr, 1). */
#define tl_arr_pushp(arr) \
    tl_arr_addnptr((arr), 1U)

/* Pops last element into `*out_ptr`. Returns 1 on success. */
#define tl_arr_pop(arr, out_ptr) \
    TLDS__EXPR( \
        TL_TYPEOF(&(arr)->data[0]) tl__out = (out_ptr); \
        tl__arr_pop_impl((TL__ArrHdr *)(arr), sizeof((arr)->data[0]), tl__out); \
    )

/* Deletes one element at `idx` and shifts tail left. Returns 1 on success. */
#define tl_arr_del(arr, idx) \
    TLDS__EXPR( \
        tl__arr_deln_impl((TL__ArrHdr *)(arr), sizeof((arr)->data[0]), (size_t)(idx), 1U); \
    )

/* Deletes `n` elements at `idx` and shifts tail left. Returns 1 on success. */
#define tl_arr_deln(arr, idx, n) \
    TLDS__EXPR( \
        tl__arr_deln_impl((TL__ArrHdr *)(arr), sizeof((arr)->data[0]), (size_t)(idx), (size_t)(n)); \
    )

/* Deletes one element at `idx` by swapping with last. Order is not preserved. */
#define tl_arr_del_swap(arr, idx) \
    TLDS__EXPR( \
        tl__arr_del_swap_impl((TL__ArrHdr *)(arr), sizeof((arr)->data[0]), (size_t)(idx)); \
    )

#endif /* TINYLIB_DATA_STRUCT_H */
