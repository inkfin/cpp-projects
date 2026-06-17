/* vim: set ft=c : -*- mode: c -*-
 * data_struct.h
 *   Hidden-header dynamic array utilities for GNU11.
 */
#ifndef TINYLIB_DATA_STRUCT_H
#define TINYLIB_DATA_STRUCT_H

#include "defs.h"
#include "c_ext.h"
#include "mem.h"
#include "strview.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef TL_DS_DEBUG_PRINT
#include <stdio.h>
#define TL_DS__PRINT(fmt, ...) fprintf(stderr, "[TL_DS] %s:%d %s " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define TL_DS__PRINT(fmt, ...)
#endif

#ifndef TL_DS_ASSERT
#define TL_DS_ASSERT(expr, msg, ...)                                                   \
    do {                                                                               \
        if (!(expr)) {                                                                 \
            TL_DS__PRINT("[ERROR] Assertion failed (" #expr "): " msg, ##__VA_ARGS__); \
            TL_BREAKPOINT();                                                           \
            abort();                                                                   \
        }                                                                              \
    } while (0)
#endif

#if !TL_HAS_STATEMENT_EXPR
#error "data_struct.h currently requires GNU statement expressions."
#endif

#define TL_DS__EXPR(...) ({ __VA_ARGS__ })

#ifdef TL_DS_DEBUG
#define TL_DS__ARR_MAGIC UINT32_C(0xDA77A77A)
#endif

typedef struct TL__ArrHdr {
    size_t len;
    size_t cap;
    size_t elem_size;
    size_t align;
    TL_Allocator *alloc;
#ifdef TL_DS_DEBUG
    uint32_t magic;
#endif
} TL__ArrHdr;

typedef struct TL__ArrResult {
    void *data;
    b32_t ok;
} TL__ArrResult;

typedef struct TL__ArrPtrResult {
    void *data;
    byte_t *ptr;
    b32_t ok;
} TL__ArrPtrResult;

/*
 * Dynamic arrays are plain `T *` values. The metadata header lives immediately
 * before the returned data pointer, padded to the element alignment.
 */
#define TL_DECLARE_ARR_TYPE(Name, T) typedef T Name
#define TL_DS__DECLARE_ARR_TYPE(Name, T) TL_DECLARE_ARR_TYPE(Name, T);

#define TL_DS_BASIC_ARR_TYPES(X)          \
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

#ifndef TL_DS_NO_BASIC_TYPES
TL_DS_BASIC_ARR_TYPES(TL_DS__DECLARE_ARR_TYPE)
#endif


#define TL_DS__HEADER_SIZE(align) tl_align_up(sizeof(TL__ArrHdr), (align))
#define TL_DS__HDR_WITH_ALIGN(arr, align) \
    ((TL__ArrHdr *)((byte_t *)(arr) - TL_DS__HEADER_SIZE((align))))
#define TL_DS__HDR(arr) TL_DS__HDR_WITH_ALIGN((arr), TL_ALIGNOF(*(arr)))

#ifdef TL_DS_DEBUG
TL_ATTR_MAYBE_UNUSED
static inline
void
tl__arr_check(const TL__ArrHdr *hdr)
{
    TL_DS_ASSERT(hdr != NULL, "array header must not be NULL");
    TL_DS_ASSERT(hdr->magic == TL_DS__ARR_MAGIC, "invalid dynamic array pointer");
}
#else
TL_ATTR_MAYBE_UNUSED
static inline
void
tl__arr_check(const TL__ArrHdr *hdr)
{
    (void)hdr;
}
#endif

TL_ATTR_MAYBE_UNUSED
static inline
TL_Allocator *
tl__arr_allocator(const TL__ArrHdr *hdr)
{
    return (hdr && hdr->alloc) ? hdr->alloc : (TL_Allocator *)&tl_default_allocator;
}

TL_ATTR_MAYBE_UNUSED
static inline
size_t
tl__arr_total_size(size_t elem_size, size_t align, size_t cap)
{
    size_t header_size;
    size_t data_size;

    header_size = TL_DS__HEADER_SIZE(align);
    if (header_size == 0) return 0;
    if (cap > SIZE_MAX / elem_size) return 0;
    data_size = cap * elem_size;
    if (data_size > SIZE_MAX - header_size) return 0;
    return header_size + data_size;
}

TL_ATTR_MAYBE_UNUSED
static inline
void *
tl__arr_data_from_hdr(TL__ArrHdr *hdr)
{
    return (byte_t *)hdr + TL_DS__HEADER_SIZE(hdr->align);
}

TL_ATTR_MAYBE_UNUSED
static inline
TL__ArrResult
tl__arr_init_impl(TL_Allocator *alloc, size_t elem_size, size_t align)
{
    TL__ArrResult result = {0};
    TL_Allocator *resolved = alloc ? alloc : (TL_Allocator *)&tl_default_allocator;
    size_t total_size = tl__arr_total_size(elem_size, align, 0);
    TL__ArrHdr *hdr;

    TL_DS_ASSERT(elem_size > 0, "element size must be greater than 0");
    TL_DS_ASSERT(tl_is_power_of_two(align), "array alignment must be a power of two");
    if (total_size == 0) return result;

    hdr = (TL__ArrHdr *)tl_allocator_alloc_aligned(resolved, total_size, align);
    if (!hdr) return result;

    hdr->len = 0;
    hdr->cap = 0;
    hdr->elem_size = elem_size;
    hdr->align = align;
    hdr->alloc = resolved;
#ifdef TL_DS_DEBUG
    hdr->magic = TL_DS__ARR_MAGIC;
#endif

    result.data = tl__arr_data_from_hdr(hdr);
    result.ok = 1;
    return result;
}

TL_ATTR_MAYBE_UNUSED
static inline
size_t
tl__arr_next_cap(size_t old_cap, size_t need_cap)
{
    if (need_cap <= old_cap) return old_cap;
    if (old_cap == 0) return need_cap;
    if (old_cap > SIZE_MAX / 2U) return need_cap;
    return TL_MAX(need_cap, old_cap * 2U);
}

TL_ATTR_MAYBE_UNUSED
static inline
TL__ArrResult
tl__arr_reserve_impl(void *arr,
                     TL_Allocator *init_alloc,
                     size_t elem_size,
                     size_t align,
                     size_t need_cap)
{
    TL__ArrResult result = {0};
    TL__ArrHdr *hdr;
    TL_Allocator *alloc;
    size_t old_total;
    size_t new_total;
    size_t new_cap;

    if (!arr) {
        result = tl__arr_init_impl(init_alloc, elem_size, align);
        if (!result.ok) return result;
        arr = result.data;
    }

    hdr = TL_DS__HDR_WITH_ALIGN(arr, align);
    tl__arr_check(hdr);
    TL_DS_ASSERT(hdr->elem_size == elem_size, "array element size mismatch");
    TL_DS_ASSERT(hdr->align == align, "array alignment mismatch");

    if (need_cap <= hdr->cap) {
        result.data = arr;
        result.ok = 1;
        return result;
    }

    new_cap = tl__arr_next_cap(hdr->cap, need_cap);
    old_total = tl__arr_total_size(elem_size, align, hdr->cap);
    new_total = tl__arr_total_size(elem_size, align, new_cap);
    if (old_total == 0 || new_total == 0) return result;

    alloc = tl__arr_allocator(hdr);
    hdr = (TL__ArrHdr *)tl_allocator_realloc_aligned(alloc, hdr, old_total, new_total, align);
    if (!hdr) return result;

    hdr->cap = new_cap;
    hdr->elem_size = elem_size;
    hdr->align = align;
    hdr->alloc = alloc;
#ifdef TL_DS_DEBUG
    hdr->magic = TL_DS__ARR_MAGIC;
#endif

    result.data = tl__arr_data_from_hdr(hdr);
    result.ok = 1;
    return result;
}

TL_ATTR_MAYBE_UNUSED
static inline
TL__ArrResult
tl__arr_resize_impl(void *arr, size_t elem_size, size_t align, size_t new_len)
{
    TL__ArrResult result = tl__arr_reserve_impl(arr, NULL, elem_size, align, new_len);
    TL__ArrHdr *hdr;

    if (!result.ok) return result;
    hdr = TL_DS__HDR_WITH_ALIGN(result.data, align);
    hdr->len = new_len;
    return result;
}

TL_ATTR_MAYBE_UNUSED
static inline
TL__ArrResult
tl__arr_append_impl(void *arr, const void *src, size_t count, size_t elem_size, size_t align)
{
    TL__ArrResult result = {0};
    TL__ArrHdr *hdr;
    size_t old_len;

    TL_DS_ASSERT(src != NULL || count == 0, "append source must not be NULL when count > 0");

    old_len = arr ? TL_DS__HDR_WITH_ALIGN(arr, align)->len : 0U;
    if (old_len > SIZE_MAX - count) return result;

    result = tl__arr_reserve_impl(arr, NULL, elem_size, align, old_len + count);
    if (!result.ok) return result;

    hdr = TL_DS__HDR_WITH_ALIGN(result.data, align);
    if (count > 0) {
        memcpy((byte_t *)result.data + old_len * elem_size, src, count * elem_size);
        hdr->len = old_len + count;
    }
    return result;
}

TL_ATTR_MAYBE_UNUSED
static inline
TL__ArrPtrResult
tl__arr_addnptr_impl(void *arr, size_t elem_size, size_t align, size_t count)
{
    TL__ArrPtrResult result = {0};
    TL__ArrResult reserve;
    TL__ArrHdr *hdr;
    size_t old_len;

    old_len = arr ? TL_DS__HDR_WITH_ALIGN(arr, align)->len : 0U;
    if (old_len > SIZE_MAX - count) return result;

    reserve = tl__arr_reserve_impl(arr, NULL, elem_size, align, old_len + count);
    if (!reserve.ok) return result;

    hdr = TL_DS__HDR_WITH_ALIGN(reserve.data, align);
    hdr->len = old_len + count;

    result.data = reserve.data;
    result.ptr = (byte_t *)reserve.data + old_len * elem_size;
    result.ok = 1;
    return result;
}

TL_ATTR_MAYBE_UNUSED
static inline
TL__ArrPtrResult
tl__arr_insn_impl(void *arr, size_t elem_size, size_t align, size_t idx, size_t count)
{
    TL__ArrPtrResult result = {0};
    TL__ArrHdr *hdr;
    size_t old_len;

    old_len = arr ? TL_DS__HDR_WITH_ALIGN(arr, align)->len : 0U;
    TL_DS_ASSERT(idx <= old_len, "insert index out of bounds");

    result = tl__arr_addnptr_impl(arr, elem_size, align, count);
    if (!result.ok) return result;

    hdr = TL_DS__HDR_WITH_ALIGN(result.data, align);
    memmove((byte_t *)result.data + (idx + count) * elem_size,
            (byte_t *)result.data + idx * elem_size,
            (old_len - idx) * elem_size);
    result.ptr = (byte_t *)result.data + idx * elem_size;
    (void)hdr;
    return result;
}

TL_ATTR_MAYBE_UNUSED
static inline
b32_t
tl__arr_pop_impl(void *arr, size_t elem_size, size_t align, void *out)
{
    TL__ArrHdr *hdr;

    TL_DS_ASSERT(arr != NULL, "array must not be NULL");
    TL_DS_ASSERT(out != NULL, "pop destination must not be NULL");

    hdr = TL_DS__HDR_WITH_ALIGN(arr, align);
    tl__arr_check(hdr);
    TL_DS_ASSERT(hdr->len > 0, "array is empty");

    hdr->len--;
    memcpy(out, (byte_t *)arr + hdr->len * elem_size, elem_size);
    return 1;
}

TL_ATTR_MAYBE_UNUSED
static inline
b32_t
tl__arr_deln_impl(void *arr, size_t elem_size, size_t align, size_t idx, size_t count)
{
    TL__ArrHdr *hdr;

    TL_DS_ASSERT(arr != NULL, "array must not be NULL");
    hdr = TL_DS__HDR_WITH_ALIGN(arr, align);
    tl__arr_check(hdr);
    TL_DS_ASSERT(idx <= hdr->len, "delete index out of bounds");
    TL_DS_ASSERT(count <= hdr->len - idx, "delete count out of bounds");

    if (count == 0) return 1;
    memmove((byte_t *)arr + idx * elem_size,
            (byte_t *)arr + (idx + count) * elem_size,
            (hdr->len - idx - count) * elem_size);
    hdr->len -= count;
    return 1;
}

TL_ATTR_MAYBE_UNUSED
static inline
b32_t
tl__arr_del_swap_impl(void *arr, size_t elem_size, size_t align, size_t idx)
{
    TL__ArrHdr *hdr;

    TL_DS_ASSERT(arr != NULL, "array must not be NULL");
    hdr = TL_DS__HDR_WITH_ALIGN(arr, align);
    tl__arr_check(hdr);
    TL_DS_ASSERT(idx < hdr->len, "delete index out of bounds");

    if (idx != hdr->len - 1U) {
        memcpy((byte_t *)arr + idx * elem_size,
               (byte_t *)arr + (hdr->len - 1U) * elem_size,
               elem_size);
    }
    hdr->len--;
    return 1;
}

TL_ATTR_MAYBE_UNUSED
static inline
void
tl__arr_clear_impl(void *arr, size_t align)
{
    TL__ArrHdr *hdr;
    if (!arr) return;
    hdr = TL_DS__HDR_WITH_ALIGN(arr, align);
    tl__arr_check(hdr);
    hdr->len = 0;
}

TL_ATTR_MAYBE_UNUSED
static inline
void
tl__arr_free_impl(void *arr, size_t elem_size, size_t align)
{
    TL__ArrHdr *hdr;
    TL_Allocator *alloc;
    size_t total_size;

    if (!arr) return;
    hdr = TL_DS__HDR_WITH_ALIGN(arr, align);
    tl__arr_check(hdr);
    alloc = tl__arr_allocator(hdr);
    total_size = tl__arr_total_size(elem_size, align, hdr->cap);
    if (total_size == 0) return;
    tl_allocator_free_aligned(alloc, hdr, total_size, align);
}

/* Core metadata and element access. */
#define tl_arr_len(arr)   ((arr) ? TL_DS__HDR(arr)->len : 0U)
#define tl_arr_cap(arr)   ((arr) ? TL_DS__HDR(arr)->cap : 0U)
#define tl_arr_empty(arr) (tl_arr_len(arr) == 0U)
#define tl_arr_data(arr) ((const TL_TYPEOF(*(arr)) *)(arr))
#define tl_arr_data_mut(arr) (arr)

#define tl_arr_at(arr, idx) \
    (((arr) != NULL && (size_t)(idx) < tl_arr_len(arr)) ? &(arr)[(idx)] : NULL)
#define tl_arr_at_mut(arr, idx) tl_arr_at((arr), (idx))
#define tl_arr_front(arr) tl_arr_at((arr), 0)
#define tl_arr_front_mut(arr) tl_arr_at_mut((arr), 0)
#define tl_arr_back(arr) \
    (((arr) != NULL && tl_arr_len(arr) > 0U) ? &(arr)[tl_arr_len(arr) - 1U] : NULL)
#define tl_arr_back_mut(arr) tl_arr_back((arr))

/* Core lifetime and capacity operations. */
#define tl_arr_init(arr, allocator) \
    do { \
        TL_REQUIRE_LVALUE(arr); \
        TL__ArrResult tl__r = tl__arr_init_impl((allocator), sizeof(*(arr)), TL_ALIGNOF(*(arr))); \
        (arr) = (TL_TYPEOF(arr))tl__r.data; \
    } while (0)

#define tl_arr_clear(arr) \
    do { \
        tl__arr_clear_impl((arr), TL_ALIGNOF(*(arr))); \
    } while (0)

#define tl_arr_free(arr) \
    do { \
        TL_REQUIRE_LVALUE(arr); \
        tl__arr_free_impl((arr), sizeof(*(arr)), TL_ALIGNOF(*(arr))); \
        (arr) = NULL; \
    } while (0)

#define tl_arr_reserve(arr, n) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        TL__ArrResult tl__r = tl__arr_reserve_impl((arr), NULL, sizeof(*(arr)), TL_ALIGNOF(*(arr)), (size_t)(n)); \
        if (tl__r.ok) (arr) = (TL_TYPEOF(arr))tl__r.data; \
        tl__r.ok; \
    )

