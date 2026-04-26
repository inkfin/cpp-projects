#ifndef TINYLIB_STR_VIEW_H
#define TINYLIB_STR_VIEW_H

#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct TL_Str8 {
    char *data;
    size_t len;
} TL_Str8;

typedef struct TL_ZStr {
    char *data;
    size_t len;  // exclusive
} TL_ZStr;

typedef struct TL_StrView {
    char *data;
    size_t beg;
    size_t end;  // exclusive
} TL_StrView;

#define STR_FMT "%.*s"
#define STR_ARG(s) (int)((s).end - (s).beg), ((s).data + (s).beg)

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

static inline
TL_StrView
tl_sv_from_str8(TL_Str8 *str) {
    return (TL_StrView){
        .data = str->data,
        .beg = 0,
        .end = str->len,
    };
}

static inline
TL_StrView
tl_sv_from_zstr(TL_ZStr *str) {
    return (TL_StrView){
        .data = str->data,
        .beg = 0,
        .end = str->len,
    };
}

static inline
TL_StrView
tl_sv_trim_left(TL_StrView *sv) {
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

static inline
TL_StrView
tl_sv_trim_right(TL_StrView *sv) {
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

static inline
TL_StrView
tl_sv_trim(TL_StrView *sv) {
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

#ifndef TLSV_NO_ABBR
typedef TL_Str8    Str8;
typedef TL_ZStr    ZStr;
typedef TL_StrView StrV;
#endif // TLSV_NO_ABBR

#endif  // TINYLIB_STR_VIEW_H
