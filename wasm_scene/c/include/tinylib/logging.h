/* vim: set ft=c : -*- mode: c -*-
 * logging.h
 *   Simple global logging utilities for C projects.
 *
 *   Features:
 *     - Global logger with centralized configuration
 *     - Struct-based configuration API
 *     - Type-safe enums for level and output target
 *     - Thin logging macros that only inject source location
 *     - Optional file output
 *
 *   Notes:
 *     - Compile tinylib/logging.c, or include it in exactly one translation
 *       unit, to provide the implementation.
 *     - In C mode, this header requires C99 or later.
 *     - This header is intended to stay C99-portable even if other TinyLib
 *       components are tested/built with GNU11 settings.
 *     - Logging operations and configuration changes are serialized by an
 *       internal lock when the platform has thread primitives.
 *     - This logger currently manages a single global output file at most.
 *     - Reconfiguration is allowed through tl_log_reconfigure().
 */

#ifndef TINYLIB_LOGGING_H
#define TINYLIB_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__cplusplus)
#  if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
#    error "tinylib/logging.h requires C99 or later in C mode."
#  endif
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#if defined(__clang__) || defined(__GNUC__)
#define TL_LOG_ATTR_PRINTF(fmt_idx, first_arg_idx) __attribute__((format(printf, fmt_idx, first_arg_idx)))
#else
#define TL_LOG_ATTR_PRINTF(fmt_idx, first_arg_idx)
#endif

/*
 * tl_log_write() is a printf-style wrapper: callers pass a format string to
 * tl_log_write(), and the implementation forwards that already-checked string
 * to vfprintf(). With -Wformat=2, Clang/GCC warn on the vfprintf() call because
 * the local `fmt` parameter is not a string literal, even though that is the
 * correct shape for a va_list forwarding helper.
 *
 * Keep the public entry point annotated with TL_LOG_ATTR_PRINTF so compilers
 * still validate caller format strings and argument types. Then suppress only
 * the internal forwarding warning around vfprintf(), instead of disabling the
 * warning for the whole translation unit.
 *
 * MSVC does not support GCC/Clang's printf format attribute for this function,
 * but warning C4774 is the analogous "format string is not a string literal"
 * diagnostic, so the same narrow push/pop suppression is used there.
 */
#if defined(__clang__)
#define TL_LOG_DIAG_PUSH                  _Pragma("clang diagnostic push")
#define TL_LOG_DIAG_POP                   _Pragma("clang diagnostic pop")
#define TL_LOG_DIAG_IGNORE_FORMAT_NONLIT  _Pragma("clang diagnostic ignored \"-Wformat-nonliteral\"")
#elif defined(__GNUC__)
#define TL_LOG_DIAG_PUSH                  _Pragma("GCC diagnostic push")
#define TL_LOG_DIAG_POP                   _Pragma("GCC diagnostic pop")
#define TL_LOG_DIAG_IGNORE_FORMAT_NONLIT  _Pragma("GCC diagnostic ignored \"-Wformat-nonliteral\"")
#elif defined(_MSC_VER)
#define TL_LOG_DIAG_PUSH                  __pragma(warning(push))
#define TL_LOG_DIAG_POP                   __pragma(warning(pop))
#define TL_LOG_DIAG_IGNORE_FORMAT_NONLIT  __pragma(warning(disable: 4774))
#else
#define TL_LOG_DIAG_PUSH
#define TL_LOG_DIAG_POP
#define TL_LOG_DIAG_IGNORE_FORMAT_NONLIT
#endif

/* -------------------------------------------------------------------------- */
/* Types                                                                      */
/* -------------------------------------------------------------------------- */

typedef enum TL_LogLevel {
    TL_LOG_LEVEL_DEBUG = 0,
    TL_LOG_LEVEL_INFO  = 1,
    TL_LOG_LEVEL_WARN  = 2,
    TL_LOG_LEVEL_ERROR = 3,
} TL_LogLevel;

typedef enum TL_LogOutput {
    TL_LOG_OUTPUT_STDOUT = 0,
    TL_LOG_OUTPUT_STDERR = 1,
    TL_LOG_OUTPUT_FILE   = 2,
} TL_LogOutput;

