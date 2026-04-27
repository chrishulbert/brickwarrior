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

#include "shaders/basic.h"

#define SCREENWIDTH 640
#define SCREENHEIGHT 480

// State for our application:
static struct {
    sg_pass_action pass_action;
    uint32_t *framebuffer;
    sg_pipeline framebuffer_pipeline;
    sg_buffer framebuffer_verts;
    sg_image framebuffer_image;
    sg_view framebuffer_view;
    sg_sampler framebuffer_sampler;
} state;

// Called once at the start:
void init(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    // Make the framebuffer:
    state.framebuffer = malloc(SCREENWIDTH * SCREENHEIGHT * 4);
    state.framebuffer_pipeline = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(basic_shader_desc(sg_query_backend())),
        .layout = {
            .attrs[0].format = SG_VERTEXFORMAT_FLOAT2
        },
        .cull_mode = SG_CULLMODE_NONE,
        .depth = {
            .write_enabled = false,
            .compare = SG_COMPAREFUNC_NEVER,//SG_COMPAREFUNC_ALWAYS,
        },
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
    free(state.framebuffer);
    state.framebuffer = NULL;
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
        .window_title = "BrickWarrior",
        .swap_interval = 1, // Enable VSync.
        .logger.func = slog_func,
    };
}
