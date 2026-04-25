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
 *     - Define TL_LOG_IMPLEMENTATION in exactly one .c file before including
 *       this header to provide the implementation.
 *     - In C mode, this header requires C99 or later.
 *     - This header is intended to stay C99-portable even if other TinyLib
 *       components are tested/built with GNU11 settings.
 *     - This logger currently manages a single global output file at most.
 *     - Reconfiguration is allowed through tl_log_reconfigure().
 */

#ifndef TINYLIB_LOG_H
#define TINYLIB_LOG_H

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

/* Returns a pointer to current global configuration.
 * The returned pointer must be treated as read-only.
 */
const TL_LogConfig *
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
#ifdef TL_LOG_ABBR
#  define LOG_DEBUG(...) TL_LOG_DEBUG(__VA_ARGS__)
#  define LOG_INFO(...)  TL_LOG_INFO(__VA_ARGS__)
#  define LOG_WARN(...)  TL_LOG_WARN(__VA_ARGS__)
#  define LOG_ERROR(...) TL_LOG_ERROR(__VA_ARGS__)
#endif

/* -------------------------------------------------------------------------- */
/* Implementation                                                             */
/* -------------------------------------------------------------------------- */

#if defined(TL_LOG_IMPLEMENTATION)

#include <string.h>
#include <time.h>

typedef struct TL_LogState {
    TL_LogConfig config;
    FILE        *file;
    int          initialized;
} TL_LogState;

static TL_LogState g__log_state = {0};

/* ---------------------------------------------------------- */
/* helpers                                                    */
/* ---------------------------------------------------------- */

static TL_LogLevel
tl__sanitize_level(TL_LogLevel level)
{
    if (level < TL_LOG_LEVEL_DEBUG) return TL_LOG_LEVEL_DEBUG;
    if (level > TL_LOG_LEVEL_ERROR) return TL_LOG_LEVEL_ERROR;
    return level;
}

static FILE *
tl__resolve_stream(TL_LogOutput out)
{
    switch (out) {
    case TL_LOG_OUTPUT_STDOUT:
        return stdout;

    case TL_LOG_OUTPUT_STDERR:
        return stderr;

    case TL_LOG_OUTPUT_FILE:
        if (g__log_state.file)
            return g__log_state.file;
        return stderr;
    }

    return stderr;
}

static FILE *
tl__select_stream(TL_LogLevel level)
{
    if (level >= TL_LOG_LEVEL_ERROR)
        return tl__resolve_stream(g__log_state.config.error_output);
    return tl__resolve_stream(g__log_state.config.output);
}

/* Convert local time in a C99-friendly way.
 * Prefer thread-safe APIs when available; otherwise copy from localtime(). */
static int
tl__to_local_time(time_t t, struct tm *out)
{
#if defined(_WIN32)
    return localtime_s(out, &t) == 0;
#elif defined(__unix__) || defined(__unix) || \
    (defined(__APPLE__) && defined(__MACH__))
    return localtime_r(&t, out) != NULL;
#else
    struct tm *tmp = localtime(&t);
    if (!tmp)
        return 0;
    *out = *tmp;
    return 1;
#endif
}

