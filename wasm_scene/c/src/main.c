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

/** Configs **/

static const char APP_NAME[] = "wasm_scene";
static const int APP_DEFAULT_WINDOW_WIDTH = 640;
static const int APP_DEFAULT_WINDOW_HEIGHT = 480;

static TL_LogConfig TL_LogGlobalCfg = {
    .level = TL_LOG_LEVEL_DEBUG,
};

struct AppState {
    const char *app_name;
} g_state;

/** life-cycle functions **/

static
void
init_cb(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });
}

static
void
frame_cb(void) {
    const sg_pass_action pass_action_clear = {
        .colors[0] =
            {
                .load_action = SG_LOADACTION_CLEAR,
                .clear_value = {0.8f, 0.8f, 0.2f, 1.0f},
            },
    };

    sg_begin_pass(&(sg_pass){
        .label     = "main_frame",
        .action    = pass_action_clear,
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
    LOG_INFO("Cleanup callback");

    sg_shutdown();
}

static
void
event_cb(const sapp_event* event) {
    LOG_INFO("Event cb");
    LOG_DEBUG("  keycode: %d", event->key_code);
}

sapp_desc
sokol_main(int argc, char* argv[]) {
    // tinylib stuff
    tl_log_init(&TL_LogGlobalCfg);

    // Init states
    g_state = (struct AppState){0};
    g_state.app_name = APP_NAME;

    // Handle arguments
    sargs_setup(&(sargs_desc){ .argc = argc, .argv = argv });
    for (int i = 0; i < sargs_num_args(); i++) {
        LOG_DEBUG("key: %s, value: %s", sargs_key_at(i), sargs_value_at(i));
    }

    if (sargs_exists("--help") || sargs_exists("-h")) {
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