#define tl_arr_resize(arr, n) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        TL__ArrResult tl__r = tl__arr_resize_impl((arr), sizeof(*(arr)), TL_ALIGNOF(*(arr)), (size_t)(n)); \
        if (tl__r.ok) (arr) = (TL_TYPEOF(arr))tl__r.data; \
        tl__r.ok; \
    )

/* Append and insertion helpers. */
#define tl_arr_push(arr, value) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        TL_TYPEOF(*(arr)) tl__value = (value); \
        TL__ArrResult tl__r = tl__arr_append_impl((arr), &tl__value, 1U, sizeof(tl__value), TL_ALIGNOF(tl__value)); \
        if (tl__r.ok) (arr) = (TL_TYPEOF(arr))tl__r.data; \
        tl__r.ok; \
    )

#define tl_arr_push_n(arr, ...) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        TL_TYPEOF(*(arr)) tl__items[] = { __VA_ARGS__ }; \
        TL__ArrResult tl__r = tl__arr_append_impl((arr), tl__items, TL_COUNT_OF(tl__items), sizeof(tl__items[0]), TL_ALIGNOF(tl__items[0])); \
        if (tl__r.ok) (arr) = (TL_TYPEOF(arr))tl__r.data; \
        tl__r.ok; \
    )

#define tl_arr_append(dst, src) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(dst); \
        const TL_TYPEOF(*(dst)) *tl__src_data = (src); \
        (void)tl__src_data; \
        TL__ArrResult tl__r = tl__arr_append_impl((dst), (src), tl_arr_len(src), sizeof(*(dst)), TL_ALIGNOF(*(dst))); \
        if (tl__r.ok) (dst) = (TL_TYPEOF(dst))tl__r.data; \
        tl__r.ok; \
    )

