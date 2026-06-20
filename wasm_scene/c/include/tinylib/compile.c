/* vim: set ft=c : -*- mode: c -*-
 * compile.c
 *   Implementation unit for compile.h.
 */
#ifndef TINYLIB_COMPILE_IMPL_
#define TINYLIB_COMPILE_IMPL_

#include "compile.h"
#include "logging.c"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#if defined(_WIN32)
#else
#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

static
TL_Allocator *
tl_compile__allocator(const TL_CompileCmd *cmd)
{
    return (cmd && cmd->allocator) ? cmd->allocator : (TL_Allocator *)&tl_default_allocator;
}

static
char *
tl_compile__strdup(TL_Allocator *allocator, const char *str)
{
    size_t len;
    char *copy;

    if (!str) return NULL;
    allocator = allocator ? allocator : (TL_Allocator *)&tl_default_allocator;
    len = strlen(str);
    copy = (char *)tl_allocator_alloc(allocator, len + 1U);
    if (!copy) return NULL;
    memcpy(copy, str, len + 1U);
    return copy;
}

static
void
tl_compile__strfree(TL_Allocator *allocator, char *str)
{
    if (!str) return;
    allocator = allocator ? allocator : (TL_Allocator *)&tl_default_allocator;
    tl_allocator_free(allocator, str, strlen(str) + 1U);
}

static
bool
tl_compile__replace_str(TL_Allocator *allocator, char **dst, const char *src)
{
    char *copy = tl_compile__strdup(allocator, src);
    if (!copy) return false;
    tl_compile__strfree(allocator, *dst);
    *dst = copy;
    return true;
}

static
void
tl_compile__free_str_array(TL_Allocator *allocator, char **arr)
{
    size_t i;

    for (i = 0; i < tl_arr_len(arr); ++i) {
        tl_compile__strfree(allocator, arr[i]);
    }
    tl_arr_free(arr);
}

static
bool
tl_compile__push_str(TL_Allocator *allocator, char ***arr, const char *str)
{
    char *copy = tl_compile__strdup(allocator, str);
    if (!copy) return false;
    if (!tl_arr_push(*arr, copy)) {
        tl_compile__strfree(allocator, copy);
        return false;
    }
    return true;
}

static
const char *
tl_compile__kind_name(TL_CompilerKind kind)
{
    switch (kind) {
    case TL_COMPILER_CLANG: return "clang";
    case TL_COMPILER_GCC: return "gcc";
    case TL_COMPILER_CC:
    default: return "cc";
    }
}

static
const char *
tl_compile__standard_flag(TL_CStandard standard)
{
    switch (standard) {
    case TL_C_STD_C99: return "-std=c99";
    case TL_C_STD_C11: return "-std=c11";
    case TL_C_STD_GNU11: return "-std=gnu11";
    case TL_C_STD_C17: return "-std=c17";
    case TL_C_STD_GNU17: return "-std=gnu17";
    case TL_C_STD_C23: return "-std=c23";
    case TL_C_STD_DEFAULT:
    default: return NULL;
    }
}

static const char *tl_compile__debug_flags[] = { "-Og", "-g", "-Wall", "-Wextra" };
static const char *tl_compile__debug_defines[] = { "DEBUG" };
static const char *tl_compile__debug_sanitize_flags[] = {
    "-O0",
    "-g3",
    "-ggdb",
    "-fsanitize=address,undefined",
    "-fno-omit-frame-pointer",
    "-fstack-protector-strong",
    "-fno-common",
    "-Wall",
    "-Wextra",
};
static const char *tl_compile__debug_sanitize_defines[] = { "DEBUG" };
static const char *tl_compile__debug_sanitize_link_flags[] = {
    "-fsanitize=address,undefined",
};
static const char *tl_compile__release_flags[] = { "-O2", "-Wall", "-Wextra" };
static const char *tl_compile__release_defines[] = { "NDEBUG" };
static const char *tl_compile__relinfo_flags[] = { "-O2", "-g", "-Wall", "-Wextra" };
static const char *tl_compile__relinfo_defines[] = { "NDEBUG" };

const TL_CompilePreset tl_compile_preset_debug = {
    .name = "debug",
    .standard = TL_C_STD_DEFAULT,
    .defines = tl_compile__debug_defines,
    .defines_count = TL_COUNT_OF(tl_compile__debug_defines),
    .flags = tl_compile__debug_flags,
    .flags_count = TL_COUNT_OF(tl_compile__debug_flags),
};

const TL_CompilePreset tl_compile_preset_debug_sanitize = {
    .name = "debug-sanitize",
    .standard = TL_C_STD_DEFAULT,
    .defines = tl_compile__debug_sanitize_defines,
    .defines_count = TL_COUNT_OF(tl_compile__debug_sanitize_defines),
    .flags = tl_compile__debug_sanitize_flags,
    .flags_count = TL_COUNT_OF(tl_compile__debug_sanitize_flags),
    .link_flags = tl_compile__debug_sanitize_link_flags,
    .link_flags_count = TL_COUNT_OF(tl_compile__debug_sanitize_link_flags),
};

const TL_CompilePreset tl_compile_preset_release = {
    .name = "release",
    .standard = TL_C_STD_DEFAULT,
    .defines = tl_compile__release_defines,
    .defines_count = TL_COUNT_OF(tl_compile__release_defines),
    .flags = tl_compile__release_flags,
    .flags_count = TL_COUNT_OF(tl_compile__release_flags),
};

const TL_CompilePreset tl_compile_preset_relwithdebinfo = {
    .name = "relwithdebinfo",
    .standard = TL_C_STD_DEFAULT,
    .defines = tl_compile__relinfo_defines,
    .defines_count = TL_COUNT_OF(tl_compile__relinfo_defines),
    .flags = tl_compile__relinfo_flags,
    .flags_count = TL_COUNT_OF(tl_compile__relinfo_flags),
};

