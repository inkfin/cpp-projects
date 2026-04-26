/* vim: set ft=c : -*- mode: c -*-
 * c_ext.h
 *   Modern C and compiler-extension helpers.
 *
 *   Scope:
 *     - feature detection for C11/C23 and common GNU/Clang extensions
 *     - small language-level helpers such as typeof/auto/static_assert
 *     - attribute wrappers with C23-first, GNU fallback, MSVC-empty behavior
 *
 *   Design rules:
 *     - this header should be permissive to include
 *     - unsupported optional features are exposed through TL_HAS_* macros
 *     - hard errors are reserved for baseline language requirements only
 *     - consumers that truly require a feature should check TL_HAS_* themselves
 *
 *   Notes:
 *     - TL_AUTO maps to C23 `auto` when available, otherwise GNU `__auto_type`
 *     - statement expressions are intentionally not wrapped as a public helper here;
 *       headers that rely on them should gate and wrap them privately
 */
#ifndef TINYLIB_C_EXT_H
#define TINYLIB_C_EXT_H

#include <stddef.h>

#if defined(__cplusplus)
#error "This header is only for C."
#endif

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201112L)
#error "TinyLib requires C11 or later."
#endif

#define TL_HAS_C11 1
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202311L)
#define TL_HAS_C23 1
#else
#define TL_HAS_C23 0
#endif

#ifndef TL_UNIQUE_NAME
#define TL__UNIQUE_NAME2(base, n) base##n
#define TL__UNIQUE_NAME(base, n) TL__UNIQUE_NAME2(base, n)
#define TL_UNIQUE_NAME(base) TL__UNIQUE_NAME(base, __COUNTER__)
#endif

#ifndef TL_PRAGMA
#if defined(_MSC_VER)
#define TL_PRAGMA(x) __pragma(x)
#else
#define TL__DO_PRAGMA(x) _Pragma(#x)
#define TL_PRAGMA(x) TL__DO_PRAGMA(x)
#endif
#endif

#ifndef TL_BREAKPOINT
#if defined(_MSC_VER)
#define TL_BREAKPOINT() __debugbreak()
#elif defined(__clang__)
#define TL_BREAKPOINT() __builtin_debugtrap()
#else
#define TL_BREAKPOINT() __builtin_trap()
#endif
#endif

#define TL_PANIC()                                \
    do {                                          \
        fprintf(stderr, "panic at %s:%d in %s\n", \
                __FILE__, __LINE__, __func__);    \
        abort();                                  \
    } while (0)

#define TL_PANIC_MSG(msg)                             \
    do {                                              \
        fprintf(stderr, "panic at %s:%d in %s: %s\n", \
                __FILE__, __LINE__, __func__, (msg)); \
        abort();                                      \
    } while (0)

#define TL_UNIMPLEMENTED(msg) \
    do { TL_PRAGMA(message("Unimplemented: " msg)); abort(); } while (0)

#ifndef TL_TODO
#define TL_TODO(msg) \
    do { TL_PRAGMA(message("TODO: " msg)); TL_BREAKPOINT(); } while (0)
#endif

#if defined(__has_c_attribute)
#define TL__HAS_C_ATTRIBUTE(attr) __has_c_attribute(attr)
#else
#define TL__HAS_C_ATTRIBUTE(attr) 0
#endif

#if defined(__has_attribute)
#define TL__HAS_ATTRIBUTE(attr) __has_attribute(attr)
#else
#define TL__HAS_ATTRIBUTE(attr) 0
#endif

#if defined(__GNUC__) || defined(__clang__)
#define TL_HAS_GNU_EXTENSIONS 1
#else
#define TL_HAS_GNU_EXTENSIONS 0
#endif

#if TL_HAS_C23
#define TL_STATIC_ASSERT(cond, msg) static_assert((cond), msg)
#else
#define TL_STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)
#endif

#define TL_COUNT_OF(a) (sizeof(a) / sizeof((a)[0]))
#define TL_REQUIRE_LVALUE(x) ((void) &(x))

#if TL_HAS_C23
#define TL_HAS_TYPEOF 1
#define TL_TYPEOF(expr) typeof(expr)
#elif TL_HAS_GNU_EXTENSIONS
#define TL_HAS_TYPEOF 1
#define TL_TYPEOF(expr) __typeof__(expr)
#else
#define TL_HAS_TYPEOF 0
#endif

