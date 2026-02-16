/* vim: set ft=c : -*- mode: c -*-
 * tl_log.h
 *   Logging utilities for C projects
 *
 *   Configurations:
 *
 *     TL_LOG_LEVEL:
 *       DEBUG, INFO, WARN, ERROR
 *
 *     TL_LOG_TO_FILE:
 *       Filename that the log will be saved to
 *
 *     TL_LOG_STREAM, TL_LOG_STREAM_ERR:
 *       Specifies the stream to log to
 *       `TL_LOG_TO_FILE`, if defined, will overwrite this setting
 *
 *     TL_LOG_NO_STDARG:
 *       Use an simple macro implementation as alternatives
 *
 *     TL_LOG_FAST:
 *       Disable expensive runtime log features like timestamp
 *
 *     TL__LOG_IMPL
 *       Log implementation function, args: (FILE*, fmt, ...)
 */
#ifndef TINYLIB_LOGS_H
#define TINYLIB_LOGS_H


/** Configurations **/

#define TL_LOG_LEVEL_DEBUG 0
#define TL_LOG_LEVEL_INFO  1
#define TL_LOG_LEVEL_WARN  2
#define TL_LOG_LEVEL_ERROR 3
#ifndef TL_LOG_LEVEL
#  define TL_LOG_LEVEL  TL_LOG_LEVEL_INFO
#endif


#ifdef TL_LOG_TO_FILE
#  define TL_LOG_STREAM tl__g_log_file
#  define TL_LOG_STREAM_ERR tl__g_log_file
#  include <stdlib.h>
static FILE* tl__g_log_file = NULL;
static inline void
tl_log_init(const char* filename) {
    tl__g_log_file = fopen(filename, "a");
    if (!tl__g_log_file) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
}
#endif
#ifndef TL_LOG_STREAM
#  define TL_LOG_STREAM stdout
#endif
#ifndef TL_LOG_STREAM_ERR
# define TL_LOG_STREAM_ERR  TL_LOG_STREAM
#endif


/** Implementation **/

#define TL_LOG__FMT_STR "%s:%d %s: "
#if TL_LOG_FAST
#include <stdio.h>
#  if TL_LOG_NO_STDARG
#define TL__LOG_IMPL(stream, fmt, ...)                                                          \
    do {                                                                                        \
        fprintf(stream, TL_LOG__FMT_STR fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stream);                                                                         \
    } while (0)
#  else  /* !TL_LOG_NO_STDARG */
#include <stdarg.h>
static inline void
tl__log_impl(FILE *stream,
             const char *file,
             int line,
             const char *func,
             const char *fmt,
             ...) {
    fprintf(stream, TL_LOG__FMT_STR, file, line, func);

    va_list args;
    va_start(args, fmt);
    vfprintf(stream, fmt, args);
    va_end(args);

    fprintf(stream, "\n");
    fflush(stream);
}
#define TL__LOG_IMPL(stream, fmt, ...) tl__log_impl(stream, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#  endif
#else  /* !TL_LOG_FAST */
#include <time.h>
#include <stdio.h>
#  if TL_LOG_NO_STDARG
#define TL__LOG_IMPL(stream, fmt, ...)                                                                      \
    do {                                                                                                    \
        time_t     t = time(NULL);                                                                          \
        struct tm *tm_info = localtime(&t);                                                                 \
        char       time_buf[20];                                                                            \
        strftime(time_buf, sizeof time_buf, "%Y-%m-%d %H:%M:%S", tm_info);                                  \
        fprintf(stream, "[%s] " TL_LOG__FMT_STR fmt "\n", time_buf, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stream);                                                                                     \
    } while (0)
#  else  /* !TL_LOG_NO_STDARG */
#include <stdarg.h>
static inline void
tl__log_impl(FILE *stream,
             const char *file,
             int line,
             const char *func,
             const char *fmt,
             ...) {
    time_t t = time(NULL);

    struct tm tm_info;
#    if defined(_WIN32)
    localtime_s(&tm_info, &t);
#    else
    localtime_r(&t, &tm_info);
#    endif

    char time_buf[20];
    strftime(time_buf, sizeof time_buf, "%Y-%m-%d %H:%H:%S", &tm_info);

    fprintf(stream, "[%s] " TL_LOG__FMT_STR, time_buf, file, line, func);

    va_list args;
    va_start(args, fmt);
    vfprintf(stream, fmt, args);
    va_end(args);

    fprintf(stream, "\n");
    fflush(stream);
}
#define TL__LOG_IMPL(stream, fmt, ...) \
    tl__log_impl(stream, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#  endif
#endif


#define LOG_DEBUG(fmt, ...) TL__LOG_IMPL(TL_LOG_STREAM, "[DEBUG] " fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  TL__LOG_IMPL(TL_LOG_STREAM, "[INFO] " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  TL__LOG_IMPL(TL_LOG_STREAM, "[WARN] " fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) TL__LOG_IMPL(TL_LOG_STREAM_ERR, "[ERROR] " fmt, ##__VA_ARGS__)


#endif  /* TINYLIB_LOGS_H */
