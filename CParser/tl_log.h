/* vim: set ft=c : -*- mode: c -*-
 * tl_log.h
 *   Simple logging utilities for GNU99+ C projects.
 *
 *   Configuration:
 *
 *     TL_LOG_MSG_BUF_SIZE:
 *       Size of reusable per-thread message buffer used for formatted logs.
 *       If a message is too long, it is truncated and ends with "...".
 *
 *     TL_LOG_FAST:
 *       Disables timestamp generation.
 *
 *   Notes:
 *     Define TL_LOG_IMPLEMENTATION in exactly one .c file before including this
 *     header to provide shared global logger state for the whole program.
 *     Logger appends '\n' automatically; pass messages without trailing newline.
 */
#ifndef TINYLIB_LOGS_H
#define TINYLIB_LOGS_H

#include <stdio.h>

enum TL_LogLevel {
    TL_LOG_LEVEL_DEBUG,
    TL_LOG_LEVEL_INFO,
    TL_LOG_LEVEL_WARN,
    TL_LOG_LEVEL_ERROR,
};

enum TL_LogStream {
    TL_LOG_STREAM_STDOUT,
    TL_LOG_STREAM_STDERR,
    TL_LOG_STREAM_FILE,
};

#ifndef TL_LOG_MSG_BUF_SIZE
#  define TL_LOG_MSG_BUF_SIZE 1024
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#  define TL_LOG__THREAD_LOCAL _Thread_local
#elif defined(__GNUC__) || defined(__clang__)
#  define TL_LOG__THREAD_LOCAL __thread
#elif defined(_MSC_VER)
#  define TL_LOG__THREAD_LOCAL __declspec(thread)
#else
#  define TL_LOG__THREAD_LOCAL
#endif

typedef struct TLLogConfig {
    enum TL_LogLevel  level;
    const char       *filename;
    enum TL_LogStream stream;
    enum TL_LogStream stream_err;
    int               append;
    int               auto_flush;
} TL_LogConfig;

extern TL_LogConfig  tl_log_global;
extern FILE        *tl__g_log_file;
extern const char  *tl__g_log_filename;

int  tl_log_init(const TL_LogConfig *cfg);
void tl_log_deinit(void);
void tl_log_set_level(enum TL_LogLevel level);

void tl__log_write(FILE *stream,
                   const char *file,
                   int line,
                   const char *func,
                   const char *msg);

FILE *tl__log_stream(int is_err);
char *tl__log_msg_buf(void);