#define tl_arr_addnptr(arr, n) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        TL__ArrPtrResult tl__r = tl__arr_addnptr_impl((arr), sizeof(*(arr)), TL_ALIGNOF(*(arr)), (size_t)(n)); \
        if (tl__r.ok) (arr) = (TL_TYPEOF(arr))tl__r.data; \
        (TL_TYPEOF(arr))tl__r.ptr; \
    )

#define tl_arr_addnidx(arr, n) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        size_t tl__old_len = tl_arr_len(arr); \
        TL__ArrPtrResult tl__r = tl__arr_addnptr_impl((arr), sizeof(*(arr)), TL_ALIGNOF(*(arr)), (size_t)(n)); \
        if (tl__r.ok) (arr) = (TL_TYPEOF(arr))tl__r.data; \
        tl__r.ok ? tl__old_len : (size_t)-1; \
    )

#define tl_arr_insn(arr, idx, n) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        TL__ArrPtrResult tl__r = tl__arr_insn_impl((arr), sizeof(*(arr)), TL_ALIGNOF(*(arr)), (size_t)(idx), (size_t)(n)); \
        if (tl__r.ok) (arr) = (TL_TYPEOF(arr))tl__r.data; \
        (TL_TYPEOF(arr))tl__r.ptr; \
    )

