/* vim: set ft=c : -*- mode: c -*-
 * tl_log.h
 *   Simple logging utilities for GNU99+ C projects.
 *
 *   Configuration:
 *
 *     TL_LOG_LEVEL:
 *       One of TL_LOG_LEVEL_DEBUG/INFO/WARN/ERROR.
 *       Lower-severity macros become no-ops at compile time.
 *
 *     TL_LOG_TO_FILE:
 *       Enables file-backed logging via tl_log_init()/tl_log_deinit().
 *       When enabled, TL_LOG_STREAM/TL_LOG_STREAM_ERR are overridden.
 *
 *     TL_LOG_STREAM, TL_LOG_STREAM_ERR:
 *       Output streams for normal and error logs.
 *
 *     TL_LOG_FAST:
 *       Disables timestamp generation.
 *
 *     TL_LOG_MSG_BUF_SIZE:
 *       Size of reusable per-thread message buffer used for formatted logs.
 *       If a message is too long, it is truncated and ends with "...".
 *
 *   Notes:
 *     Logger appends '\n' automatically; pass messages without trailing newline.
 */
#ifndef TINYLIB_LOGS_H
#define TINYLIB_LOGS_H


/** Configurations **/

#include <stdio.h>

#define TL_LOG_LEVEL_DEBUG 0
#define TL_LOG_LEVEL_INFO  1
#define TL_LOG_LEVEL_WARN  2
#define TL_LOG_LEVEL_ERROR 3
#ifndef TL_LOG_LEVEL
#  define TL_LOG_LEVEL  TL_LOG_LEVEL_INFO
#endif

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


#if defined(TL_LOG_TO_FILE)
#define TL_LOG_STREAM     tl__g_log_file
#define TL_LOG_STREAM_ERR tl__g_log_file
#include <stdlib.h>
static FILE *tl__g_log_file = NULL;
static inline void
tl_log_init(const char *filename) {
    tl__g_log_file = fopen(filename, "a");
    if (!tl__g_log_file) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
}
static inline void
tl_log_deinit() {
    if (tl__g_log_file) {
        fclose(tl__g_log_file);
        tl__g_log_file = NULL;
    }
}
#endif
#ifndef TL_LOG_STREAM
#  define TL_LOG_STREAM stdout
#endif
#ifndef TL_LOG_STREAM_ERR
#  define TL_LOG_STREAM_ERR stderr
#endif


/** Implementation **/

#define TL_LOG__FMT_STR "%s:%d %s: "

#include <time.h>

static inline char *
tl__log_msg_buf(void) {
    static TL_LOG__THREAD_LOCAL char buf[TL_LOG_MSG_BUF_SIZE];
    return buf;
}

static inline void
tl__log_write(FILE *stream,
              const char *file,
              int line,
              const char *func,
              const char *msg) {
#if !defined(TL_LOG_FAST)
    time_t     t = time(NULL);
    struct tm  tm_info;
    struct tm *tm_ptr = NULL;
#  if defined(_WIN32)
    if (localtime_s(&tm_info, &t) == 0) {
        tm_ptr = &tm_info;
    }
#  else /* POSIX */
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
    fprintf(stream, "[%s] " TL_LOG__FMT_STR "%s\n", time_buf, file, line, func, msg);
#else /* !defined(TL_LOG_FAST) */
    fprintf(stream, TL_LOG__FMT_STR "%s\n", file, line, func, msg);
#endif
}

#define TL__LOG_IMPL(stream, prefix, fmt, ...)                                               \
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
        tl__log_write(stream, __FILE__, __LINE__, __func__, tl__msg_buf);                    \
    } while (0)

#if TL_LOG_LEVEL <= TL_LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...) TL__LOG_IMPL(TL_LOG_STREAM, "[DEBUG] ", fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#if TL_LOG_LEVEL <= TL_LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...) TL__LOG_IMPL(TL_LOG_STREAM, "[INFO] ", fmt, ##__VA_ARGS__)
#else
#define LOG_INFO(...) ((void)0)
#endif

#if TL_LOG_LEVEL <= TL_LOG_LEVEL_WARN
#define LOG_WARN(fmt, ...) TL__LOG_IMPL(TL_LOG_STREAM, "[WARN] ", fmt, ##__VA_ARGS__)
#else
#define LOG_WARN(...) ((void)0)
#endif

#if TL_LOG_LEVEL <= TL_LOG_LEVEL_ERROR
#define LOG_ERROR(fmt, ...) TL__LOG_IMPL(TL_LOG_STREAM_ERR, "[ERROR] ", fmt, ##__VA_ARGS__)
#else
#define LOG_ERROR(...) ((void)0)
#endif

#endif /* TINYLIB_LOGS_H */