#define TL__LOG_IMPL(is_err, prefix, fmt, ...)                                               \
    do {                                                                                     \
        char *tl__msg_buf = tl__log_msg_buf();                                               \
        int   tl__n = snprintf(tl__msg_buf, TL_LOG_MSG_BUF_SIZE, prefix fmt, ##__VA_ARGS__); \
        if (tl__n < 0 && TL_LOG_MSG_BUF_SIZE > 0) {                                          \
            tl__msg_buf[0] = '\0';                                                           \
        } else if ((size_t)tl__n >= TL_LOG_MSG_BUF_SIZE && TL_LOG_MSG_BUF_SIZE >= 4) {       \
            tl__msg_buf[TL_LOG_MSG_BUF_SIZE - 4] = '.';                                      \
            tl__msg_buf[TL_LOG_MSG_BUF_SIZE - 3] = '.';                                      \
            tl__msg_buf[TL_LOG_MSG_BUF_SIZE - 2] = '.';                                      \
            tl__msg_buf[TL_LOG_MSG_BUF_SIZE - 1] = '\0';                                     \
        }                                                                                    \
        tl__log_write(tl__log_stream(is_err), __FILE__, __LINE__, __func__, tl__msg_buf);    \
    } while (0)

#define LOG_DEBUG(fmt, ...)                                  \
    do {                                                     \
        if (tl_log_global.level <= TL_LOG_LEVEL_DEBUG) {     \
            TL__LOG_IMPL(0, "[DEBUG] ", fmt, ##__VA_ARGS__); \
        }                                                    \
    } while (0)

#define LOG_INFO(fmt, ...)                                  \
    do {                                                    \
        if (tl_log_global.level <= TL_LOG_LEVEL_INFO) {     \
            TL__LOG_IMPL(0, "[INFO] ", fmt, ##__VA_ARGS__); \
        }                                                   \
    } while (0)

#define LOG_WARN(fmt, ...)                                  \
    do {                                                    \
        if (tl_log_global.level <= TL_LOG_LEVEL_WARN) {     \
            TL__LOG_IMPL(0, "[WARN] ", fmt, ##__VA_ARGS__); \
        }                                                   \
    } while (0)

#define LOG_ERROR(fmt, ...)                                  \
    do {                                                     \
        if (tl_log_global.level <= TL_LOG_LEVEL_ERROR) {     \
            TL__LOG_IMPL(1, "[ERROR] ", fmt, ##__VA_ARGS__); \
        }                                                    \
    } while (0)

#if defined(TL_LOG_IMPLEMENTATION)
#include <string.h>
#include <time.h>

TL_LogConfig tl_log_global = {
    .level = TL_LOG_LEVEL_INFO,
    .filename = NULL,
    .stream = TL_LOG_STREAM_STDOUT,
    .stream_err = TL_LOG_STREAM_STDERR,
    .append = 1,
    .auto_flush = 1,
};

FILE       *tl__g_log_file     = NULL;
const char *tl__g_log_filename = NULL;

static enum TL_LogLevel
tl__sanitize_level(enum TL_LogLevel level) {
    if (level < TL_LOG_LEVEL_DEBUG) return TL_LOG_LEVEL_DEBUG;
    if (level > TL_LOG_LEVEL_ERROR) return TL_LOG_LEVEL_ERROR;
    return level;
}

static enum TL_LogStream
tl__sanitize_stream(enum TL_LogStream stream, enum TL_LogStream fallback) {
    if (stream == TL_LOG_STREAM_STDOUT || stream == TL_LOG_STREAM_STDERR ||
        stream == TL_LOG_STREAM_FILE) {
        return stream;
    }
    return fallback;
}

int
tl_log_init(const TL_LogConfig *cfg) {
    TL_LogConfig next = tl_log_global;

    if (cfg != NULL) {
        next.level      = tl__sanitize_level(cfg->level);
        next.filename   = cfg->filename;
        next.stream     = tl__sanitize_stream(cfg->stream, TL_LOG_STREAM_STDOUT);
        next.stream_err = tl__sanitize_stream(cfg->stream_err, TL_LOG_STREAM_STDERR);
        next.append     = (cfg->append != 0) ? 1 : 0;
        next.auto_flush = (cfg->auto_flush != 0) ? 1 : 0;
    }

    if ((next.stream == TL_LOG_STREAM_FILE || next.stream_err == TL_LOG_STREAM_FILE) &&
        next.filename == NULL) {
        fprintf(stderr, "tl_log_init: [ERROR] Log stream set to FILE but filename is NULL\n");
        return 0;
    }

    if (next.stream == TL_LOG_STREAM_FILE || next.stream_err == TL_LOG_STREAM_FILE) {
        if (tl__g_log_file != NULL && tl__g_log_filename != NULL && next.filename != NULL &&
            strcmp(tl__g_log_filename, next.filename) == 0) {
            tl_log_global = next;
            return 1;
        }

        if (tl__g_log_file != NULL) {
            fclose(tl__g_log_file);
            tl__g_log_file = NULL;
            tl__g_log_filename = NULL;
        }

        tl__g_log_file = fopen(next.filename, next.append ? "a" : "w");
        if (tl__g_log_file == NULL) {
            fprintf(stderr, "tl_log_init: [ERROR] Failed to open log file '%s' for writing\n", next.filename);
            return 0;
        }
        tl__g_log_filename = next.filename;
    } else if (tl__g_log_file != NULL) {
        fclose(tl__g_log_file);
        tl__g_log_file = NULL;
        tl__g_log_filename = NULL;
    }

    tl_log_global = next;
    return 1;
}

void
tl_log_deinit(void) {
    if (tl__g_log_file != NULL) {
        fclose(tl__g_log_file);
        tl__g_log_file = NULL;
        tl__g_log_filename = NULL;
    }
}

void
tl_log_set_level(enum TL_LogLevel level) {
    tl_log_global.level = tl__sanitize_level(level);
}

char *
tl__log_msg_buf(void) {
    static TL_LOG__THREAD_LOCAL char buf[TL_LOG_MSG_BUF_SIZE];
    return buf;
}

FILE *
tl__log_stream(int is_err) {
    enum TL_LogStream stream = is_err ? tl_log_global.stream_err : tl_log_global.stream;

    if (stream == TL_LOG_STREAM_STDERR) {
        return stderr;
    }
    if (stream == TL_LOG_STREAM_FILE) {
        if (tl__g_log_file != NULL) {
            return tl__g_log_file;
        }
        return stderr;
    }
    return stdout;
}

void
tl__log_write(FILE *stream,
              const char *file,
              int line,
              const char *func,
              const char *msg) {
#if !defined(TL_LOG_FAST)
    time_t     t      = time(NULL);
    struct tm  tm_info;
    struct tm *tm_ptr = NULL;
#  if defined(_WIN32)
    if (localtime_s(&tm_info, &t) == 0) {
        tm_ptr = &tm_info;
    }
#  else
    if (localtime_r(&t, &tm_info) != NULL) {
        tm_ptr = &tm_info;
    }
#  endif

    char time_buf[20];
    if (tm_ptr) {
        strftime(time_buf, sizeof time_buf, "%Y-%m-%d %H:%M:%S", tm_ptr);
    } else {
        snprintf(time_buf, sizeof time_buf, "0000-00-00 00:00:00");
    }

    fprintf(stream, "[%s] %s:%d %s: %s\n", time_buf, file, line, func, msg);
#else
    fprintf(stream, "%s:%d %s: %s\n", file, line, func, msg);
#endif

    if (tl_log_global.auto_flush) {
        fflush(stream);
    }
}
#endif

#endif /* TINYLIB_LOGS_H */