#if TL_HAS_C23
#define TL_THREAD_LOCAL thread_local
#else
#define TL_THREAD_LOCAL _Thread_local
#endif

#if TL_HAS_C23
#define TL_HAS_AUTO 1
#define TL_AUTO auto
#elif TL_HAS_GNU_EXTENSIONS
#define TL_HAS_AUTO 1
#define TL_AUTO __auto_type
#else
#define TL_HAS_AUTO 0
#endif

#if TL_HAS_GNU_EXTENSIONS
#define TL_HAS_STATEMENT_EXPR 1
#else
#define TL_HAS_STATEMENT_EXPR 0
#endif

#define TL_MAX(a, b) ((a) > (b) ? (a) : (b))
#define TL_MIN(a, b) ((a) < (b) ? (a) : (b))
#if TL_HAS_TYPEOF
#define TL_SWAP(a, b)                \
    do {                             \
        TL_TYPEOF(a) tl__tmp = (a);  \
        (a) = (b);                   \
        (b) = tl__tmp;               \
    } while (0)
#else
#define TL_SWAP(a, b)                      \
    do {                                   \
        char tl__tmp[sizeof(a)];           \
        memcpy(tl__tmp, &(a), sizeof(a));  \
        memcpy(&(a), &(b), sizeof(a));     \
        memcpy(&(b), tl__tmp, sizeof(a));  \
    } while (0)
#endif

#if TL_HAS_C23 && TL__HAS_C_ATTRIBUTE(maybe_unused)
#define TL_ATTR_MAYBE_UNUSED [[maybe_unused]]
#elif TL__HAS_ATTRIBUTE(unused)
#define TL_ATTR_MAYBE_UNUSED __attribute__((unused))
#elif defined(_MSC_VER)
#define TL_ATTR_MAYBE_UNUSED
#else
#define TL_ATTR_MAYBE_UNUSED
#endif

#if TL_HAS_C23 && TL__HAS_C_ATTRIBUTE(nodiscard)
#define TL_ATTR_NODISCARD [[nodiscard]]
#elif TL__HAS_ATTRIBUTE(warn_unused_result)
#define TL_ATTR_NODISCARD __attribute__((warn_unused_result))
#elif defined(_MSC_VER)
#define TL_ATTR_NODISCARD
#else
#define TL_ATTR_NODISCARD
#endif

#if TL_HAS_C23 && TL__HAS_C_ATTRIBUTE(noreturn)
#define TL_ATTR_NORETURN [[noreturn]]
#elif TL__HAS_ATTRIBUTE(noreturn)
#define TL_ATTR_NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define TL_ATTR_NORETURN
#else
#define TL_ATTR_NORETURN
#endif

#if TL_HAS_C23 && TL__HAS_C_ATTRIBUTE(fallthrough)
#define TL_ATTR_FALLTHROUGH [[fallthrough]]
#elif TL__HAS_ATTRIBUTE(fallthrough)
#define TL_ATTR_FALLTHROUGH __attribute__((fallthrough))
#elif defined(_MSC_VER)
#define TL_ATTR_FALLTHROUGH
#else
#define TL_ATTR_FALLTHROUGH
#endif

#if TL__HAS_ATTRIBUTE(always_inline)
#define TL_ATTR_ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
#define TL_ATTR_ALWAYS_INLINE
#else
#define TL_ATTR_ALWAYS_INLINE
#endif

#if TL__HAS_ATTRIBUTE(noinline)
#define TL_ATTR_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define TL_ATTR_NOINLINE
#else
#define TL_ATTR_NOINLINE
#endif

#if TL__HAS_ATTRIBUTE(returns_nonnull)
#define TL_ATTR_RETURNS_NONNULL __attribute__((returns_nonnull))
#elif defined(_MSC_VER)
#define TL_ATTR_RETURNS_NONNULL
#else
#define TL_ATTR_RETURNS_NONNULL
#endif

#if TL__HAS_ATTRIBUTE(nonnull)
#define TL_ATTR_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#elif defined(_MSC_VER)
#define TL_ATTR_NONNULL(...)
#else
#define TL_ATTR_NONNULL(...)
#endif

#endif /* TINYLIB_C_EXT_H */
