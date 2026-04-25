/* vim: set ft=c : -*- mode: c -*-
 * macros.h
 *   TinyLib Macro Utilities (macros.h)
 *
 *   Usage:
 *     - Token concatenation helpers: TL_CONCAT2..TL_CONCAT6
 *     - Foreach expansion helpers:
 *         TL_FOREACH(F, SEP, ...)
 *         TL_FOREACH_ONE_PARAM(F, SEP, fixed1, ...)
 *         TL_FOREACH_TWO_PARAM(F, SEP, fixed1, fixed2, ...)
 *         TL_FOREACH_F* variants use ';' as separator.
 *     - Unimplemented marker:
 *         TL_TODO("message")  // emits pragma message and triggers TL_BREAKPOINT()
 *
 *   Notes:
 *     - Supported compilers: MSVC, Clang, GCC.
 *     - C standard: requires C99 or newer (MSVC allowed via _MSC_VER check).
 *     - Foreach supports 1..12 variadic elements; 0 and overflow are invalid.
 *     - Overflow paths emit a compile-time pragma error.
 */
#ifndef TINYLIB_MACROS_H
#define TINYLIB_MACROS_H


#if !defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
#error "TinyLib macros.h supports only MSVC, Clang, and GCC."
#endif

#if !defined(_MSC_VER) && (!defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L))
#error "TinyLib macros.h requires C99 or newer."
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

#ifndef TL_TODO
#define TL_TODO(msg) \
    do { TL_PRAGMA(message("TODO: " msg)); TL_BREAKPOINT(); } while (0)
#endif


/** Concat macros **/

#define TL__CONCAT2(a, b) a##b
#define TL_CONCAT2(a, b) TL__CONCAT2(a, b)
#define TL__CONCAT3(a, b, c) a##b##c
#define TL_CONCAT3(a, b, c) TL__CONCAT3(a, b, c)
#define TL__CONCAT4(a, b, c, d) a##b##c##d
#define TL_CONCAT4(a, b, c, d) TL__CONCAT4(a, b, c, d)
#define TL__CONCAT5(a, b, c, d, e) a##b##c##d##e
#define TL_CONCAT5(a, b, c, d, e) TL__CONCAT5(a, b, c, d, e)
#define TL__CONCAT6(a, b, c, d, e, f) a##b##c##d##e##f
#define TL_CONCAT6(a, b, c, d, e, f) TL__CONCAT6(a, b, c, d, e, f)


/** unique name **/

#ifndef TL_UNIQUE_NAME
#define TL_UNIQUE_NAME(base) TL_CONCAT2(base, __COUNTER__)
#endif


/** Foreach macros **/

/// support 1..12 variadic arguments
/// example:
///     TL_FOREACH(puts, ;, "hello", "world!");
///     TL_FOREACH_ONE_PARAM(printf, ;, "Hello, %s!\n", "Bob", "Tom");
/// note:
///     0 variadic arguments are not supported.

#define TL__NUM_VA_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, n, ...) n
#define TL_NUM_VA_ARGS(...) TL__NUM_VA_ARGS_HELPER(__VA_ARGS__, OVERFLOW, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#if defined(_MSC_VER)
#define TL__FOREACH_INTERFACE_BROKEN() \
    TL_PRAGMA(message("error: TL_FOREACH interface is broken: variadic argument count must be 1..12")) \
    ((void)sizeof(char[-1]))
#else
#define TL__FOREACH_INTERFACE_BROKEN() \
    TL_PRAGMA(GCC error "TL_FOREACH interface is broken: variadic argument count must be 1..12") \
    ((void)sizeof(char[-1]))
#endif

// two parameters

#define TL__EXPAND_ARGS_HELPER_TWO_PARAM_0(F, SEP, _param1, _param2, ...) \
    TL__FOREACH_INTERFACE_BROKEN()
#define TL__EXPAND_ARGS_HELPER_TWO_PARAM_1(F, SEP, _param1, _param2, _first) \
    F(_param1, _param2, _first)