#define tl_arr_ins(arr, idx, value) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(arr); \
        TL_TYPEOF(*(arr)) tl__value = (value); \
        TL_TYPEOF(arr) tl__slot = tl_arr_insn((arr), (idx), 1U); \
        if (tl__slot != NULL) *tl__slot = tl__value; \
        tl__slot != NULL; \
    )

#define tl_arr_pushp(arr) tl_arr_addnptr((arr), 1U)

/* Removal helpers. */
#define tl_arr_pop(arr, out_ptr) \
    TL_DS__EXPR( \
        TL_TYPEOF(arr) tl__out = (out_ptr); \
        tl__arr_pop_impl((arr), sizeof(*(arr)), TL_ALIGNOF(*(arr)), tl__out); \
    )

#define tl_arr_del(arr, idx) \
    TL_DS__EXPR( \
        tl__arr_deln_impl((arr), sizeof(*(arr)), TL_ALIGNOF(*(arr)), (size_t)(idx), 1U); \
    )

#define tl_arr_deln(arr, idx, n) \
    TL_DS__EXPR( \
        tl__arr_deln_impl((arr), sizeof(*(arr)), TL_ALIGNOF(*(arr)), (size_t)(idx), (size_t)(n)); \
    )

#define tl_arr_del_swap(arr, idx) \
    TL_DS__EXPR( \
        tl__arr_del_swap_impl((arr), sizeof(*(arr)), TL_ALIGNOF(*(arr)), (size_t)(idx)); \
    )

/* -------------------------------------------------------------------------- */
/* Hash map                                                                   */
/* -------------------------------------------------------------------------- */

#define TL_MAP_MIN_CAP 16U
#define TL_MAP_EMPTY 0U
#define TL_MAP_FULL  1U
#define TL_MAP_TOMB  2U

typedef u64_t (*TL_MapHashFn)(const void *key, size_t key_size);
typedef b32_t (*TL_MapEqFn)(const void *lhs, const void *rhs, size_t key_size);

typedef struct TL_Map {
    size_t len;
    size_t cap;
    size_t tombs;
    size_t key_size;
    size_t key_align;
    size_t key_stride;
    size_t value_size;
    size_t value_align;
    size_t value_stride;
    TL_Allocator *alloc;
    TL_MapHashFn hash;
    TL_MapEqFn eq;
    byte_t *keys;
    byte_t *values;
    byte_t *states;
} TL_Map;