/* Error reporting: uses logging.h macros so output goes through the same
 * pipeline as the build log (locking, stream selection, flush).
 * Before tl_build_log_init() the logger auto-initialises with defaults,
 * which include file, line, and func in the prefix. */
#define TL_COMPILE_ERR(msg, hint) \
    TL_LOG_ERROR((hint) ? "%s\n  hint: %s" : "%s", msg, (hint) ? hint : "")

#define TL_COMPILE_ERR_ERRNO(msg, ctx, saved_errno) \
    TL_LOG_ERROR("%s: %s: %s", msg, ctx, strerror(saved_errno))

bool
tl_compile_cmd_init(TL_CompileCmd *cmd, TL_Allocator *allocator)
{
    if (!cmd) {
        TL_COMPILE_ERR("cmd is NULL", "allocate on the stack: TL_CompileCmd cmd = {0}; tl_compile_cmd_init(&cmd, NULL)");
        return false;
    }
    memset(cmd, 0, sizeof(*cmd));
    if (allocator) {
        cmd->allocator = allocator;
    } else {
        cmd->internal_allocator = tl_get_allocator_arena(&cmd->internal_arena);
        cmd->allocator = &cmd->internal_allocator;
    }
    cmd->compiler_kind = TL_COMPILER_CC;
    cmd->standard = TL_C_STD_DEFAULT;
    tl_arr_init(cmd->sources, cmd->allocator);
    tl_arr_init(cmd->include_dirs, cmd->allocator);
    tl_arr_init(cmd->defines, cmd->allocator);
    tl_arr_init(cmd->flags, cmd->allocator);
    tl_arr_init(cmd->link_flags, cmd->allocator);
    tl_arr_init(cmd->libs, cmd->allocator);
    return tl_compile_set_compiler(cmd, tl_compile__kind_name(cmd->compiler_kind));
}

void
tl_compile_cmd_free(TL_CompileCmd *cmd)
{
    TL_Allocator *allocator;

    if (!cmd) return;

    if (cmd->allocator == &cmd->internal_allocator) {
        tl_arena_destroy(&cmd->internal_arena);
    } else {
        allocator = tl_compile__allocator(cmd);
        tl_compile__strfree(allocator, cmd->compiler);
        tl_compile__strfree(allocator, cmd->output);
        tl_compile__free_str_array(allocator, cmd->sources);
        tl_compile__free_str_array(allocator, cmd->include_dirs);
        tl_compile__free_str_array(allocator, cmd->defines);
        tl_compile__free_str_array(allocator, cmd->flags);
        tl_compile__free_str_array(allocator, cmd->link_flags);
        tl_compile__free_str_array(allocator, cmd->libs);
    }
    memset(cmd, 0, sizeof(*cmd));
}

bool
tl_compile_set_compiler(TL_CompileCmd *cmd, const char *compiler)
{
    if (!cmd || !compiler) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        if (!compiler) TL_COMPILE_ERR("compiler is NULL", "pass a valid compiler string, e.g. \"cc\", \"clang\", or \"gcc\"");
        return false;
    }
    return tl_compile__replace_str(tl_compile__allocator(cmd), &cmd->compiler, compiler);
}

bool
tl_compile_set_compiler_kind(TL_CompileCmd *cmd, TL_CompilerKind kind)
{
    if (!cmd) {
        TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        return false;
    }
    cmd->compiler_kind = kind;
    return tl_compile_set_compiler(cmd, tl_compile__kind_name(kind));
}

bool
tl_compile_set_standard(TL_CompileCmd *cmd, TL_CStandard standard)
{
    if (!cmd) {
        TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        return false;
    }
    cmd->standard = standard;
    return true;
}

bool
tl_compile_set_output(TL_CompileCmd *cmd, const char *path)
{
    if (!cmd || !path) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        if (!path) TL_COMPILE_ERR("path is NULL", "pass a valid output path, e.g. \"target/myapp\"");
        return false;
    }
    return tl_compile__replace_str(tl_compile__allocator(cmd), &cmd->output, path);
}

bool
tl_compile_apply_preset(TL_CompileCmd *cmd, const TL_CompilePreset *preset)
{
    if (!cmd || !preset) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        if (!preset) TL_COMPILE_ERR("preset is NULL", "pass a valid preset pointer, e.g. &tl_compile_preset_debug");
        return false;
    }

    if (preset->standard != TL_C_STD_DEFAULT &&
        !tl_compile_set_standard(cmd, preset->standard)) {
        return false;
    }
    {
        size_t i;
        for (i = 0; i < preset->defines_count; ++i) {
            if (!tl_compile_add_define(cmd, preset->defines[i])) return false;
        }
        for (i = 0; i < preset->flags_count; ++i) {
            if (!tl_compile_add_flag(cmd, preset->flags[i])) return false;
        }
        for (i = 0; i < preset->link_flags_count; ++i) {
            if (!tl_compile_add_link_flag(cmd, preset->link_flags[i])) return false;
        }
        for (i = 0; i < preset->libs_count; ++i) {
            if (!tl_compile_add_lib(cmd, preset->libs[i])) return false;
        }
    }
    return true;
}

bool
tl_compile_add_source(TL_CompileCmd *cmd, const char *path)
{
    if (!cmd || !path) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        if (!path) TL_COMPILE_ERR("path is NULL", "pass a valid source file path, e.g. \"src/main.c\"");
        return false;
    }
    return tl_compile__push_str(tl_compile__allocator(cmd), &cmd->sources, path);
}

bool
tl_compile_add_include(TL_CompileCmd *cmd, const char *path)
{
    if (!cmd || !path) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        if (!path) TL_COMPILE_ERR("path is NULL", "pass a valid include directory, e.g. \"include\"");
        return false;
    }
    return tl_compile__push_str(tl_compile__allocator(cmd), &cmd->include_dirs, path);
}

bool
tl_compile_add_define(TL_CompileCmd *cmd, const char *define)
{
    if (!cmd || !define) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        if (!define) TL_COMPILE_ERR("define is NULL", "pass the define name, e.g. \"DEBUG\" or \"VERSION=1\"");
        return false;
    }
    return tl_compile__push_str(tl_compile__allocator(cmd), &cmd->defines, define);
}

