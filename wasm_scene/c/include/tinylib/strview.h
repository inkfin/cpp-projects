#ifndef TINYLIB_STRVIEW_H
#define TINYLIB_STRVIEW_H

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#if defined(__has_attribute)
#define TL_STR__HAS_ATTRIBUTE(attr) __has_attribute(attr)
#else
#define TL_STR__HAS_ATTRIBUTE(attr) 0
#endif

#if TL_STR__HAS_ATTRIBUTE(unused)
#define TL_STR__MAYBE_UNUSED __attribute__((unused))
#else
#define TL_STR__MAYBE_UNUSED
#endif

typedef struct TL_Str8 {
    char *data;
    size_t len;
} TL_Str8;

typedef struct TL_ZStr {
    char *data;
    size_t len;  // exclusive
} TL_ZStr;

typedef struct TL_StrView {
    const char *data;
    size_t beg;
    size_t end;  // exclusive
} TL_StrView;

#define TL_STR_FMT "%.*s"
#define TL_STR_ARG(s) (int)((s).end - (s).beg), ((s).data + (s).beg)

TL_STR__MAYBE_UNUSED
static inline
TL_Str8
tl_str8_new(const char *str, size_t len) {
    char *data = malloc(len);
    if (data == NULL) { return (TL_Str8){0}; }
    memcpy(data, str, len);
    return (TL_Str8){
        .data = data,
        .len = len,
    };
}

TL_STR__MAYBE_UNUSED
static inline
TL_ZStr
tl_zstr_new(const char *str, size_t len) {
    char *data = malloc(len + 1);
    if (data == NULL) { return (TL_ZStr){0}; }
    memcpy(data, str, len);
    data[len] = '\0';
    return (TL_ZStr){
        .data = data,
        .len = len,
    };
}

TL_STR__MAYBE_UNUSED
static inline
TL_StrView
tl_sv_from_str8(const TL_Str8 *str) {
    return (TL_StrView){
        .data = str->data,
        .beg = 0,
        .end = str->len,
    };
}

TL_STR__MAYBE_UNUSED
static inline
TL_StrView
tl_sv_from_zstr(const TL_ZStr *str) {
    return (TL_StrView){
        .data = str->data,
        .beg = 0,
        .end = str->len,
    };
}

TL_STR__MAYBE_UNUSED
static inline
TL_StrView
tl_sv_trim_left(const TL_StrView *sv) {
    assert(sv != NULL);
    size_t beg = sv->beg;
    size_t end = sv->end;
    while (beg < end && (
           sv->data[beg] == ' '  ||
           sv->data[beg] == '\t' ||
           sv->data[beg] == '\n'
        )) { beg++; }
    return (TL_StrView) {
        .data = sv->data,
        .beg = beg,
        .end = end,
    };
}

TL_STR__MAYBE_UNUSED
static inline
TL_StrView
tl_sv_trim_right(const TL_StrView *sv) {
    assert(sv != NULL);
    size_t beg = sv->beg;
    size_t end = sv->end;
    while (end > beg && (
           sv->data[end - 1] == ' '  ||
           sv->data[end - 1] == '\t' ||
           sv->data[end - 1] == '\n'
        )) { end--; }
    return (TL_StrView) {
        .data = sv->data,
        .beg = beg,
        .end = end,
    };
}

TL_STR__MAYBE_UNUSED
static inline
TL_StrView
tl_sv_trim(const TL_StrView *sv) {
    assert(sv != NULL);
    size_t beg = sv->beg;
    size_t end = sv->end;
    while (beg < end && (
           sv->data[beg] == ' '  ||
           sv->data[beg] == '\t' ||
           sv->data[beg] == '\n'
        )) { beg++; }
    while (end > beg && (
           sv->data[end - 1] == ' '  ||
           sv->data[end - 1] == '\t' ||
           sv->data[end - 1] == '\n'
        )) { end--; }
    return (TL_StrView) {
        .data = sv->data,
        .beg = beg,
        .end = end,
    };
}

#undef TL_STR__MAYBE_UNUSED
#undef TL_STR__HAS_ATTRIBUTE

#if defined(TL_STR_SHORT_NAMES) || defined(TL_SHORT_NAMES)
typedef TL_Str8    Str8;
typedef TL_ZStr    ZStr;
typedef TL_StrView StrV;
#endif // TL_STR_SHORT_NAMES

#endif  // TINYLIB_STRVIEW_H