TL_ATTR_MAYBE_UNUSED
static inline
u64_t
tl_hash_bytes(const void *data, size_t size)
{
    const byte_t *bytes = (const byte_t *)data;
    u64_t hash = UINT64_C(1469598103934665603);
    size_t i;

    for (i = 0; i < size; ++i) {
        hash ^= (u64_t)bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

TL_ATTR_MAYBE_UNUSED
static inline
u64_t
tl_hash_cstr(const char *str)
{
    return str ? tl_hash_bytes(str, strlen(str)) : 0U;
}

TL_ATTR_MAYBE_UNUSED
static inline
u64_t
tl_map_hash_bytes_key(const void *key, size_t key_size)
{
    return tl_hash_bytes(key, key_size);
}

TL_ATTR_MAYBE_UNUSED
static inline
b32_t
tl_map_eq_bytes_key(const void *lhs, const void *rhs, size_t key_size)
{
    return memcmp(lhs, rhs, key_size) == 0;
}

TL_ATTR_MAYBE_UNUSED
static inline
u64_t
tl_map_hash_strview_key(const void *key, size_t key_size)
{
    const TL_StrView *sv = (const TL_StrView *)key;
    (void)key_size;
    return tl_hash_bytes(sv->data + sv->beg, sv->end - sv->beg);
}

TL_ATTR_MAYBE_UNUSED
static inline
b32_t
tl_map_eq_strview_key(const void *lhs, const void *rhs, size_t key_size)
{
    const TL_StrView *a = (const TL_StrView *)lhs;
    const TL_StrView *b = (const TL_StrView *)rhs;
    size_t lena = a->end - a->beg;
    size_t lenb = b->end - b->beg;
    (void)key_size;
    if (lena != lenb) return 0;
    return memcmp(a->data + a->beg, b->data + b->beg, lena) == 0;
}

static inline
byte_t *
tl__map_key_at(const TL_Map *map, size_t idx)
{
    return map->keys + idx * map->key_stride;
}

static inline
byte_t *
tl__map_value_at(const TL_Map *map, size_t idx)
{
    return map->values + idx * map->value_stride;
}

static inline
TL_Allocator *
tl__map_allocator(const TL_Map *map)
{
    return (map && map->alloc) ? map->alloc : (TL_Allocator *)&tl_default_allocator;
}

static inline
b32_t
tl__map_should_grow(size_t used_slots, size_t cap)
{
    return cap == 0 || used_slots >= cap - cap / 4U;
}

typedef enum TL__MapRehashKind {
    TL__MAP_REHASH_NONE = 0,
    TL__MAP_REHASH_SAME_CAP,
    TL__MAP_REHASH_GROW,
} TL__MapRehashKind;

typedef struct TL__MapRehashDecision {
    TL__MapRehashKind kind;
    size_t new_cap;
    b32_t ok;
} TL__MapRehashDecision;

static inline
size_t
tl__map_next_cap(size_t need_len)
{
    size_t cap = TL_MAP_MIN_CAP;
    while (tl__map_should_grow(need_len, cap)) {
        if (cap > SIZE_MAX / 2U) return 0;
        cap *= 2U;
    }
    return cap;
}

static inline
TL__MapRehashDecision
tl__map_rehash_decision(const TL_Map *map, size_t target_live)
{
    TL__MapRehashDecision decision = { TL__MAP_REHASH_NONE, map->cap, 1 };
    size_t used_slots;

    if (map->tombs > SIZE_MAX - target_live) {
        decision.ok = 0;
        return decision;
    }

    used_slots = target_live + map->tombs;
    if (!tl__map_should_grow(used_slots, map->cap)) return decision;

    if (map->cap != 0 && !tl__map_should_grow(target_live, map->cap)) {
        decision.kind = TL__MAP_REHASH_SAME_CAP;
        return decision;
    }

    decision.kind = TL__MAP_REHASH_GROW;
    decision.new_cap = tl__map_next_cap(target_live);
    decision.ok = decision.new_cap != 0;
    return decision;
}

static inline
b32_t
tl__map_alloc_arrays(TL_Map *map, size_t cap)
{
    TL_Allocator *alloc = tl__map_allocator(map);
    size_t keys_size;
    size_t values_size;

    if (cap > SIZE_MAX / map->key_stride) return 0;
    if (cap > SIZE_MAX / map->value_stride) return 0;
    keys_size = cap * map->key_stride;
    values_size = cap * map->value_stride;

    map->keys = (byte_t *)tl_allocator_alloc_aligned(alloc, keys_size, map->key_align);
    if (!map->keys) return 0;

    map->values = (byte_t *)tl_allocator_alloc_aligned(alloc, values_size, map->value_align);
    if (!map->values) {
        tl_allocator_free_aligned(alloc, map->keys, keys_size, map->key_align);
        map->keys = NULL;
        return 0;
    }

    map->states = (byte_t *)tl_allocator_alloc_aligned(alloc, cap, TL_ALIGNOF(byte_t));
    if (!map->states) {
        tl_allocator_free_aligned(alloc, map->values, values_size, map->value_align);
        tl_allocator_free_aligned(alloc, map->keys, keys_size, map->key_align);
        map->keys = NULL;
        map->values = NULL;
        return 0;
    }

    memset(map->states, TL_MAP_EMPTY, cap);
    map->cap = cap;
    return 1;
}

static inline
void
tl__map_free_arrays(TL_Map *map)
{
    TL_Allocator *alloc = tl__map_allocator(map);

    if (!map || map->cap == 0) return;
    if (map->keys) {
        tl_allocator_free_aligned(alloc, map->keys, map->cap * map->key_stride, map->key_align);
    }
    if (map->values) {
        tl_allocator_free_aligned(alloc, map->values, map->cap * map->value_stride, map->value_align);
    }
    if (map->states) {
        tl_allocator_free_aligned(alloc, map->states, map->cap, TL_ALIGNOF(byte_t));
    }
    map->keys = NULL;
    map->values = NULL;
    map->states = NULL;
    map->cap = 0;
}

static inline
size_t
tl__map_find_entry(const TL_Map *map, const void *key)
{
    size_t idx;
    size_t i;

    if (map->cap == 0) return SIZE_MAX;

    idx = (size_t)(map->hash(key, map->key_size) & (u64_t)(map->cap - 1U));
    for (i = 0; i < map->cap; ++i) {
        byte_t state = map->states[idx];
        if (state == TL_MAP_EMPTY) return SIZE_MAX;
        if (state == TL_MAP_FULL && map->eq(tl__map_key_at(map, idx), key, map->key_size)) {
            return idx;
        }
        idx = (idx + 1U) & (map->cap - 1U);
    }

    return SIZE_MAX;
}

static inline
size_t
tl__map_find_insert_slot(const TL_Map *map, const void *key)
{
    size_t idx;
    size_t first_tomb = SIZE_MAX;
    size_t i;

    if (map->cap == 0) return SIZE_MAX;

    idx = (size_t)(map->hash(key, map->key_size) & (u64_t)(map->cap - 1U));
    for (i = 0; i < map->cap; ++i) {
        byte_t state = map->states[idx];
        if (state == TL_MAP_EMPTY) {
            return first_tomb != SIZE_MAX ? first_tomb : idx;
        }
        if (state == TL_MAP_TOMB) {
            if (first_tomb == SIZE_MAX) first_tomb = idx;
        } else if (map->eq(tl__map_key_at(map, idx), key, map->key_size)) {
            return idx;
        }
        idx = (idx + 1U) & (map->cap - 1U);
    }

    return first_tomb;
}

static inline
b32_t
tl__map_insert_no_grow(TL_Map *map, const void *key, const void *value)
{
    size_t idx = tl__map_find_insert_slot(map, key);

    if (idx == SIZE_MAX) return 0;
    if (map->states[idx] != TL_MAP_FULL) {
        if (map->states[idx] == TL_MAP_TOMB) map->tombs--;
        map->states[idx] = TL_MAP_FULL;
        map->len++;
        memcpy(tl__map_key_at(map, idx), key, map->key_size);
    }
    memcpy(tl__map_value_at(map, idx), value, map->value_size);
    return 1;
}

static inline
b32_t
tl__map_rehash(TL_Map *map, size_t new_cap)
{
    TL_Map next = *map;
    byte_t *old_keys = map->keys;
    byte_t *old_values = map->values;
    byte_t *old_states = map->states;
    size_t old_cap = map->cap;
    size_t i;

    next.len = 0;
    next.cap = 0;
    next.tombs = 0;
    next.keys = NULL;
    next.values = NULL;
    next.states = NULL;
    if (!tl__map_alloc_arrays(&next, new_cap)) return 0;

    for (i = 0; i < old_cap; ++i) {
        if (old_states[i] == TL_MAP_FULL &&
            !tl__map_insert_no_grow(&next,
                                    old_keys + i * map->key_stride,
                                    old_values + i * map->value_stride)) {
            tl__map_free_arrays(&next);
            return 0;
        }
    }

    map->keys = old_keys;
    map->values = old_values;
    map->states = old_states;
    map->cap = old_cap;
    tl__map_free_arrays(map);
    *map = next;
    return 1;
}

static inline
b32_t
tl__map_apply_rehash_decision(TL_Map *map, TL__MapRehashDecision decision)
{
    if (!decision.ok) return 0;

    switch (decision.kind) {
    case TL__MAP_REHASH_NONE:
        return 1;
    case TL__MAP_REHASH_SAME_CAP:
    case TL__MAP_REHASH_GROW:
        return tl__map_rehash(map, decision.new_cap);
    default:
        return 0;
    }
}

TL_ATTR_MAYBE_UNUSED
static inline
b32_t
tl_map_init_impl(TL_Map *map,
                 size_t key_size,
                 size_t key_align,
                 size_t value_size,
                 size_t value_align,
                 TL_Allocator *alloc,
                 TL_MapHashFn hash,
                 TL_MapEqFn eq)
{
    size_t key_stride;
    size_t value_stride;

    assert(map != NULL);
    key_align = tl_normalize_align(key_align);
    value_align = tl_normalize_align(value_align);
    if (key_size == 0 || value_size == 0 || key_align == 0 || value_align == 0) return 0;

    key_stride = tl_align_up(key_size, key_align);
    value_stride = tl_align_up(value_size, value_align);
    if (key_stride == 0 || value_stride == 0) return 0;

    map->len = 0;
    map->cap = 0;
    map->tombs = 0;
    map->key_size = key_size;
    map->key_align = key_align;
    map->key_stride = key_stride;
    map->value_size = value_size;
    map->value_align = value_align;
    map->value_stride = value_stride;
    map->alloc = alloc ? alloc : (TL_Allocator *)&tl_default_allocator;
    map->hash = hash ? hash : tl_map_hash_bytes_key;
    map->eq = eq ? eq : tl_map_eq_bytes_key;
    map->keys = NULL;
    map->values = NULL;
    map->states = NULL;
    return 1;
}

TL_ATTR_MAYBE_UNUSED
static inline
void
tl_map_free_impl(TL_Map *map)
{
    if (!map) return;
    tl__map_free_arrays(map);
    map->len = 0;
    map->tombs = 0;
}

TL_ATTR_MAYBE_UNUSED
static inline
b32_t
tl_map_reserve_impl(TL_Map *map, size_t need_len)
{
    TL__MapRehashDecision decision;

    assert(map != NULL);
    if (need_len <= map->len) return 1;

    decision = tl__map_rehash_decision(map, need_len);
    return tl__map_apply_rehash_decision(map, decision);
}

TL_ATTR_MAYBE_UNUSED
static inline
b32_t
tl_map_put_impl(TL_Map *map,
                const void *key,
                size_t key_size,
                size_t key_align,
                const void *value,
                size_t value_size,
                size_t value_align)
{
    size_t idx;
    TL__MapRehashDecision decision;

    assert(map != NULL);
    TL_DS_ASSERT(key != NULL, "map key must not be NULL");
    TL_DS_ASSERT(value != NULL, "map value must not be NULL");
    TL_DS_ASSERT(map->key_size == key_size, "map key size mismatch");
    TL_DS_ASSERT(map->value_size == value_size, "map value size mismatch");
    TL_DS_ASSERT(map->key_align == tl_normalize_align(key_align), "map key alignment mismatch");
    TL_DS_ASSERT(map->value_align == tl_normalize_align(value_align), "map value alignment mismatch");

    idx = tl__map_find_entry(map, key);
    if (idx != SIZE_MAX) {
        memcpy(tl__map_value_at(map, idx), value, map->value_size);
        return 1;
    }

    if (map->len == SIZE_MAX) return 0;
    decision = tl__map_rehash_decision(map, map->len + 1U);
    if (!tl__map_apply_rehash_decision(map, decision)) return 0;

    return tl__map_insert_no_grow(map, key, value);
}

TL_ATTR_MAYBE_UNUSED
static inline
const void *
tl_map_get_const_impl(const TL_Map *map, const void *key, size_t key_size, size_t key_align)
{
    size_t idx;

    assert(map != NULL);
    if (!map->cap) return NULL;
    TL_DS_ASSERT(key != NULL, "map key must not be NULL");
    TL_DS_ASSERT(map->key_size == key_size, "map key size mismatch");
    TL_DS_ASSERT(map->key_align == tl_normalize_align(key_align), "map key alignment mismatch");

    idx = tl__map_find_entry(map, key);
    return idx != SIZE_MAX ? tl__map_value_at(map, idx) : NULL;
}

TL_ATTR_MAYBE_UNUSED
static inline
void *
tl_map_get_mut_impl(TL_Map *map, const void *key, size_t key_size, size_t key_align)
{
    size_t idx;

    assert(map != NULL);
    if (!map->cap) return NULL;
    TL_DS_ASSERT(key != NULL, "map key must not be NULL");
    TL_DS_ASSERT(map->key_size == key_size, "map key size mismatch");
    TL_DS_ASSERT(map->key_align == tl_normalize_align(key_align), "map key alignment mismatch");

    idx = tl__map_find_entry(map, key);
    return idx != SIZE_MAX ? tl__map_value_at(map, idx) : NULL;
}

TL_ATTR_MAYBE_UNUSED
static inline
b32_t
tl_map_try_get_impl(const TL_Map *map,
                    const void *key,
                    size_t key_size,
                    size_t key_align,
                    void *out,
                    size_t out_size,
                    size_t out_align)
{
    const void *value;

    assert(map != NULL);
    TL_DS_ASSERT(out != NULL, "map try_get output must not be NULL");
    TL_DS_ASSERT(map->value_size == out_size, "map value size mismatch");
    TL_DS_ASSERT(map->value_align == tl_normalize_align(out_align), "map value alignment mismatch");

    value = tl_map_get_const_impl(map, key, key_size, key_align);
    if (!value) return 0;
    memcpy(out, value, out_size);
    return 1;
}

TL_ATTR_MAYBE_UNUSED
static inline
b32_t
tl_map_remove_impl(TL_Map *map, const void *key, size_t key_size, size_t key_align)
{
    size_t idx;

    assert(map != NULL);
    if (!map->cap) return 0;
    TL_DS_ASSERT(key != NULL, "map key must not be NULL");
    TL_DS_ASSERT(map->key_size == key_size, "map key size mismatch");
    TL_DS_ASSERT(map->key_align == tl_normalize_align(key_align), "map key alignment mismatch");

    idx = tl__map_find_entry(map, key);
    if (idx == SIZE_MAX) return 0;
    map->states[idx] = TL_MAP_TOMB;
    map->len--;
    map->tombs++;
    return 1;
}

#define tl_map_len(map) ((map).len)
#define tl_map_cap(map) ((map).cap)
#define tl_map_empty(map) (tl_map_len(map) == 0U)

/* Bytewise maps hash and compare the object representation of each key.
 * This is a good default for scalar keys and fully initialized POD-like keys.
 * Do not use it for struct keys with padding-sensitive equality semantics;
 * use tl_map_init_ex(...) with an explicit hash/eq pair instead. */
#define tl_map_init_bytewise(map, KeyType, ValueType, allocator) \
    do { \
        TL_REQUIRE_LVALUE(map); \
        (void)tl_map_init_impl(&(map), sizeof(KeyType), TL_ALIGNOF(KeyType), sizeof(ValueType), TL_ALIGNOF(ValueType), (allocator), NULL, NULL); \
    } while (0)

#define tl_map_init_ex(map, KeyType, ValueType, allocator, hash_fn, eq_fn) \
    do { \
        TL_REQUIRE_LVALUE(map); \
        (void)tl_map_init_impl(&(map), sizeof(KeyType), TL_ALIGNOF(KeyType), sizeof(ValueType), TL_ALIGNOF(ValueType), (allocator), (hash_fn), (eq_fn)); \
    } while (0)

#define tl_map_init_strview(map, ValueType, allocator) \
    tl_map_init_ex((map), TL_StrView, ValueType, (allocator), tl_map_hash_strview_key, tl_map_eq_strview_key)

#define tl_map_init_cstr(map, ValueType, allocator) \
    tl_map_init_strview((map), ValueType, (allocator))

#define tl_map_free(map) \
    do { \
        TL_REQUIRE_LVALUE(map); \
        tl_map_free_impl(&(map)); \
    } while (0)

#define tl_map_reserve(map, n) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(map); \
        tl_map_reserve_impl(&(map), (size_t)(n)); \
    )

#define tl_map_put(map, key, value) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(map); \
        TL_TYPEOF(key) tl__key = (key); \
        TL_TYPEOF(value) tl__value = (value); \
        tl_map_put_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key), &tl__value, sizeof(tl__value), TL_ALIGNOF(tl__value)); \
    )