bool
tl_compile_add_flag(TL_CompileCmd *cmd, const char *flag)
{
    if (!cmd || !flag) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        if (!flag) TL_COMPILE_ERR("flag is NULL", "pass a compiler flag, e.g. \"-Wall\"");
        return false;
    }
    return tl_compile__push_str(tl_compile__allocator(cmd), &cmd->flags, flag);
}

bool
tl_compile_add_link_flag(TL_CompileCmd *cmd, const char *flag)
{
    if (!cmd || !flag) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        if (!flag) TL_COMPILE_ERR("flag is NULL", "pass a linker flag, e.g. \"-pthread\"");
        return false;
    }
    return tl_compile__push_str(tl_compile__allocator(cmd), &cmd->link_flags, flag);
}

bool
tl_compile_add_lib(TL_CompileCmd *cmd, const char *lib)
{
    if (!cmd || !lib) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        if (!lib) TL_COMPILE_ERR("lib is NULL", "pass the library name, e.g. \"m\" or \"pthread\"");
        return false;
    }
    return tl_compile__push_str(tl_compile__allocator(cmd), &cmd->libs, lib);
}

bool
tl_compile_add_sources(TL_CompileCmd *cmd, const char **paths, size_t paths_count)
{
    size_t i;
    if (!cmd || (!paths && paths_count > 0)) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        else TL_COMPILE_ERR("paths is NULL but paths_count > 0", "set count to 0 or provide a valid array of source paths");
        return false;
    }
    for (i = 0; i < paths_count; ++i) {
        if (!tl_compile_add_source(cmd, paths[i])) return false;
    }
    return true;
}

bool
tl_compile_add_includes(TL_CompileCmd *cmd, const char **paths, size_t paths_count)
{
    size_t i;
    if (!cmd || (!paths && paths_count > 0)) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        else TL_COMPILE_ERR("paths is NULL but paths_count > 0", "set count to 0 or provide a valid array of include directories");
        return false;
    }
    for (i = 0; i < paths_count; ++i) {
        if (!tl_compile_add_include(cmd, paths[i])) return false;
    }
    return true;
}

bool
tl_compile_add_flags(TL_CompileCmd *cmd, const char **flags, size_t flags_count)
{
    size_t i;
    if (!cmd || (!flags && flags_count > 0)) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        else TL_COMPILE_ERR("flags is NULL but flags_count > 0", "set count to 0 or provide a valid array of compile flags");
        return false;
    }
    for (i = 0; i < flags_count; ++i) {
        if (!tl_compile_add_flag(cmd, flags[i])) return false;
    }
    return true;
}

bool
tl_compile_add_defines(TL_CompileCmd *cmd, const char **defines, size_t defines_count)
{
    size_t i;
    if (!cmd || (!defines && defines_count > 0)) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        else TL_COMPILE_ERR("defines is NULL but defines_count > 0", "set count to 0 or provide a valid array of define strings");
        return false;
    }
    for (i = 0; i < defines_count; ++i) {
        if (!tl_compile_add_define(cmd, defines[i])) return false;
    }
    return true;
}

bool
tl_compile_add_link_flags(TL_CompileCmd *cmd, const char **flags, size_t flags_count)
{
    size_t i;
    if (!cmd || (!flags && flags_count > 0)) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        else TL_COMPILE_ERR("flags is NULL but flags_count > 0", "set count to 0 or provide a valid array of linker flags");
        return false;
    }
    for (i = 0; i < flags_count; ++i) {
        if (!tl_compile_add_link_flag(cmd, flags[i])) return false;
    }
    return true;
}

bool
tl_compile_add_libs(TL_CompileCmd *cmd, const char **libs, size_t libs_count)
{
    size_t i;
    if (!cmd || (!libs && libs_count > 0)) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        else TL_COMPILE_ERR("libs is NULL but libs_count > 0", "set count to 0 or provide a valid array of library names");
        return false;
    }
    for (i = 0; i < libs_count; ++i) {
        if (!tl_compile_add_lib(cmd, libs[i])) return false;
    }
    return true;
}

static
bool
tl_compile__has_suffix(const char *str, const char *suffix)
{
    size_t str_len;
    size_t suffix_len;

    if (!str || !suffix) return false;
    str_len = strlen(str);
    suffix_len = strlen(suffix);
    return str_len >= suffix_len && strcmp(str + str_len - suffix_len, suffix) == 0;
}

static
bool
tl_source_matches_extension(const char *path, const TL_SourceFindConfig *cfg)
{
    size_t i;
    static const char *default_exts[] = { ".c" };
    const char **exts = cfg->extensions ? cfg->extensions : default_exts;
    size_t count = cfg->extensions ? cfg->extensions_count : 1U;

    for (i = 0; i < count; ++i) {
        if (tl_compile__has_suffix(path, exts[i])) return true;
    }
    return false;
}

static
bool
tl_source_ignore_dir(const char *name, const TL_SourceFindConfig *cfg)
{
    size_t i;
    static const char *default_ignores[] = { ".git", "target", "build" };
    const char **ignores = cfg->ignore_dirs ? cfg->ignore_dirs : default_ignores;
    size_t count = cfg->ignore_dirs ? cfg->ignore_dirs_count : 3U;

    if (!cfg->include_hidden && name[0] == '.') return true;
    if (strncmp(name, "cmake-build-", 12U) == 0) return true;
    for (i = 0; i < count; ++i) {
        if (strcmp(name, ignores[i]) == 0) return true;
    }
    return false;
}

static
char *
tl_path_join(TL_Allocator *allocator, const char *a, const char *b)
{
    size_t a_len = strlen(a);
    size_t b_len = strlen(b);
    bool need_slash = a_len > 0 && a[a_len - 1U] != '/';
    size_t total = a_len + (need_slash ? 1U : 0U) + b_len + 1U;
    char *path = (char *)tl_allocator_alloc(allocator, total);

    if (!path) return NULL;
    memcpy(path, a, a_len);
    if (need_slash) path[a_len++] = '/';
    memcpy(path + a_len, b, b_len + 1U);
    return path;
}

