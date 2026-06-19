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
 *   How to start:
 *
 *     TL_LogConfig cfg = {0};
 *     tl_log_init(&cfg);
 *     TL_LOG_INFO("started");
 *
 *   Zero-initialized TL_LogConfig is the default configuration:
 *
 *     - level:        INFO and above
 *     - output:       stdout for non-error logs
 *     - error_output: stderr for errors
 *     - file mode:    overwrite if file output is selected
 *     - flushing:     enabled after every write
 *     - prefix:       time, level, file, line, and function
 *
 *   To customize, start from {0} and set only the fields that differ:
 *
 *     TL_LogConfig cfg = {0};
 *     cfg.level = TL_LOG_LEVEL_DEBUG;
 *     cfg.output = TL_LOG_OUTPUT_FILE;
 *     cfg.error_output = TL_LOG_OUTPUT_FILE;
 *     cfg.filename = "app.log";
 *     cfg.disable_file = 1;
 *     cfg.disable_line = 1;
 *     cfg.disable_func = 1;
 *     tl_log_init(&cfg);
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
    TL_LOG_LEVEL_DEBUG = -1,
    TL_LOG_LEVEL_INFO  = 0,
    TL_LOG_LEVEL_WARN  = 1,
    TL_LOG_LEVEL_ERROR = 2,
} TL_LogLevel;

typedef enum TL_LogOutput {
    TL_LOG_OUTPUT_DEFAULT = 0,
    TL_LOG_OUTPUT_STDOUT  = 1,
    TL_LOG_OUTPUT_STDERR  = 2,
    TL_LOG_OUTPUT_FILE    = 3,
} TL_LogOutput;

typedef struct TL_LogConfig {
    /* Minimum enabled level. Default: TL_LOG_LEVEL_INFO. */
    TL_LogLevel level;

    /* Output target for DEBUG/LOG/WARN messages. DEFAULT resolves to stdout. */
    TL_LogOutput output;

    /* Output target for ERROR messages. DEFAULT resolves to stderr. */
    TL_LogOutput error_output;

    /* Path when either output field is TL_LOG_OUTPUT_FILE. Default: NULL. */
    const char *filename;

    /* File open mode. Default: 0 overwrites; non-zero appends. */
    int append;

    /* Disable switches.
     *
     * These are intentionally negative flags so TL_LogConfig cfg = {0} is a
     * complete, useful default configuration. Leave a field at 0 to keep that
     * behavior enabled; set it non-zero to turn that behavior off.
     *
     * Default output shape:
     *     [YYYY-MM-DD HH:MM:SS] [INFO] file.c:42 function(): message
     */

    /* Default: 0, flush after each log write. */
    int disable_auto_flush;

    /* Default: 0, include timestamp prefix. */
    int disable_time;

    /* Default: 0, include level prefix. */
    int disable_level;

    /* Default: 0, include source file path when provided. */
    int disable_file;

    /* Default: 0, include source line. */
    int disable_line;

    /* Default: 0, include function name when provided. */
    int disable_func;
} TL_LogConfig;

/* -------------------------------------------------------------------------- */
/* Configuration API                                                          */
/* -------------------------------------------------------------------------- */

/* Initializes the global logger with cfg.
 * If cfg is NULL, the zero-initialized default configuration is used.
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
