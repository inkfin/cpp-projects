#ifndef TINYLIB_ERROR_H
#define TINYLIB_ERROR_H

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TLERROR_NO_CALLBACK

/**
 * Type definition for the error callback function.
 *
 * @param func Name or identifier of the function where the error occurred.
 * @param err  Error code of type errno_t (e.g., ENOMEM, EINVAL).
 * @param msg  Optional message providing additional context; may be NULL.
 * @param ctx  User-provided context pointer supplied at registration.
 */
typedef void (*error_callback_t)(
    const char* func, errno_t err, const char* msg, void* ctx);

/**
 * Register a global error callback function.
 *
 * @param cb  Pointer to the callback function; passing NULL unregisters the
 *            callback.
 * @param ctx User-defined context pointer that will be passed to the callback;
 *            may be NULL.
 */
extern void register_error_callback(error_callback_t cb, void* ctx);

/**
 * Unregister the current global error callback.
 */
extern void unregister_error_callback(void);

/**
 * Report an error to the registered callback.
 *
 * This function is intended for internal use by the library.
 *
 * @param func Name or identifier of the function reporting the error.
 * @param err  The error code of type errno_t.
 * @param msg  Optional descriptive message; may be NULL.
 */
extern void report_error(const char* func, errno_t err, const char* msg);

#ifndef ERROR_MODULE
#define ERROR_MODULE(name)                                                     \
    extern void name##_error_handler(                                          \
        const char* func, errno_t err, const char* msg, void* ctx);            \
    static inline void name##_Error_Init(void)                                 \
    {                                                                          \
        register_error_callback(name##_error_handler, NULL);                   \
    }
#endif // ERROR_MODULE

#else // TLERROR_NO_CALLBACK

/* Stub definitions when error callbacks are disabled */
typedef void (*error_callback_t)(const char*, errno_t, const char*, void*);

static inline void register_error_callback(error_callback_t cb, void* ctx)
{
    (void)cb;
    (void)ctx;
}

static inline void unregister_error_callback(void) { }

static inline void report_error(const char* func, errno_t err, const char* msg)
{
    (void)func;
    (void)err;
    (void)msg;
}

#ifndef ERROR_MODULE
#define ERROR_MODULE(name)
#endif // ERROR_MODULE

#endif // TLERROR_NO_CALLBACK

#ifdef __cplusplus
}
#endif

#endif // TINYLIB_ERROR_H