static
int
tl_source_qsort_cmp(const void *a, const void *b)
{
    const char *const *sa = (const char *const *)a;
    const char *const *sb = (const char *const *)b;
    return strcmp(*sa, *sb);
}

#if defined(_WIN32)
static
bool
tl_source_find_dir(TL_Allocator *allocator, const char *dir, const TL_SourceFindConfig *cfg, char ***out)
{
    (void)allocator;
    (void)dir;
    (void)cfg;
    (void)out;
    return false;
}
#else
static
bool
tl_source_find_dir(TL_Allocator *allocator, const char *dir, const TL_SourceFindConfig *cfg, char ***out)
{
    DIR *handle;
    struct dirent *entry;

    handle = opendir(dir);
    if (!handle) return false;

    while ((entry = readdir(handle)) != NULL) {
        char *path;
        struct stat st;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        path = tl_path_join(allocator, dir, entry->d_name);
        if (!path) {
            closedir(handle);
            return false;
        }

        if (lstat(path, &st) != 0) {
            tl_compile__strfree(allocator, path);
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (cfg->recursive && !tl_source_ignore_dir(entry->d_name, cfg)) {
                if (!tl_source_find_dir(allocator, path, cfg, out)) {
                    tl_compile__strfree(allocator, path);
                    closedir(handle);
                    return false;
                }
            }
            tl_compile__strfree(allocator, path);
            continue;
        }

        if (S_ISREG(st.st_mode) && tl_source_matches_extension(path, cfg)) {
            if (!tl_arr_push(*out, path)) {
                tl_compile__strfree(allocator, path);
                closedir(handle);
                return false;
            }
        } else {
            tl_compile__strfree(allocator, path);
        }
    }

    closedir(handle);
    return true;
}
#endif

bool
tl_source_find(const TL_SourceFindConfig *cfg, char ***out_sources)
{
    TL_SourceFindConfig resolved;
    TL_Allocator *allocator = (TL_Allocator *)&tl_default_allocator;
    char **sources = NULL;

    if (!cfg || !cfg->root || !out_sources) {
        if (!cfg) TL_COMPILE_ERR("cfg is NULL", "initialize a TL_SourceFindConfig, e.g. TL_SourceFindConfig cfg = { .root = \"src\" }");
        else if (!cfg->root) TL_COMPILE_ERR("cfg->root is NULL", "set cfg.root to the directory to scan, e.g. \"src\"");
        else TL_COMPILE_ERR("out_sources is NULL", "pass a char*** pointer to receive the results");
        return false;
    }
    resolved = *cfg;

    if (!tl_source_find_dir(allocator, resolved.root, &resolved, &sources)) {
        tl_source_find_free(sources);
        return false;
    }

    qsort(sources, tl_arr_len(sources), sizeof(sources[0]), tl_source_qsort_cmp);
    *out_sources = sources;
    return true;
}

void
tl_source_find_free(char **sources)
{
    tl_compile__free_str_array((TL_Allocator *)&tl_default_allocator, sources);
}

bool
tl_compile_add_sources_recursive(TL_CompileCmd *cmd, const TL_SourceFindConfig *cfg)
{
    char **sources = NULL;
    TL_SourceFindConfig recursive_cfg;
    size_t i;

    if (!cmd || !cfg) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        if (!cfg) TL_COMPILE_ERR("cfg is NULL", "initialize a TL_SourceFindConfig with .root set");
        return false;
    }
    recursive_cfg = *cfg;
    recursive_cfg.recursive = true;
    if (!tl_source_find(&recursive_cfg, &sources)) return false;
    for (i = 0; i < tl_arr_len(sources); ++i) {
        if (!tl_compile_add_source(cmd, sources[i])) {
            tl_source_find_free(sources);
            return false;
        }
    }
    tl_source_find_free(sources);
    return true;
}

static
bool
tl_compile_argv_push_dup(const TL_CompileCmd *cmd, char ***argv, const char *arg)
{
    TL_Allocator *allocator = tl_compile__allocator(cmd);
    char *copy = tl_compile__strdup(allocator, arg);

    if (!copy) return false;
    if (!tl_arr_push(*argv, copy)) {
        tl_compile__strfree(allocator, copy);
        return false;
    }
    return true;
}

static
char *
tl_compile_prefix_arg(const TL_CompileCmd *cmd, const char *prefix, const char *value)
{
    TL_Allocator *allocator = tl_compile__allocator(cmd);
    size_t prefix_len = strlen(prefix);
    size_t value_len = strlen(value);
    char *arg = (char *)tl_allocator_alloc(allocator, prefix_len + value_len + 1U);

    if (!arg) return NULL;
    memcpy(arg, prefix, prefix_len);
    memcpy(arg + prefix_len, value, value_len + 1U);
    return arg;
}

static
bool
tl_compile_argv_push_owned(const TL_CompileCmd *cmd, char ***argv, char *arg)
{
    if (!arg) return false;
    if (!tl_arr_push(*argv, arg)) {
        tl_compile__strfree(tl_compile__allocator(cmd), arg);
        return false;
    }
    return true;
}

