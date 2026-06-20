/* -*- c -*-
 * vi: ft=c
 *
 * @project: wasm_scene
 * @author:  Ziyue Ink Zhang
 * @date:    2026 06/19
 */

#define SOKOL_IMPL
#define SOKOL_WGPU
#include "sokol/log.h"
#include "sokol/args.h"
#include "sokol/gfx.h"
#include "sokol/app.h"
#include "sokol/glue.h"

#define TL_SHORT_NAMES
#include "tinylib/tinylib.h"
#include "tinylib/tinylib.c"

#include <stdio.h>
#include <stdlib.h>


const char *s_app_name = "wasm_scene";
#define APP_DEFAULT_WINDOW_WIDTH 640
#define APP_DEFAULT_WINDOW_HEIGHT 480

struct AppState {
    const char *app_name;
    sg_pass_action pass_action;
} g_state;


static
void
init_cb(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });
    g_state.pass_action = (sg_pass_action) {
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = {0.8f, 0.8f, 0.2f, 1.0f},
        },
    };

    // void *devices = sapp_wgpu_get_device();
    // assert(devices && "WGPU not supported!");
}

static
void
frame_cb(void) {
    sg_begin_pass(&(sg_pass){
        .label     = "main_frame",
        .action    = g_state.pass_action,
        .swapchain = sglue_swapchain(),
    });
    {
        // draw calls
    }
    sg_end_pass();
    sg_commit();
}

static
void
cleanup_cb(void) {
    fprintf(stdout, "Cleanup callback\n");

    sg_shutdown();
}

static
void
event_cb(const sapp_event* event) {
    fprintf(stdout, "Event cb\n");
    fprintf(stdout, "  keycode: %d\n", event->key_code);
}

sapp_desc
sokol_main(int argc, char* argv[]) {
    // tinylib stuff
    tl_log_init(&(TL_LogConfig){
        .level = TL_LOG_LEVEL_INFO,
        .output = TL_LOG_OUTPUT_STDOUT,
        .error_output = TL_LOG_OUTPUT_STDERR,
        .filename = NULL,
        .append = true,
        .auto_flush = true,
        .show_time = true,
        .show_level = true,
        .show_file = true,
        .show_line = true,
        .show_func = true,
    });

    // Init states
    g_state = (struct AppState){0};
    g_state.app_name = s_app_name;

    // Handle arguments
    sargs_setup(&(sargs_desc){
        .argc = argc,
        .argv = argv
    });

    if (sargs_exists("help")) {
        fprintf(stdout,
            "Usage: [%s] <opts>\n"
            "Opts:\n"
            "  --help: show this help message\n",
            argv[0]);
        exit(EXIT_SUCCESS);  // haven't allocate anything yet
    }

    return (sapp_desc){
        .width       = APP_DEFAULT_WINDOW_WIDTH,
        .height      = APP_DEFAULT_WINDOW_HEIGHT,
        .init_cb     = init_cb,
        .frame_cb    = frame_cb,
        .cleanup_cb  = cleanup_cb,
        .event_cb    = event_cb,
        .logger.func = slog_func,
    };
}
