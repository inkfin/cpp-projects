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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#ifndef TL_PRAGMA
#if defined(_MSC_VER)
#define TL_PRAGMA(x) __pragma(x)
#else
#define TL__DO_PRAGMA(x) _Pragma(#x)
#define TL_PRAGMA(x) TL__DO_PRAGMA(x)
#endif
#endif

#ifndef TL_UNIQUE_NAME
#define TL__UNIQUE_NAME2(base, n) base##n
#define TL__UNIQUE_NAME(base, n) TL__UNIQUE_NAME2(base, n)
#define TL_UNIQUE_NAME(base) TL__UNIQUE_NAME(base, __COUNTER__)
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

#if TL_HAS_GNU_EXTENSIONS
#define TL_ALIGNOF(x) __alignof__(x)
#elif defined(_MSC_VER)
#define TL_ALIGNOF(x) __alignof(x)
#else
#define TL_ALIGNOF(x) _Alignof(x)
#endif

#if TL_HAS_C23
#define TL_ALIGNAS(n) alignas(n)
#elif TL_HAS_GNU_EXTENSIONS
#define TL_ALIGNAS(n) __attribute__((aligned(n)))
#elif defined(_MSC_VER)
#define TL_ALIGNAS(n) __declspec(align(n))
#else
#define TL_ALIGNAS(n) _Alignas(n)
#endif

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

/* --- Constructor / Destructor auto-registration --- */

/* TL_ATTR_CONSTRUCTOR / TL_ATTR_DESTRUCTOR
 * GCC/Clang attributes to mark a function for automatic execution
 * before main() / after exit().
 *
 * These wrappers are attribute-only. They expand to nothing on MSVC and on
 * other compilers without GNU-style attributes.
 *
 * For cross-platform code prefer TL_REGISTER_INIT / TL_REGISTER_FINI.
 *
 * Example:
 *   static void TL_ATTR_CONSTRUCTOR my_ctor(void) { ... }
 *   static void TL_ATTR_DESTRUCTOR  my_dtor(void) { ... } */
#if TL_HAS_GNU_EXTENSIONS
#define TL_ATTR_CONSTRUCTOR __attribute__((constructor))
#define TL_ATTR_DESTRUCTOR  __attribute__((destructor))
#else
#define TL_ATTR_CONSTRUCTOR
#define TL_ATTR_DESTRUCTOR
#endif

/* TL_REGISTER_INIT(fn) / TL_REGISTER_FINI(fn)
 * Register a file-scope void(void) function to run automatically before main()
 * or after exit(), respectively.
 *
 * Usage:
 *   static void my_init(void) { ... }
 *   TL_REGISTER_INIT(my_init)
 *
 *   static void my_fini(void) { ... }
 *   TL_REGISTER_FINI(my_fini)
 *
 * Rules:
 *   - use only at file scope
 *   - the target function must have signature void fn(void)
 *   - do not add a trailing semicolon after TL_REGISTER_*
 *   - keep the registered work small and side-effect aware
 *   - do not rely on cross-translation-unit execution order
 *
 * Notes:
 *   - prefer these macros over TL_ATTR_CONSTRUCTOR / TL_ATTR_DESTRUCTOR when
 *     the code should also build on MSVC
 *   - unsupported compilers fail only when TL_REGISTER_* is used
 */
#if TL_HAS_GNU_EXTENSIONS || defined(_MSC_VER)
#define TL_HAS_CONSTRUCTOR_REGISTRATION 1
#else
#define TL_HAS_CONSTRUCTOR_REGISTRATION 0
#endif

#if TL_HAS_GNU_EXTENSIONS
#define TL_REGISTER_INIT(fn) \
    static void __attribute__((constructor)) TL_UNIQUE_NAME(tl__init)(void) { (fn)(); }
#define TL_REGISTER_FINI(fn) \
    static void __attribute__((destructor)) TL_UNIQUE_NAME(tl__fini)(void) { (fn)(); }