bool
tl_compile_render_argv(const TL_CompileCmd *cmd, char ***argv_out)
{
    char **argv = NULL;
    const char *standard;
    size_t i;

    if (!cmd || !cmd->compiler || !argv_out) {
        if (!cmd) TL_COMPILE_ERR("cmd is NULL", "did you call tl_compile_cmd_init() first?");
        else if (!cmd->compiler) TL_COMPILE_ERR("cmd->compiler is NULL", "did you call tl_compile_cmd_init() or tl_compile_set_compiler()?");
        else TL_COMPILE_ERR("argv_out is NULL", "pass a char*** to receive the rendered argument vector");
        return false;
    }

    if (!tl_compile_argv_push_dup(cmd, &argv, cmd->compiler)) goto fail;

    standard = tl_compile__standard_flag(cmd->standard);
    if (standard && !tl_compile_argv_push_dup(cmd, &argv, standard)) goto fail;

    for (i = 0; i < tl_arr_len(cmd->include_dirs); ++i) {
        if (!tl_compile_argv_push_owned(cmd, &argv, tl_compile_prefix_arg(cmd, "-I", cmd->include_dirs[i]))) goto fail;
    }
    for (i = 0; i < tl_arr_len(cmd->defines); ++i) {
        if (!tl_compile_argv_push_owned(cmd, &argv, tl_compile_prefix_arg(cmd, "-D", cmd->defines[i]))) goto fail;
    }
    for (i = 0; i < tl_arr_len(cmd->flags); ++i) {
        if (!tl_compile_argv_push_dup(cmd, &argv, cmd->flags[i])) goto fail;
    }

    if (cmd->output) {
        if (!tl_compile_argv_push_dup(cmd, &argv, "-o")) goto fail;
        if (!tl_compile_argv_push_dup(cmd, &argv, cmd->output)) goto fail;
    }

    for (i = 0; i < tl_arr_len(cmd->sources); ++i) {
        if (!tl_compile_argv_push_dup(cmd, &argv, cmd->sources[i])) goto fail;
    }
    for (i = 0; i < tl_arr_len(cmd->link_flags); ++i) {
        if (!tl_compile_argv_push_dup(cmd, &argv, cmd->link_flags[i])) goto fail;
    }
    for (i = 0; i < tl_arr_len(cmd->libs); ++i) {
        if (!tl_compile_argv_push_owned(cmd, &argv, tl_compile_prefix_arg(cmd, "-l", cmd->libs[i]))) goto fail;
    }

    if (!tl_arr_push(argv, NULL)) goto fail;
    *argv_out = argv;
    return true;

fail:
    tl_compile_argv_free(cmd, argv);
    return false;
}

void
tl_compile_argv_free(const TL_CompileCmd *cmd, char **argv)
{
    TL_Allocator *allocator = tl_compile__allocator(cmd);
    size_t i;

    if (!argv) return;
    for (i = 0; i < tl_arr_len(argv); ++i) {
        tl_compile__strfree(allocator, argv[i]);
    }
    tl_arr_free(argv);
}

static TL_BuildConfig g_tl_build_config = {0};
static bool g_tl_build_log_initialized = false;
static size_t g_tl_build_built_count;
static size_t g_tl_build_skipped_count;
static size_t g_tl_build_failed_count;
static struct timespec g_tl_build_time_start;
static struct timespec g_tl_build_target_time_start;
static bool g_tl_build_target_timer_active;

static
void
tl__build_target_begin(void)
{
    if (!g_tl_build_target_timer_active) {
        clock_gettime(CLOCK_MONOTONIC, &g_tl_build_target_time_start);
        g_tl_build_target_timer_active = true;
    }
}

static
void
tl_build_log_setting(const char *name, const char *value);

static
void
tl_build_log_write(TL_LogLevel level, const char *fmt, ...)
{
    va_list args;

    if (!g_tl_build_log_initialized) {
        TL_LogConfig cfg = {0};
        cfg.disable_time = 1;
        cfg.disable_level = 1;
        cfg.disable_file = 1;
        cfg.disable_line = 1;
        cfg.disable_func = 1;
        if (!tl_log_init(&cfg)) return;
        g_tl_build_log_initialized = true;
    }

    va_start(args, fmt);
    {
        char buffer[2048];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        tl_log_write_raw(level, NULL, 0, NULL, buffer);
    }
    va_end(args);
}

static
bool
tl_build_log_init(const TL_BuildConfig *cfg)
{
    TL_LogConfig log_cfg = {0};

    log_cfg.disable_time = 1;
    log_cfg.disable_level = 1;
    log_cfg.disable_file = 1;
    log_cfg.disable_line = 1;
    log_cfg.disable_func = 1;

    if (!tl_log_init(&log_cfg)) return false;
    g_tl_build_config = cfg ? *cfg : (TL_BuildConfig){0};
    g_tl_build_log_initialized = true;
    g_tl_build_built_count = 0;
    g_tl_build_skipped_count = 0;
    g_tl_build_failed_count = 0;
    clock_gettime(CLOCK_MONOTONIC, &g_tl_build_time_start);

    if (g_tl_build_config.project_name) {
        tl_build_log_write(TL_LOG_LEVEL_INFO, "-- Configuring %s", g_tl_build_config.project_name);
    } else {
        tl_build_log_write(TL_LOG_LEVEL_INFO, "-- Configuring build");
    }
    if (g_tl_build_config.build_dir) {
        tl_build_log_setting("build dir", g_tl_build_config.build_dir);
    }
    if (g_tl_build_config.compiler) {
        tl_build_log_setting("compiler", g_tl_build_config.compiler);
    }
    return true;
}

static
void
tl_build_log_setting(const char *name, const char *value)
{
    tl_build_log_write(TL_LOG_LEVEL_INFO, "-- %-12s %s", name ? name : "setting", value ? value : "(null)");
}

static
double
tl_build_elapsed_s(struct timespec *start)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)(now.tv_sec - start->tv_sec) +
           (double)(now.tv_nsec - start->tv_nsec) / 1e9;
}

static
void
tl_build_log_event(const char *kind, const char *name, const char *detail)
{
    TL_LogLevel level = TL_LOG_LEVEL_INFO;

    if (kind && strcmp(kind, "failed") == 0) level = TL_LOG_LEVEL_ERROR;
    if (detail) {
        tl_build_log_write(level, "-- %-12s %s (%s)", kind ? kind : "build", name ? name : "(null)", detail);
    } else {
        tl_build_log_write(level, "-- %-12s %s", kind ? kind : "build", name ? name : "(null)");
    }
}

