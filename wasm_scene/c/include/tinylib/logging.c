/* vim: set ft=c : -*- mode: c -*-
 * logging.c
 *   Implementation for tinylib/logging.h.
 *
 *   Use either:
 *     - compile this file as one translation unit, or
 *     - include this file in exactly one project .c file.
 */
#ifndef TINYLIB_LOGGING_IMPL_
#define TINYLIB_LOGGING_IMPL_

#include "logging.h"

#include <string.h>
#include <time.h>

#if !defined(TL_LOG_NO_THREADS) && defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define TL_LOG_HAS_LOCK 1
static SRWLOCK g__log_lock = SRWLOCK_INIT;
static
void
tl__log_lock(void)
{
    AcquireSRWLockExclusive(&g__log_lock);
}
static
void
tl__log_unlock(void)
{
    ReleaseSRWLockExclusive(&g__log_lock);
}
#elif !defined(TL_LOG_NO_THREADS) && \
    (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
#include <pthread.h>
#define TL_LOG_HAS_LOCK 1
static pthread_mutex_t g__log_mutex = PTHREAD_MUTEX_INITIALIZER;
static
void
tl__log_lock(void)
{
    pthread_mutex_lock(&g__log_mutex);
}
static
void
tl__log_unlock(void)
{
    pthread_mutex_unlock(&g__log_mutex);
}
#else
#define TL_LOG_HAS_LOCK 0
static
void
tl__log_lock(void)
{
}
static
void
tl__log_unlock(void)
{
}
#endif

typedef struct TL_LogState {
    TL_LogConfig config;
    FILE        *file;
    int          initialized;
} TL_LogState;

static TL_LogState g__log_state = {0};

static
void
tl__ensure_initialized_locked(void)
{
    if (!g__log_state.initialized) {
        g__log_state.config = (TL_LogConfig){0};
        g__log_state.initialized = 1;
    }
}

static
TL_LogLevel
tl__sanitize_level(TL_LogLevel level)
{
    if (level < TL_LOG_LEVEL_DEBUG) return TL_LOG_LEVEL_DEBUG;
    if (level > TL_LOG_LEVEL_ERROR) return TL_LOG_LEVEL_ERROR;
    return level;
}

static
FILE *
tl__resolve_stream(TL_LogOutput out, FILE *default_stream)
{
    switch (out) {
    case TL_LOG_OUTPUT_DEFAULT:
        return default_stream;

    case TL_LOG_OUTPUT_STDOUT:
        return stdout;

    case TL_LOG_OUTPUT_STDERR:
        return stderr;

    case TL_LOG_OUTPUT_FILE:
        if (g__log_state.file)
            return g__log_state.file;
        return stderr;
    }

    return default_stream;
}

static
FILE *
tl__select_stream(TL_LogLevel level)
{
    if (level >= TL_LOG_LEVEL_ERROR)
        return tl__resolve_stream(g__log_state.config.error_output, stderr);
    return tl__resolve_stream(g__log_state.config.output, stdout);
}

static
int
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

static
const char *
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

static
int
tl__open_file(const TL_LogConfig *cfg)
{
    int need_file = (cfg->output == TL_LOG_OUTPUT_FILE ||
                     cfg->error_output == TL_LOG_OUTPUT_FILE);

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
    TL_LogConfig next = cfg ? *cfg : (TL_LogConfig){0};
    int ok = 0;

    next.level = tl__sanitize_level(next.level);

    tl__log_lock();
    if (!tl__open_file(&next))
        goto done;

    g__log_state.config = next;
    g__log_state.initialized = 1;
    ok = 1;

done:
    tl__log_unlock();
    return ok;
}

int
tl_log_reconfigure(const TL_LogConfig *cfg)
{
    TL_LogConfig next;
    int ok = 0;

    if (!cfg)
        return 0;

    next = *cfg;

    next.level = tl__sanitize_level(next.level);

    tl__log_lock();
    if (!tl__open_file(&next))
        goto done;

    g__log_state.config = next;
    g__log_state.initialized = 1;
    ok = 1;

done:
    tl__log_unlock();
    return ok;
}

void
tl_log_shutdown(void)
{
    tl__log_lock();
    if (g__log_state.file) {
        fclose(g__log_state.file);
        g__log_state.file = NULL;
    }

    g__log_state.initialized = 0;
    tl__log_unlock();
}

int
tl_log_flush(void)
{
    int ok = 1;
    FILE *out = NULL;
    FILE *err = NULL;

    tl__log_lock();
    tl__ensure_initialized_locked();

    out = tl__resolve_stream(g__log_state.config.output, stdout);
    if (fflush(out) != 0)
        ok = 0;

    err = tl__resolve_stream(g__log_state.config.error_output, stderr);
    if (err != out && fflush(err) != 0)
        ok = 0;

    tl__log_unlock();
    return ok;
}

void
tl_log_set_level(TL_LogLevel level)
{
    tl__log_lock();
    tl__ensure_initialized_locked();
    g__log_state.config.level = tl__sanitize_level(level);
    tl__log_unlock();
}

TL_LogLevel
tl_log_get_level(void)
{
    TL_LogLevel level;

    tl__log_lock();
    tl__ensure_initialized_locked();
    level = g__log_state.config.level;
    tl__log_unlock();

    return level;
}

TL_LogConfig
tl_log_get_config(void)
{
    TL_LogConfig config;

    tl__log_lock();
    tl__ensure_initialized_locked();
    config = g__log_state.config;
    tl__log_unlock();

    return config;
}

int
tl_log_is_enabled(TL_LogLevel level)
{
    int enabled;

    tl__log_lock();
    tl__ensure_initialized_locked();
    enabled = level >= g__log_state.config.level;
    tl__log_unlock();

    return enabled;
}

static
void
tl__write_prefix(FILE *stream,
                 TL_LogLevel level,
                 const char *file,
                 int line,
                 const char *func)
{
    int has_prefix = 0;
    int has_location = 0;

    if (!g__log_state.config.disable_time) {

        time_t t = time(NULL);
        struct tm tmv;
        char buf[32];

        if (tl__to_local_time(t, &tmv)) {
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
            fprintf(stream, "[%s] ", buf);
            has_prefix = 1;
        }
    }

    if (!g__log_state.config.disable_level) {
        fprintf(stream, "[%s] ", tl__level_string(level));
        has_prefix = 1;
    }

    if (!g__log_state.config.disable_file && file) {
        fprintf(stream, "%s", file);
        has_location = 1;
    }

    if (!g__log_state.config.disable_line) {
        if (has_location)
            fprintf(stream, ":%d", line);
        else
            fprintf(stream, "line:%d", line);
        has_location = 1;
    }

    if (!g__log_state.config.disable_func && func) {
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

static
void
tl__log_vwrite(TL_LogLevel level,
               const char *file,
               int line,
               const char *func,
               const char *fmt,
               va_list args)
{
    FILE *stream;

    tl__log_lock();
    tl__ensure_initialized_locked();

    if (level < g__log_state.config.level) {
        tl__log_unlock();
        return;
    }

    stream = tl__select_stream(level);

    tl__write_prefix(stream, level, file, line, func);

    TL_LOG_DIAG_PUSH;
    TL_LOG_DIAG_IGNORE_FORMAT_NONLIT;
    vfprintf(stream, fmt, args);
    TL_LOG_DIAG_POP;

    fprintf(stream, "\n");

    if (!g__log_state.config.disable_auto_flush)
        fflush(stream);

    tl__log_unlock();
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
    FILE *stream;

    tl__log_lock();
    tl__ensure_initialized_locked();

    if (level < g__log_state.config.level) {
        tl__log_unlock();
        return;
    }

    stream = tl__select_stream(level);

    tl__write_prefix(stream, level, file, line, func);

    fputs(msg ? msg : "(null)", stream);
    fputc('\n', stream);

    if (!g__log_state.config.disable_auto_flush)
        fflush(stream);

    tl__log_unlock();
}

#endif /* TINYLIB_LOGGING_IMPL_ */