#define tl_map_put_as(map, key, ValueType, value) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(map); \
        TL_TYPEOF(key) tl__key = (key); \
        ValueType tl__value = (value); \
        tl_map_put_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key), &tl__value, sizeof(tl__value), TL_ALIGNOF(tl__value)); \
    )

#define tl_map_get_const(map, key, ValueType) \
    TL_DS__EXPR( \
        TL_TYPEOF(key) tl__key = (key); \
        (ValueType const *)tl_map_get_const_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key)); \
    )

#define tl_map_get_mut(map, key, ValueType) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(map); \
        TL_TYPEOF(key) tl__key = (key); \
        (ValueType *)tl_map_get_mut_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key)); \
    )

#define tl_map_try_get(map, key, out_ptr) \
    TL_DS__EXPR( \
        TL_TYPEOF(key) tl__key = (key); \
        TL_TYPEOF(out_ptr) tl__out = (out_ptr); \
        tl_map_try_get_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key), tl__out, sizeof(*tl__out), TL_ALIGNOF(*tl__out)); \
    )

#define tl_map_contains(map, key) \
    TL_DS__EXPR( \
        TL_TYPEOF(key) tl__key = (key); \
        tl_map_get_const_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key)) != NULL; \
    )

#define tl_map_remove(map, key) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(map); \
        TL_TYPEOF(key) tl__key = (key); \
        tl_map_remove_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key)); \
    )