static
void
tl_build_log_summary(void)
{
    TL_LogLevel level = g_tl_build_failed_count ? TL_LOG_LEVEL_ERROR : TL_LOG_LEVEL_INFO;
    double total = tl_build_elapsed_s(&g_tl_build_time_start);
    tl_build_log_write(level,
                       "-- Summary      built %lu, skipped %lu, failed %lu in %.3fs",
                       (unsigned long)g_tl_build_built_count,
                       (unsigned long)g_tl_build_skipped_count,
                       (unsigned long)g_tl_build_failed_count,
                       total);
}

static
void
tl_cmd_echo_argv(const char *prefix, const char *const argv[])
{
    size_t i;
    char buffer[4096];
    size_t used;

    if (!argv) return;
    used = (size_t)snprintf(buffer, sizeof(buffer), "-- %-12s", prefix ? prefix : "cmd");
    for (i = 0; argv[i]; ++i) {
        int n;
        if (used >= sizeof(buffer)) break;
        n = snprintf(buffer + used, sizeof(buffer) - used, " %s", argv[i]);
        if (n < 0) break;
        used += (size_t)n;
    }
    tl_build_log_write(TL_LOG_LEVEL_INFO, "%s", buffer);
}

bool
tl_build_target_skipped(const char *target, const char *reason)
{
    ++g_tl_build_skipped_count;
    tl_build_log_event("skipped", target, reason);
    return true;
}

static
bool
tl__build_target_built(const char *target)
{
    double elapsed;
    char buf[32];

    ++g_tl_build_built_count;
    elapsed = g_tl_build_target_timer_active ? tl_build_elapsed_s(&g_tl_build_target_time_start) : 0.0;
    g_tl_build_target_timer_active = false;
    snprintf(buf, sizeof(buf), "%.3fs", elapsed);
    if (elapsed > 0.0) {
        tl_build_log_event("built", target, buf);
    } else {
        tl_build_log_event("built", target, NULL);
    }
    return true;
}

static
bool
tl__build_target_failed(const char *target)
{
    double elapsed;
    char buf[32];

    ++g_tl_build_failed_count;
    elapsed = g_tl_build_target_timer_active ? tl_build_elapsed_s(&g_tl_build_target_time_start) : 0.0;
    g_tl_build_target_timer_active = false;
    snprintf(buf, sizeof(buf), "%.3fs", elapsed);
    if (elapsed > 0.0) {
        tl_build_log_event("failed", target, buf);
    } else {
        tl_build_log_event("failed", target, NULL);
    }
    return false;
}

bool
tl_build_target_finish(const char *target, TL_CmdResult result)
{
    return result.ok ? tl__build_target_built(target) : tl__build_target_failed(target);
}

TL_CmdResult
tl_cmd_run_ex(const char *const argv[], const TL_CmdOptions *options)
{
    TL_CmdResult result = {0};
    TL_CmdOptions resolved = {0};

    if (!argv || !argv[0]) {
        TL_COMPILE_ERR("argv is NULL or empty", "provide a NULL-terminated argument array, e.g. (const char *[]){ \"cc\", \"-o\", \"out\", \"in.c\", NULL }");
        result.exit_code = -1;
        return result;
    }

    tl__build_target_begin();

    resolved.echo = g_tl_build_log_initialized ? g_tl_build_config.verbose : true;
    if (options) {
        resolved.stdout_path = options->stdout_path;
        resolved.redirect_stderr = options->redirect_stderr;
        if (options->echo_override)
            resolved.echo = options->echo;
    }
    if (resolved.echo) tl_cmd_echo_argv("cmd", argv);

#if defined(_WIN32)
    result.exit_code = -1;
    result.ok = 0;
#else
    {
        pid_t pid = fork();
        if (pid == 0) {
            if (resolved.stdout_path) {
                int fd = open(resolved.stdout_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fd < 0) {
                    fprintf(stderr, "[ERROR] %s:%d %s(): failed to open `%s`: %s\n", __FILE__, __LINE__, __func__, resolved.stdout_path, strerror(errno));
                    _exit(EXIT_FAILURE);
                }
                if (dup2(fd, STDOUT_FILENO) < 0 ||
                    (resolved.redirect_stderr && dup2(fd, STDERR_FILENO) < 0)) {
                    fprintf(stderr, "[ERROR] %s:%d %s(): failed to redirect output: %s\n", __FILE__, __LINE__, __func__, strerror(errno));
                    close(fd);
                    _exit(EXIT_FAILURE);
                }
                close(fd);
            }
            execvp(argv[0], (char *const *)argv);
            fprintf(stderr, "[ERROR] %s:%d %s(): failed to execute `%s`: %s\n", __FILE__, __LINE__, __func__, argv[0], strerror(errno));
            _exit(127);
        } else if (pid < 0) {
            fprintf(stderr, "[ERROR] %s:%d %s(): fork failed: %s\n", __FILE__, __LINE__, __func__, strerror(errno));
            result.exit_code = -1;
            result.ok = 0;
        } else {
            int status = 0;
            if (waitpid(pid, &status, 0) < 0) {
                result.exit_code = -1;
                result.ok = 0;
            } else if (WIFEXITED(status)) {
                result.exit_code = WEXITSTATUS(status);
                result.ok = result.exit_code == 0;
            } else {
                result.exit_code = -1;
                result.ok = 0;
            }
        }
    }
#endif

    return result;
}

TL_CmdResult
tl_cmd_run(const char *const argv[])
{
    return tl_cmd_run_ex(argv, NULL);
}

TL_CmdResult
tl_compile_run(TL_CompileCmd *cmd)
{
    TL_CmdResult result = {0};
    TL_CmdOptions options = {0};
    char **argv = NULL;

    tl__build_target_begin();

    if (!tl_compile_render_argv(cmd, &argv)) {
        result.exit_code = -1;
        goto cleanup;
    }

    if (g_tl_build_config.log_path) {
        options.stdout_path = g_tl_build_config.log_path;
        options.redirect_stderr = true;
    }
    options.echo = g_tl_build_log_initialized ? g_tl_build_config.verbose : true;
    if (cmd && cmd->echo_override)
        options.echo = cmd->echo;
    if (options.echo) tl_cmd_echo_argv("compile", (const char *const *)argv);
    options.echo = false;
    result = tl_cmd_run_ex((const char *const *)argv, &options);

    tl_compile_argv_free(cmd, argv);
cleanup:
    if (cmd && cmd->allocator == &cmd->internal_allocator)
        tl_arena_destroy(&cmd->internal_arena);
    return result;
}