#define TL__EXPAND_ARGS_HELPER_TWO_PARAM_2(F, SEP, _param1, _param2, _first, ...) \
    F(_param1, _param2, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_TWO_PARAM_1(F, SEP, _param1, _param2, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_TWO_PARAM_3(F, SEP, _param1, _param2, _first, ...) \
    F(_param1, _param2, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_TWO_PARAM_2(F, SEP, _param1, _param2, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_TWO_PARAM_4(F, SEP, _param1, _param2, _first, ...) \
    F(_param1, _param2, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_TWO_PARAM_3(F, SEP, _param1, _param2, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_TWO_PARAM_5(F, SEP, _param1, _param2, _first, ...) \
    F(_param1, _param2, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_TWO_PARAM_4(F, SEP, _param1, _param2, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_TWO_PARAM_6(F, SEP, _param1, _param2, _first, ...) \
    F(_param1, _param2, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_TWO_PARAM_5(F, SEP, _param1, _param2, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_TWO_PARAM_7(F, SEP, _param1, _param2, _first, ...) \
    F(_param1, _param2, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_TWO_PARAM_6(F, SEP, _param1, _param2, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_TWO_PARAM_8(F, SEP, _param1, _param2, _first, ...) \
    F(_param1, _param2, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_TWO_PARAM_7(F, SEP, _param1, _param2, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_TWO_PARAM_9(F, SEP, _param1, _param2, _first, ...) \
    F(_param1, _param2, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_TWO_PARAM_8(F, SEP, _param1, _param2, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_TWO_PARAM_10(F, SEP, _param1, _param2, _first, ...) \
    F(_param1, _param2, _first)SEP                                                 \
    TL__EXPAND_ARGS_HELPER_TWO_PARAM_9(F, SEP, _param1, _param2, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_TWO_PARAM_11(F, SEP, _param1, _param2, _first, ...) \
    F(_param1, _param2, _first)SEP                                                 \
    TL__EXPAND_ARGS_HELPER_TWO_PARAM_10(F, SEP, _param1, _param2, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_TWO_PARAM_12(F, SEP, _param1, _param2, _first, ...) \
    F(_param1, _param2, _first)SEP                                                 \
    TL__EXPAND_ARGS_HELPER_TWO_PARAM_11(F, SEP, _param1, _param2, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_TWO_PARAM_OVERFLOW(F, SEP, _param1, _param2, ...) \
    TL__FOREACH_INTERFACE_BROKEN()

// one parameter

#define TL__EXPAND_ARGS_HELPER_ONE_PARAM_0(F, SEP, _param1, ...) \
    TL__FOREACH_INTERFACE_BROKEN()
#define TL__EXPAND_ARGS_HELPER_ONE_PARAM_1(F, SEP, _param1, _first) \
    F(_param1, _first)
#define TL__EXPAND_ARGS_HELPER_ONE_PARAM_2(F, SEP, _param1, _first, ...) \
    F(_param1, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_ONE_PARAM_1(F, SEP, _param1, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_ONE_PARAM_3(F, SEP, _param1, _first, ...) \
    F(_param1, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_ONE_PARAM_2(F, SEP, _param1, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_ONE_PARAM_4(F, SEP, _param1, _first, ...) \
    F(_param1, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_ONE_PARAM_3(F, SEP, _param1, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_ONE_PARAM_5(F, SEP, _param1, _first, ...) \
    F(_param1, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_ONE_PARAM_4(F, SEP, _param1, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_ONE_PARAM_6(F, SEP, _param1, _first, ...) \
    F(_param1, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_ONE_PARAM_5(F, SEP, _param1, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_ONE_PARAM_7(F, SEP, _param1, _first, ...) \
    F(_param1, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_ONE_PARAM_6(F, SEP, _param1, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_ONE_PARAM_8(F, SEP, _param1, _first, ...) \
    F(_param1, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_ONE_PARAM_7(F, SEP, _param1, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_ONE_PARAM_9(F, SEP, _param1, _first, ...) \
    F(_param1, _first)SEP                                                \
    TL__EXPAND_ARGS_HELPER_ONE_PARAM_8(F, SEP, _param1, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_ONE_PARAM_10(F, SEP, _param1, _first, ...) \
    F(_param1, _first)SEP                                                 \
    TL__EXPAND_ARGS_HELPER_ONE_PARAM_9(F, SEP, _param1, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_ONE_PARAM_11(F, SEP, _param1, _first, ...) \
    F(_param1, _first)SEP                                                 \
    TL__EXPAND_ARGS_HELPER_ONE_PARAM_10(F, SEP, _param1, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_ONE_PARAM_12(F, SEP, _param1, _first, ...) \
    F(_param1, _first)SEP                                                 \
    TL__EXPAND_ARGS_HELPER_ONE_PARAM_11(F, SEP, _param1, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_ONE_PARAM_OVERFLOW(F, SEP, _param1, ...) \
    TL__FOREACH_INTERFACE_BROKEN()

// no parameter

#define TL__EXPAND_ARGS_HELPER_0(F, SEP, ...) \
    TL__FOREACH_INTERFACE_BROKEN()
#define TL__EXPAND_ARGS_HELPER_1(F, SEP, _first) \
    F(_first)
#define TL__EXPAND_ARGS_HELPER_2(F, SEP, _first, ...) \
    F(_first)SEP                                      \
    TL__EXPAND_ARGS_HELPER_1(F, SEP, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_3(F, SEP, _first, ...) \
    F(_first)SEP                                      \
    TL__EXPAND_ARGS_HELPER_2(F, SEP, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_4(F, SEP, _first, ...) \
    F(_first)SEP                                      \
    TL__EXPAND_ARGS_HELPER_3(F, SEP, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_5(F, SEP, _first, ...) \
    F(_first)SEP                                      \
    TL__EXPAND_ARGS_HELPER_4(F, SEP, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_6(F, SEP, _first, ...) \
    F(_first)SEP                                      \
    TL__EXPAND_ARGS_HELPER_5(F, SEP, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_7(F, SEP, _first, ...) \
    F(_first)SEP                                      \
    TL__EXPAND_ARGS_HELPER_6(F, SEP, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_8(F, SEP, _first, ...) \
    F(_first)SEP                                      \
    TL__EXPAND_ARGS_HELPER_7(F, SEP, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_9(F, SEP, _first, ...) \
    F(_first)SEP                                      \
    TL__EXPAND_ARGS_HELPER_8(F, SEP, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_10(F, SEP, _first, ...) \
    F(_first)SEP                                       \
    TL__EXPAND_ARGS_HELPER_9(F, SEP, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_11(F, SEP, _first, ...) \
    F(_first)SEP                                       \
    TL__EXPAND_ARGS_HELPER_10(F, SEP, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_12(F, SEP, _first, ...) \
    F(_first)SEP                                       \
    TL__EXPAND_ARGS_HELPER_11(F, SEP, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_OVERFLOW(F, SEP, ...) \
    TL__FOREACH_INTERFACE_BROKEN()

#define TL_EMPTY()
#define TL_DEFER(m) m TL_EMPTY()
#define TL_EVAL(m) m

#define TL__EXPAND_ARGS_HELPER_SELECTOR_TWO_PARAM(F, SEP, _param1, _param2, n, ...) \
    TL_CONCAT2(TL__EXPAND_ARGS_HELPER_TWO_PARAM_, n)(F, SEP, _param1, _param2, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_SELECTOR_ONE_PARAM(F, SEP, _param1, n, ...) \
    TL_CONCAT2(TL__EXPAND_ARGS_HELPER_ONE_PARAM_, n)(F, SEP, _param1, __VA_ARGS__)
#define TL__EXPAND_ARGS_HELPER_SELECTOR(F, SEP, n, ...) \
    TL_CONCAT2(TL__EXPAND_ARGS_HELPER_, n)(F, SEP, __VA_ARGS__)

#define TL_FOREACH_TWO_PARAM(F, SEP, _param1, _param2, ...) \
    TL_EVAL(TL_DEFER(TL__EXPAND_ARGS_HELPER_SELECTOR_TWO_PARAM)(F, SEP, _param1, _param2, TL_NUM_VA_ARGS(__VA_ARGS__), __VA_ARGS__))

#define TL_FOREACH_ONE_PARAM(F, SEP, _param1, ...) \
    TL_EVAL(TL_DEFER(TL__EXPAND_ARGS_HELPER_SELECTOR_ONE_PARAM)(F, SEP, _param1, TL_NUM_VA_ARGS(__VA_ARGS__), __VA_ARGS__))

#define TL_FOREACH(F, SEP, ...) \
    TL_EVAL(TL_DEFER(TL__EXPAND_ARGS_HELPER_SELECTOR)(F, SEP, TL_NUM_VA_ARGS(__VA_ARGS__), __VA_ARGS__))

#define TL_FOREACH_F(F, ...) TL_FOREACH(F, ;, __VA_ARGS__)
#define TL_FOREACH_F_ONE_PARAM(F, ...) TL_FOREACH_ONE_PARAM(F, ;, __VA_ARGS__)
#define TL_FOREACH_F_TWO_PARAM(F, ...) TL_FOREACH_TWO_PARAM(F, ;, __VA_ARGS__)


#endif /* TINYLIB_MACROS_H */
