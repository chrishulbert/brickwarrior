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

#define SCREENWIDTH (640)
#define SCREENHEIGHT (480)

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
            .compare = SG_COMPAREFUNC_ALWAYS, // Never?
        },
    });
    const float verts[] = {
        // X     Y      U      V
        -1.0f, -1.0f,  0.0f,  1.0f, // Bottom-left
        3.0f,  -1.0f,  2.0f,  1.0f, // Bottom-right (covers the screen)
        -1.0f,  3.0f,  0.0f, -1.0f  // Top-left (covers the screen)
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
            .clear_value = { 0.5f, 0.5f, 0.5f, 1.0f }
        }
    };
}

// Called every frame:s
void frame(void) {
    // Draw to the framebuffer:
    uint32_t palette[3] = {0xffff0000, 0xff00ff00, 0xff0000ff};
    for (int x=0; x<SCREENWIDTH; x++) {
        for (int y=0; y<SCREENHEIGHT; y++) {
            // 0xAABBGGRR
            state.framebuffer[y*SCREENWIDTH + x] = palette[rand() % 3];
        }
    }

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