bool
tl_mkdir_if_needed(const char *path)
{
    if (!path) {
        TL_COMPILE_ERR("path is NULL", "pass the directory path to create");
        return false;
    }
#if defined(_WIN32)
    return false;
#else
    if (mkdir(path, 0777) == 0) return true;
    return errno == EEXIST;
#endif
}

bool
tl_remove_dir(const char *path)
{
#if defined(_WIN32)
    (void)path;
    return false;
#else
    DIR *handle;
    struct dirent *entry;
    struct stat st;
    bool ok = 1;

    if (!path) {
        TL_COMPILE_ERR("path is NULL", "pass the directory path to remove");
        return false;
    }
    if (lstat(path, &st) != 0) {
        TL_COMPILE_ERR_ERRNO("failed to stat directory", path, errno);
        return false;
    }
    if (!S_ISDIR(st.st_mode)) return false;

    handle = opendir(path);
    if (!handle) return false;

    while ((entry = readdir(handle)) != NULL) {
        char *child;
        size_t path_len = strlen(path);
        size_t name_len = strlen(entry->d_name);
        size_t total = path_len + 1 + name_len + 1;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        child = (char *)malloc(total);
        if (!child) { ok = 0; break; }
        memcpy(child, path, path_len);
        child[path_len] = '/';
        memcpy(child + path_len + 1, entry->d_name, name_len + 1);

        if (lstat(child, &st) == 0 && S_ISDIR(st.st_mode)) {
            if (!tl_remove_dir(child)) ok = 0;
        } else {
            if (unlink(child) != 0) ok = 0;
        }

        free(child);
        if (!ok) break;
    }

    closedir(handle);
    if (ok) {
        if (rmdir(path) != 0) ok = 0;
    }
    return ok;
#endif
}

bool
tl_copy_file(const char *src_path, const char *dst_path)
{
    FILE *src;
    FILE *dst;
    char buffer[8192];
    size_t n;
    bool ok = 1;

    if (!src_path || !dst_path) {
        if (!src_path) TL_COMPILE_ERR("src_path is NULL", "pass the source file path");
        if (!dst_path) TL_COMPILE_ERR("dst_path is NULL", "pass the destination file path");
        return false;
    }
    src = fopen(src_path, "rb");
    if (!src) {
        TL_LOG_ERROR("failed to open `%s`: %s", src_path, strerror(errno));
        return false;
    }
    dst = fopen(dst_path, "wb");
    if (!dst) {
        TL_LOG_ERROR("failed to open `%s`: %s", dst_path, strerror(errno));
        fclose(src);
        return false;
    }
    while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, n, dst) != n) {
            TL_LOG_ERROR("failed to write `%s`: %s", dst_path, strerror(errno));
            ok = 0;
            break;
        }
    }
    if (ferror(src)) {
        TL_LOG_ERROR("failed to read `%s`: %s", src_path, strerror(errno));
        ok = 0;
    }
    fclose(dst);
    fclose(src);
    return ok;
}

int
tl_diff_files(const char *expected_path, const char *actual_path)
{
    TL_CmdResult result;

    if (!expected_path || !actual_path) {
        if (!expected_path) TL_COMPILE_ERR("expected_path is NULL", "pass the expected (reference) file path");
        if (!actual_path) TL_COMPILE_ERR("actual_path is NULL", "pass the actual (generated) file path");
        return EXIT_FAILURE;
    }
    result = tl_cmd("diff", "-u", expected_path, actual_path);
    return result.ok ? EXIT_SUCCESS : (result.exit_code == 0 ? EXIT_FAILURE : result.exit_code);
}

int
tl_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count)
{
    struct stat output_stat;
    size_t i;

    if (!output_path || (!input_paths && input_paths_count > 0)) {
        if (!output_path) TL_COMPILE_ERR("output_path is NULL", "pass the output file path to check");
        else TL_COMPILE_ERR("input_paths is NULL but input_paths_count > 0", "set count to 0 or pass a valid array of input paths");
        return -1;
    }
    if (stat(output_path, &output_stat) != 0) {
        if (errno == ENOENT) return true;
        TL_COMPILE_ERR_ERRNO("failed to stat output", output_path, errno);
        return -1;
    }

    for (i = 0; i < input_paths_count; ++i) {
        struct stat input_stat;
        if (stat(input_paths[i], &input_stat) != 0) {
            TL_COMPILE_ERR_ERRNO("failed to stat input", input_paths[i], errno);
            return -1;
        }
        if (input_stat.st_mtime > output_stat.st_mtime) return true;
    }
    return false;
}

int
tl_needs_rebuild1(const char *output_path, const char *input_path)
{
    return tl_needs_rebuild(output_path, &input_path, 1U);
}

static
bool
tl_compile__push_const_paths(const char ***paths, const char **items, size_t count)
{
    size_t i;

    if (!paths || (!items && count > 0)) return false;
    for (i = 0; i < count; ++i) {
        if (!tl_arr_push(*paths, items[i])) return false;
    }
    return true;
}