#define tl_map_put_cstr(map, key, value) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(map); \
        TL_StrView tl__key = { .data = (key), .beg = 0, .end = (key) ? strlen(key) : 0 }; \
        TL_TYPEOF(value) tl__value = (value); \
        tl_map_put_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key), &tl__value, sizeof(tl__value), TL_ALIGNOF(tl__value)); \
    )

#define tl_map_put_cstr_as(map, key, ValueType, value) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(map); \
        TL_StrView tl__key = { .data = (key), .beg = 0, .end = (key) ? strlen(key) : 0 }; \
        ValueType tl__value = (value); \
        tl_map_put_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key), &tl__value, sizeof(tl__value), TL_ALIGNOF(tl__value)); \
    )

#define tl_map_get_const_cstr(map, key, ValueType) \
    TL_DS__EXPR( \
        TL_StrView tl__key = { .data = (key), .beg = 0, .end = (key) ? strlen(key) : 0 }; \
        (ValueType const *)tl_map_get_const_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key)); \
    )

#define tl_map_get_mut_cstr(map, key, ValueType) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(map); \
        TL_StrView tl__key = { .data = (key), .beg = 0, .end = (key) ? strlen(key) : 0 }; \
        (ValueType *)tl_map_get_mut_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key)); \
    )

#define tl_map_try_get_cstr(map, key, out_ptr) \
    TL_DS__EXPR( \
        TL_TYPEOF(out_ptr) tl__out = (out_ptr); \
        TL_StrView tl__key = { .data = (key), .beg = 0, .end = (key) ? strlen(key) : 0 }; \
        tl_map_try_get_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key), tl__out, sizeof(*tl__out), TL_ALIGNOF(*tl__out)); \
    )