typedef struct TL_LogConfig {
    /* Minimum enabled level. Messages below this level are ignored. */
    TL_LogLevel level;

    /* Default output target for non-error logs. */
    TL_LogOutput output;

    /* Output target for error logs. */
    TL_LogOutput error_output;

    /* Path to log file when output or error_output is TL_LOG_OUTPUT_FILE. */
    const char *filename;

    /* true: append to file. false: overwrite file. */
    bool append;

    /* true: flush after each log write. */
    bool auto_flush;

    /* Formatting switches. */
    bool show_time;
    bool show_level;
    bool show_file;
    bool show_line;
    bool show_func;
} TL_LogConfig;

/* Full default initializer for TL_LogConfig. */
#define TL_LOG_CONFIG_DEFAULT \
    (TL_LogConfig){ \
        .level = TL_LOG_LEVEL_INFO, \
        .output = TL_LOG_OUTPUT_STDOUT, \
        .error_output = TL_LOG_OUTPUT_STDERR, \
        .filename = NULL, \
        .append = true, \
        .auto_flush = true, \
        .show_time = true, \
        .show_level = true, \
        .show_file = true, \
        .show_line = true, \
        .show_func = true, \
    }

/* -------------------------------------------------------------------------- */
/* Configuration API                                                          */
/* -------------------------------------------------------------------------- */

/* Initializes the global logger with cfg.
 * If cfg is NULL, TL_LOG_CONFIG_DEFAULT is used.
 *
 * Returns non-zero on success, zero on failure.
 */
int
tl_log_init(const TL_LogConfig *cfg);

/* Reconfigures the global logger.
 * Safe to call after tl_log_init().
 *
 * Returns non-zero on success, zero on failure.
 */
int
tl_log_reconfigure(const TL_LogConfig *cfg);

/* Shuts down the global logger and releases internal resources.
 * Safe to call multiple times.
 */
void
tl_log_shutdown(void);

/* Flushes current output stream/file if possible.
 * Returns non-zero on success, zero on failure.
 */
int
tl_log_flush(void);

/* -------------------------------------------------------------------------- */
/* Focused setters/getters                                                    */
/* -------------------------------------------------------------------------- */

/* Sets minimum enabled log level. */
void
tl_log_set_level(TL_LogLevel level);

/* Returns current minimum enabled log level. */
TL_LogLevel
tl_log_get_level(void);

/* Returns a snapshot of current global configuration. */
TL_LogConfig
tl_log_get_config(void);

/* -------------------------------------------------------------------------- */
/* Logging API                                                                */
/* -------------------------------------------------------------------------- */

/* Core printf-style logging entry point. */
void
tl_log_write(TL_LogLevel level,
             const char *file,
             int line,
             const char *func,
             const char *fmt,
             ...) TL_LOG_ATTR_PRINTF(5, 6);

/* Optional raw message entry point.
 * Writes a preformatted message without printf-style formatting.
 */
void
tl_log_write_raw(TL_LogLevel level,
                 const char *file,
                 int line,
                 const char *func,
                 const char *msg);

/* Returns non-zero if the given level is currently enabled. */
int
tl_log_is_enabled(TL_LogLevel level);

/* -------------------------------------------------------------------------- */
/* Logging macros                                                             */
/* -------------------------------------------------------------------------- */

#define TL_LOG_DEBUG(...) \
    tl_log_write(TL_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define TL_LOG_INFO(...) \
    tl_log_write(TL_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define TL_LOG_WARN(...) \
    tl_log_write(TL_LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define TL_LOG_ERROR(...) \
    tl_log_write(TL_LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)

/* Optional short aliases. */
#if defined(TL_LOG_SHORT_NAMES) || defined(TL_SHORT_NAMES)
#  if defined(__APPLE__)
/* macOS <sys/syslog.h> defines LOG_DEBUG/INFO/WARN/ERROR as ints */
#    undef LOG_DEBUG
#    undef LOG_INFO
#    undef LOG_WARN
#    undef LOG_ERROR
#  endif
#  define LOG_DEBUG(...) TL_LOG_DEBUG(__VA_ARGS__)
#  define LOG_INFO(...)  TL_LOG_INFO(__VA_ARGS__)
#  define LOG_WARN(...)  TL_LOG_WARN(__VA_ARGS__)
#  define LOG_ERROR(...) TL_LOG_ERROR(__VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif

#endif /* TINYLIB_LOGGING_H */