int
tl_needs_rebuild_with_sources(const char *output_path,
                              const char **input_paths,
                              size_t input_paths_count,
                              const TL_SourceFindConfig *source_sets,
                              size_t source_sets_count)
{
    const char **deps = NULL;
    char ***found_sets = NULL;
    int needs = -1;
    size_t i;
    size_t j;

    if (!output_path ||
        (!input_paths && input_paths_count > 0) ||
        (!source_sets && source_sets_count > 0)) {
        if (!output_path) TL_COMPILE_ERR("output_path is NULL", "pass the output file path to check");
        else if (!input_paths && input_paths_count > 0)
            TL_COMPILE_ERR("input_paths is NULL but input_paths_count > 0", "set count to 0 or pass a valid array of input paths");
        else
            TL_COMPILE_ERR("source_sets is NULL but source_sets_count > 0", "set count to 0 or pass a valid array of TL_SourceFindConfig");
        return -1;
    }

    tl_arr_init(deps, NULL);
    tl_arr_init(found_sets, NULL);
    if (!tl_compile__push_const_paths(&deps, input_paths, input_paths_count)) goto done;

    for (i = 0; i < source_sets_count; ++i) {
        char **sources = NULL;
        if (!tl_source_find(&source_sets[i], &sources)) goto done;
        if (!tl_arr_push(found_sets, sources)) {
            tl_source_find_free(sources);
            goto done;
        }
        for (j = 0; j < tl_arr_len(sources); ++j) {
            if (!tl_arr_push(deps, sources[j])) goto done;
        }
    }

    needs = tl_needs_rebuild(output_path, deps, tl_arr_len(deps));

done:
    for (i = 0; i < tl_arr_len(found_sets); ++i) {
        tl_source_find_free(found_sets[i]);
    }
    tl_arr_free(found_sets);
    tl_arr_free(deps);
    return needs;
}

bool
tl_go_rebuild_urself(int argc, char **argv, const char *source_path)
{
#if defined(_WIN32)
    (void)argc;
    (void)argv;
    (void)source_path;
    return true;
#else
    const char *cc;
    int needs_rebuild;
    TL_CompileCmd cmd = {0};
    TL_CmdResult result;

    if (argc <= 0 || !argv || !argv[0] || !source_path) {
        if (argc <= 0) TL_COMPILE_ERR("argc <= 0", "pass main()'s argc/argv directly");
        else if (!argv || !argv[0])
            TL_COMPILE_ERR("argv or argv[0] is NULL", "pass main()'s argv directly; use TL_GO_REBUILD_URSELF(argc, argv) macro");
        else
            TL_COMPILE_ERR("source_path is NULL", "use TL_GO_REBUILD_URSELF(argc, argv) which passes __FILE__ automatically");
        return false;
    }

    needs_rebuild = tl_needs_rebuild1(argv[0], source_path);
    if (needs_rebuild <= 0) {
        if (needs_rebuild < 0) TL_LOG_ERROR("failed to stat self-rebuild inputs");
        return needs_rebuild == 0;
    }

    cc = getenv("CC");
    if (!cc) cc = "cc";

    if (!tl_compile_cmd_init(&cmd, NULL)) exit(EXIT_FAILURE);
    cmd.echo_override = true;
    cmd.echo = true;
    if (!tl_compile_set_compiler(&cmd, cc) ||
        !tl_compile_set_output(&cmd, argv[0]) ||
        !tl_compile_add_source(&cmd, source_path)) {
        tl_compile_cmd_free(&cmd);
        exit(EXIT_FAILURE);
    }

    result = tl_compile_run(&cmd);
    tl_compile_cmd_free(&cmd);
    if (!result.ok) exit(result.exit_code == 0 ? EXIT_FAILURE : result.exit_code);

    execv(argv[0], argv);
    TL_LOG_ERROR("failed to re-execute `%s`: %s", argv[0], strerror(errno));
    exit(EXIT_FAILURE);
#endif
}

static
void
tl_build_print_usage(const char *program,
                     const TL_BuildTarget *targets,
                     size_t targets_count);

static
int
tl_build_dispatch(int argc,
                  char **argv,
                  const TL_BuildTarget *targets,
                  size_t targets_count,
                  const char *default_target)
{
    const char *target;
    size_t i;

    if (!targets || targets_count == 0) return EXIT_FAILURE;
    target = argc > 1 ? argv[1] : default_target;
    if (!target) target = targets[0].name;

    for (i = 0; i < targets_count; ++i) {
        if (targets[i].name && strcmp(targets[i].name, target) == 0) {
            return (targets[i].run && targets[i].run()) ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }

    tl_build_print_usage((argc > 0 && argv && argv[0]) ? argv[0] : "build", targets, targets_count);
    return EXIT_FAILURE;
}

static
bool
tl_build_wants_help(int argc, char **argv)
{
    if (argc <= 1 || !argv || !argv[1]) return false;
    return strcmp(argv[1], "help") == 0 ||
           strcmp(argv[1], "-help") == 0 ||
           strcmp(argv[1], "--help") == 0;
}

int
tl_build_run(int argc, char **argv, const TL_BuildConfig *config)
{
    int status;
    const char *program = (argc > 0 && argv && argv[0]) ? argv[0] : "build";

    if (!config || !config->targets || config->targets_count == 0) {
        if (!config) TL_COMPILE_ERR("config is NULL", "create a TL_BuildConfig with targets");
        else TL_COMPILE_ERR("config->targets is NULL or empty", "add at least one target, e.g. { .name = \"app\", .run = build_app }");
        return EXIT_FAILURE;
    }
    if (tl_build_wants_help(argc, argv)) {
        tl_build_print_usage(program, config->targets, config->targets_count);
        return EXIT_SUCCESS;
    }

    if (!tl_build_log_init(config)) return EXIT_FAILURE;
    status = tl_build_dispatch(argc,
                               argv,
                               config->targets,
                               config->targets_count,
                               config->default_target);
    tl_build_log_summary();
    return status;
}

static
void
tl_build_print_usage(const char *program,
                     const TL_BuildTarget *targets,
                     size_t targets_count)
{
    size_t i;

    fprintf(stderr, "usage: %s [target]\n", program ? program : "build");
    fprintf(stderr, "targets:");
    for (i = 0; i < targets_count; ++i) {
        if (targets[i].name) fprintf(stderr, " %s", targets[i].name);
    }
    fputc('\n', stderr);
}

#endif /* TINYLIB_COMPILE_IMPL_ */