#define tl_map_contains_cstr(map, key) \
    TL_DS__EXPR( \
        TL_StrView tl__key = { .data = (key), .beg = 0, .end = (key) ? strlen(key) : 0 }; \
        tl_map_get_const_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key)) != NULL; \
    )

#define tl_map_remove_cstr(map, key) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(map); \
        TL_StrView tl__key = { .data = (key), .beg = 0, .end = (key) ? strlen(key) : 0 }; \
        tl_map_remove_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key)); \
    )

#define tl_map_get_const_strview(map, key, ValueType) \
    TL_DS__EXPR( \
        TL_StrView tl__key = (key); \
        (ValueType const *)tl_map_get_const_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key)); \
    )

#define tl_map_get_mut_strview(map, key, ValueType) \
    TL_DS__EXPR( \
        TL_REQUIRE_LVALUE(map); \
        TL_StrView tl__key = (key); \
        (ValueType *)tl_map_get_mut_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key)); \
    )

#define tl_map_try_get_strview(map, key, out_ptr) \
    TL_DS__EXPR( \
        TL_TYPEOF(out_ptr) tl__out = (out_ptr); \
        TL_StrView tl__key = (key); \
        tl_map_try_get_impl(&(map), &tl__key, sizeof(tl__key), TL_ALIGNOF(tl__key), tl__out, sizeof(*tl__out), TL_ALIGNOF(*tl__out)); \
    )

#if defined(TL_DS_SHORT_NAMES) || defined(TL_SHORT_NAMES)
/* Optional short aliases. */
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
#define Map            TL_Map
#define map_len        tl_map_len
#define map_cap        tl_map_cap
#define map_empty      tl_map_empty
#define map_init_bytewise tl_map_init_bytewise
#define map_init_ex    tl_map_init_ex
#define map_init_strview tl_map_init_strview
#define map_init_cstr  tl_map_init_cstr
#define map_free       tl_map_free
#define map_reserve    tl_map_reserve
#define map_put        tl_map_put
#define map_put_as     tl_map_put_as
#define map_get_const  tl_map_get_const
#define map_get_mut    tl_map_get_mut
#define map_try_get    tl_map_try_get
#define map_contains   tl_map_contains
#define map_remove     tl_map_remove
#define map_put_cstr   tl_map_put_cstr
#define map_put_cstr_as tl_map_put_cstr_as
#define map_get_const_cstr tl_map_get_const_cstr
#define map_get_mut_cstr tl_map_get_mut_cstr
#define map_try_get_cstr tl_map_try_get_cstr
#define map_contains_cstr tl_map_contains_cstr
#define map_remove_cstr tl_map_remove_cstr
#define map_get_const_strview tl_map_get_const_strview
#define map_get_mut_strview tl_map_get_mut_strview
#define map_try_get_strview tl_map_try_get_strview
#endif

#endif /* TINYLIB_DATA_STRUCT_H */
