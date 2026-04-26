// Brickwarrior main Sokol entry file.

// Auto-select thee platform graphics backend:
#if defined(_WIN32)
    #define SOKOL_D3D11
#elif defined(__APPLE__)
    #define SOKOL_METAL
#else // Linux:
    #define SOKOL_GLCORE33
#endif

#define SOKOL_IMPL // So the implementation is compiled in.

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "sokol_glue.h"

// State for our application:
static struct {
    sg_pass_action pass_action;
} state;

// Called once at the start:
void init(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    // Define the background color:
    state.pass_action = (sg_pass_action) {
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = { 0.5f, 0.5f, 0.5f, 1.0f }
        }
    };
}

// Called every frame:s
void frame(void) {
    sg_begin_pass(&(sg_pass){ .action = state.pass_action, .swapchain = sglue_swapchain() });
    sg_end_pass();
    sg_commit();
}

// Called when the app shuts down:
void cleanup(void) {
    sg_shutdown();
}

// Application entry point configuration:
sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .width = 640,
        .height = 480,
        .window_title = "Brickwarrior",
        .swap_interval = 1, // Enable VSync.
        .logger.func = slog_func,
        .icon.sokol_default = true,
    };
}
