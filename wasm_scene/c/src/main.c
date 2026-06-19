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
#include "sokol/gfx.h"
#include "sokol/app.h"
#include "sokol/glue.h"

#define TL_SHORT_NAMES
#include "tinylib/tinylib.h"
#include "tinylib/tinylib.c"

#include <stdio.h>
#include <stdlib.h>


const char *s_app_name = "wasm_scene";

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
        .label = "main_frame",
        .action = g_state.pass_action,
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
}

static
void
event_cb(const sapp_event* event) {
    fprintf(stdout, "Event cb\n");
    fprintf(stdout, "  keycode: %d\n", event->key_code);
}

sapp_desc
sokol_main(int argc, char* argv[]) {
    g_state = (struct AppState){0};
    g_state.app_name = s_app_name;

    return (sapp_desc){
        .width = 640,
        .height = 480,
        .init_cb = init_cb,
        .frame_cb = frame_cb,
        .cleanup_cb = cleanup_cb,
        .event_cb = event_cb,
        .logger.func = slog_func,
    };
}