static const char *
tl__level_string(TL_LogLevel level)
{
    switch (level) {
    case TL_LOG_LEVEL_DEBUG: return "DEBUG";
    case TL_LOG_LEVEL_INFO:  return "INFO";
    case TL_LOG_LEVEL_WARN:  return "WARN";
    case TL_LOG_LEVEL_ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

/* ---------------------------------------------------------- */
/* config                                                     */
/* ---------------------------------------------------------- */

static int
tl__open_file(const TL_LogConfig *cfg)
{
    int need_file = (cfg->output == TL_LOG_OUTPUT_FILE ||
                     cfg->error_output == TL_LOG_OUTPUT_FILE);

    /* If file output was disabled by reconfiguration, release old handle. */
    if (!need_file) {
        if (g__log_state.file) {
            fclose(g__log_state.file);
            g__log_state.file = NULL;
        }
        return 1;
    }

    if (!cfg->filename)
        return 0;

    if (g__log_state.file) {
        fclose(g__log_state.file);
        g__log_state.file = NULL;
    }

    g__log_state.file = fopen(cfg->filename, cfg->append ? "a" : "w");
    if (!g__log_state.file)
        return 0;

    return 1;
}

int
tl_log_init(const TL_LogConfig *cfg)
{
    TL_LogConfig next = cfg ? *cfg : TL_LOG_CONFIG_DEFAULT;

    next.level = tl__sanitize_level(next.level);

    if (!tl__open_file(&next))
        return 0;

    g__log_state.config = next;
    g__log_state.initialized = 1;

    return 1;
}

int
tl_log_reconfigure(const TL_LogConfig *cfg)
{
    if (!cfg)
        return 0;

    TL_LogConfig next = *cfg;

    next.level = tl__sanitize_level(next.level);

    if (!tl__open_file(&next))
        return 0;

    g__log_state.config = next;

    return 1;
}

void
tl_log_shutdown(void)
{
    if (g__log_state.file) {
        fclose(g__log_state.file);
        g__log_state.file = NULL;
    }

    g__log_state.initialized = 0;
}

int
tl_log_flush(void)
{
    int ok = 1;
    FILE *out = NULL;
    FILE *err = NULL;

    if (!g__log_state.initialized)
        tl_log_init(NULL);

    out = tl__resolve_stream(g__log_state.config.output);
    if (fflush(out) != 0)
        ok = 0;

    err = tl__resolve_stream(g__log_state.config.error_output);
    if (err != out && fflush(err) != 0)
        ok = 0;

    return ok;
}

/* ---------------------------------------------------------- */
/* getters/setters                                            */
/* ---------------------------------------------------------- */

void
tl_log_set_level(TL_LogLevel level)
{
    g__log_state.config.level = tl__sanitize_level(level);
}

TL_LogLevel
tl_log_get_level(void)
{
    return g__log_state.config.level;
}

const TL_LogConfig *
tl_log_get_config(void)
{
    return &g__log_state.config;
}

int
tl_log_is_enabled(TL_LogLevel level)
{
    return level >= g__log_state.config.level;
}

/* ---------------------------------------------------------- */
/* logging core                                               */
/* ---------------------------------------------------------- */

static void
tl__write_prefix(FILE *stream,
                 TL_LogLevel level,
                 const char *file,
                 int line,
                 const char *func)
{
    int has_prefix = 0;
    int has_location = 0;

    if (g__log_state.config.show_time) {

        time_t t = time(NULL);
        struct tm tmv;
        char buf[32];

        if (tl__to_local_time(t, &tmv)) {
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
            fprintf(stream, "[%s] ", buf);
            has_prefix = 1;
        }
    }

    if (g__log_state.config.show_level) {
        fprintf(stream, "[%s] ", tl__level_string(level));
        has_prefix = 1;
    }

    if (g__log_state.config.show_file && file) {
        fprintf(stream, "%s", file);
        has_location = 1;
    }

    if (g__log_state.config.show_line) {
        if (has_location)
            fprintf(stream, ":%d", line);
        else
            fprintf(stream, "line:%d", line);
        has_location = 1;
    }

    if (g__log_state.config.show_func && func) {
        if (has_location)
            fprintf(stream, " %s()", func);
        else
            fprintf(stream, "%s()", func);
        has_location = 1;
    }

    if (has_location)
        has_prefix = 1;

    if (has_prefix)
        fprintf(stream, ": ");
}

/* Internal va_list path used by tl_log_write(). */
static void
tl__log_vwrite(TL_LogLevel level,
               const char *file,
               int line,
               const char *func,
               const char *fmt,
               va_list args)
{
    if (!g__log_state.initialized)
        tl_log_init(NULL);

    if (!tl_log_is_enabled(level))
        return;

    FILE *stream = tl__select_stream(level);

    tl__write_prefix(stream, level, file, line, func);

    TL_LOG_DIAG_PUSH;
    TL_LOG_DIAG_IGNORE_FORMAT_NONLIT;
    vfprintf(stream, fmt, args);
    TL_LOG_DIAG_POP;

    fprintf(stream, "\n");

    if (g__log_state.config.auto_flush)
        fflush(stream);
}

void
tl_log_write(TL_LogLevel level,
             const char *file,
             int line,
             const char *func,
             const char *fmt,
             ...)
{
    va_list args;

    va_start(args, fmt);
    tl__log_vwrite(level, file, line, func, fmt, args);
    va_end(args);
}

void
tl_log_write_raw(TL_LogLevel level,
                 const char *file,
                 int line,
                 const char *func,
                 const char *msg)
{
    if (!g__log_state.initialized)
        tl_log_init(NULL);

    if (!tl_log_is_enabled(level))
        return;

    FILE *stream = tl__select_stream(level);

    tl__write_prefix(stream, level, file, line, func);

    fputs(msg ? msg : "(null)", stream);
    fputc('\n', stream);

    if (g__log_state.config.auto_flush)
        fflush(stream);
}

#endif /* TL_LOG_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif

#endif /* TINYLIB_LOG_H */
