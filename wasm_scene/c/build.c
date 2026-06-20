/*  build.c — wasm_scene build script
 *
 *  First-time bootstrap:
 *    cc -o build build.c && ./build
 *
 *  Usage:
 *    ./build [debug|release|clean]
 */

#define TL_SHORT_NAMES
#include "include/tinylib/compile.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static
const char *
dawn_dir(void)
{
    const char *dir = getenv("DAWN_DIR");
    return dir ? dir : "/usr/local";
}

static
void
add_native_platform(CompileCmd *cmd)
{
#if defined(__APPLE__)
    compile_flags(cmd, "-x", "objective-c");
    {
        char buf[512];
        snprintf(buf, sizeof(buf), "%s/include", dawn_dir());
        compile_include(cmd, buf);
        compile_link_flags(cmd, "-x", "none");
        snprintf(buf, sizeof(buf), "%s/lib/libwebgpu_dawn.a", dawn_dir());
        compile_link_flag(cmd, buf);
    }
    compile_libs(cmd, "c++", "objc");
    compile_link_flags(cmd,
        "-framework", "AppKit",
        "-framework", "QuartzCore",
        "-framework", "Metal",
        "-framework", "CoreFoundation",
        "-framework", "IOKit",
        "-framework", "IOSurface");

#elif defined(__linux__)
    {
        char buf[512];
        snprintf(buf, sizeof(buf), "%s/include", dawn_dir());
        compile_include(cmd, buf);
        snprintf(buf, sizeof(buf), "%s/lib/libwebgpu_dawn.a", dawn_dir());
        compile_link_flag(cmd, buf);
    }
    compile_libs(cmd, "c++", "dl", "pthread", "m", "X11", "Xi", "Xcursor");

#elif defined(_WIN32)
    {
        char buf[512];
        snprintf(buf, sizeof(buf), "%s\\include", dawn_dir());
        compile_include(cmd, buf);
        snprintf(buf, sizeof(buf), "%s\\lib\\webgpu_dawn.lib", dawn_dir());
        compile_link_flag(cmd, buf);
    }
    compile_libs(cmd, "c++", "kernel32", "user32", "shell32");
#endif
}

static
bool
build_debug(void)
{
    CompileCmd cmd = {0};
    CmdResult  result;

    compile_cmd_init(&cmd, NULL);

    compile_set_standard(&cmd, TL_C_STD_GNU11);
    compile_flags(&cmd,
        "-Wshadow", "-Wconversion", "-Wformat=2", "-Wundef",
        "-Wmissing-prototypes", "-Wimplicit-fallthrough");

    compile_include(&cmd, "include");
    compile_source(&cmd, "src/main.c");
    add_native_platform(&cmd);

    compile_flags(&cmd, "-O0", "-g");
    compile_define(&cmd, "DEBUG");
    compile_flags(&cmd, "-fsanitize=address,undefined", "-fno-omit-frame-pointer");
    compile_link_flag(&cmd, "-fsanitize=address,undefined");

    compile_set_output(&cmd, "target/app");
    mkdir_if_needed("target");

    result = compile_run(&cmd);

    return tl_build_target_finish("debug", result);
}

static
bool
build_release(void)
{
    CompileCmd cmd = {0};
    CmdResult  result;

    compile_cmd_init(&cmd, NULL);

    compile_set_standard(&cmd, TL_C_STD_GNU11);
    compile_flags(&cmd,
        "-Wshadow", "-Wconversion", "-Wformat=2", "-Wundef",
        "-Wmissing-prototypes", "-Wimplicit-fallthrough");

    compile_include(&cmd, "include");
    compile_source(&cmd, "src/main.c");
    add_native_platform(&cmd);

    compile_apply_preset(&cmd, &tl_compile_preset_release);

    compile_set_output(&cmd, "target/app");
    mkdir_if_needed("target");

    result = compile_run(&cmd);

    return tl_build_target_finish("release", result);
}

static
bool
build_clean(void)
{
    return tl_build_target_finish("clean", (CmdResult){ .ok = remove_dir("target") });
}

int
main(int argc, char **argv)
{
    static const BuildTarget targets[] = {
        { "debug",   build_debug },
        { "release", build_release },
        { "clean",   build_clean },
    };

    BuildConfig build = {
        .project_name   = "wasm_scene",
        .build_dir      = "target",
        .compiler       = "cc",
        .default_target = "debug",
        .targets        = targets,
        .targets_count  = TL_COUNT_OF(targets),
    };

    return tl_build_run_auto(argc, argv, &build);
}