#elif defined(_MSC_VER)
#define TL_REGISTER_INIT(fn) \
    TL__MSVC_REGISTER(fn, TL_UNIQUE_NAME(tl__cinit), ".CRT$XCU")
#define TL_REGISTER_FINI(fn) \
    TL__MSVC_REGISTER(fn, TL_UNIQUE_NAME(tl__cfini), ".CRT$XPU")

/* MSVC walks .CRT$XCU before main() and .CRT$XPU during CRT teardown. */
#define TL__MSVC_REGISTER(fn, uniq, sect) \
    TL_PRAGMA(section(sect, read)) \
    __declspec(allocate(sect)) static void (*const uniq)(void) = (fn)

#else
#define TL_REGISTER_INIT(fn) \
    TL__UNSUPPORTED_CONSTRUCTOR()
#define TL_REGISTER_FINI(fn) \
    TL__UNSUPPORTED_CONSTRUCTOR()

#define TL__UNSUPPORTED_CONSTRUCTOR() \
    TL_STATIC_ASSERT(0, "TL_REGISTER_INIT/TL_REGISTER_FINI require GCC, Clang, or MSVC")
#endif

#if defined(TL_C_EXT_SHORT_NAMES) || defined(TL_SHORT_NAMES)
#ifndef count_of
#define count_of        TL_COUNT_OF
#endif
#ifndef require_lvalue
#define require_lvalue  TL_REQUIRE_LVALUE
#endif
#ifndef max
#define max             TL_MAX
#endif
#ifndef min
#define min             TL_MIN
#endif
#ifndef swap
#define swap            TL_SWAP
#endif
#ifndef panic
#define panic           TL_PANIC
#endif
#ifndef panic_msg
#define panic_msg       TL_PANIC_MSG
#endif
#ifndef unimplemented
#define unimplemented   TL_UNIMPLEMENTED
#endif
#ifndef todo
#define todo            TL_TODO
#endif
#ifndef breakpoint
#define breakpoint      TL_BREAKPOINT
#endif
#ifndef ALIGNOF
#define ALIGNOF         TL_ALIGNOF
#endif
#ifndef ALIGNAS
#define ALIGNAS         TL_ALIGNAS
#endif
#ifndef TYPEOF
#define TYPEOF          TL_TYPEOF
#endif
#ifndef HAS_TYPEOF
#define HAS_TYPEOF      TL_HAS_TYPEOF
#endif
#ifndef HAS_AUTO
#define HAS_AUTO        TL_HAS_AUTO
#endif
#ifndef HAS_C11
#define HAS_C11         TL_HAS_C11
#endif
#ifndef HAS_C23
#define HAS_C23         TL_HAS_C23
#endif
#ifndef HAS_GNU_EXTENSIONS
#define HAS_GNU_EXTENSIONS TL_HAS_GNU_EXTENSIONS
#endif
#ifndef HAS_STATEMENT_EXPR
#define HAS_STATEMENT_EXPR TL_HAS_STATEMENT_EXPR
#endif
#ifndef HAS_CONSTRUCTOR_REGISTRATION
#define HAS_CONSTRUCTOR_REGISTRATION TL_HAS_CONSTRUCTOR_REGISTRATION
#endif
#ifndef STATIC_ASSERT
#define STATIC_ASSERT   TL_STATIC_ASSERT
#endif
#ifndef THREAD_LOCAL
#define THREAD_LOCAL    TL_THREAD_LOCAL
#endif
#ifndef REGISTER_INIT
#define REGISTER_INIT   TL_REGISTER_INIT
#endif
#ifndef REGISTER_FINI
#define REGISTER_FINI   TL_REGISTER_FINI
#endif
#ifndef ATTR_CONSTRUCTOR
#define ATTR_CONSTRUCTOR TL_ATTR_CONSTRUCTOR
#endif
#ifndef ATTR_DESTRUCTOR
#define ATTR_DESTRUCTOR TL_ATTR_DESTRUCTOR
#endif
#endif

#endif /* TINYLIB_C_EXT_H */
