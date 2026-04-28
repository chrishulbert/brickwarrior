// Main Sokol entry file.
// The intention re separation of responsibilities is for this to be game-agnostic, with no game-specific code here.

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

#include "game.h"

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
            .attrs = {
                [ATTR_basic_position].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_basic_texcoord].format = SG_VERTEXFORMAT_FLOAT2,
            },
        },
        .cull_mode = SG_CULLMODE_NONE,
        .depth = {
            .write_enabled = false,
            .compare = SG_COMPAREFUNC_ALWAYS,
        },
    });
    const float verts[] = {
        // X Y  U   V
        -1, -1, 0,  1, // Bottom-left.
         3, -1, 2,  1, // Bottom-right. (covers the screen)
        -1,  3, 0, -1, // Top-left. (covers the screen)
    };
    state.framebuffer_verts = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(verts),
    });
    state.framebuffer_sampler = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
    });
    state.framebuffer_image = sg_make_image(&(sg_image_desc){
        .width = SCREENWIDTH,
        .height = SCREENHEIGHT,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .usage.stream_update = true,
    });
    state.framebuffer_view = sg_make_view(&(sg_view_desc){
        .texture.image = state.framebuffer_image,
    });

    // Define the background color:
    state.pass_action = (sg_pass_action) {
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = { 0, 0, 0, 1 }
        }
    };

    // Let the game logic have a chance to init:
    game_init();
}

// Called every frame:
void frame(void) {
    // Let the game do its thing:
    game_update((float)sapp_frame_duration());
    game_draw(state.framebuffer);

    // Copy the framebuffer to the GPU:
    sg_update_image(state.framebuffer_image, &(sg_image_data){
        .mip_levels[0] = {
            .ptr = state.framebuffer,
            .size = SCREENWIDTH * SCREENHEIGHT * 4,
        }
    });

    // Start the pass:
    sg_begin_pass(&(sg_pass){
        .action = state.pass_action,
        .swapchain = sglue_swapchain(),
    });

    // Fix the aspect ratio:
    float target_aspect = (float)SCREENWIDTH / (float)SCREENHEIGHT;
    int window_width = sapp_width();
    int window_height = sapp_height();
    float window_aspect = (float)window_width / (float)window_height;
    int vp_x = 0, vp_y = 0, vp_w = window_width, vp_h = window_height;
    if (window_aspect > target_aspect) {
        // Window is too wide: add bars on the sides.
        vp_w = (int)(window_height * target_aspect);
        vp_x = (window_width - vp_w) / 2;
    } else {
        // Window is too tall: add bars on top/bottom.
        vp_h = (int)(window_width / target_aspect);
        vp_y = (window_height - vp_h) / 2;
    }
    sg_apply_viewport(vp_x, vp_y, vp_w, vp_h, true);

    // Draw the framebuffer:
    sg_apply_pipeline(state.framebuffer_pipeline);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = state.framebuffer_verts,
        .views[0] = state.framebuffer_view,
        .samplers[0] = state.framebuffer_sampler,
    });
    sg_draw(0, 3, 1);

    // Complete the pass:
    sg_end_pass();
    sg_commit();
}

// Called when the app shuts down:
void cleanup(void) {
    game_deinit();
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
        .width = SCREENWIDTH,
        .height = SCREENHEIGHT,
        .window_title = TITLE,
        .swap_interval = 1, // Enable VSync.
        .logger.func = slog_func,
    };
}
