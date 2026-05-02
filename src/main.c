// Main Sokol entry file.
// The intention re separation of responsibilities is for this to be game-agnostic, with no game-specific code here.

#define SOKOL_IMPL // So the implementation is compiled in.
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_audio.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "shaders/basic.h"

#include "main.h"
#include "game.h"
#include "mixer.h"

#define QUEUES_LEN 20 // Length of the queue arrays. Actual queueable events are this-1 because it's null terminated.

// State for our application:
static struct {
    sg_pass_action pass_action;
    uint32_t *framebuffer;
    sg_pipeline framebuffer_pipeline;
    sg_buffer framebuffer_verts;
    sg_image framebuffer_image;
    sg_view framebuffer_view;
    sg_sampler framebuffer_sampler;
    int keys[QUEUES_LEN]; // Keys like F1, Escape (also includes characters, but don't use it for that).
    int chars[QUEUES_LEN]; // For text input, as it takes shift into account.
    MouseEvent mouseEvents[QUEUES_LEN];
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

    // Audio:
    saudio_setup(&(saudio_desc){
        .sample_rate = SAMPLE_RATE,
        .num_channels = 1, // Mono.
        .stream_cb = mixer_stream_callback,
        .logger = {
            .func = slog_func,
        }
    });


    // Define the background color:
    state.pass_action = (sg_pass_action) {
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = { 0, 0, 0, 1 }
        }
    };

    #ifdef __APPLE__
        // macOS helper to snap the CWD to the Resources folder so files can load.
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
        char path[PATH_MAX];
        if (CFURLGetFileSystemRepresentation(resourcesURL, true, (UInt8 *)path, PATH_MAX)) {
            chdir(path); // Change Working Directory to /Contents/Resources
            // Path is the current folder if run outside of a .app.
        }
        CFRelease(resourcesURL);

        // Add a menu for macOS so quit works:
        // https://github.com/floooh/sokol/pull/631/changes
        NSMenuItem* quit_menu_item = [[NSMenuItem alloc]
            initWithTitle:@"Quit"
            action:@selector(terminate:)
            keyEquivalent:@"q"];
        
        NSMenu* app_menu = [[NSMenu alloc] init];
        [app_menu addItem:quit_menu_item];

        NSMenuItem* app_menu_item = [[NSMenuItem alloc] init];
        app_menu_item.submenu = app_menu;
        
        NSMenu* menu_bar = [[NSMenu alloc] init];
        [menu_bar addItem:app_menu_item];

        NSApp.mainMenu = menu_bar;
    #endif

    // Let the game logic have a chance to init:
    game_init();
}

// Called every frame:
void frame(void) {
    // Update the game:
    game_update(sapp_frame_duration(), state.keys, state.chars, state.mouseEvents);
    state.keys[0] = 0; // Empty the event queues.
    state.chars[0] = 0;
    state.mouseEvents[0].isActual = false;
    if (game_should_quit()) {
        sapp_request_quit();
    }

    // Draw the game:
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

// Called on key events:
void event(const sapp_event* ev) {
    switch (ev->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            for (int i=0; i<QUEUES_LEN-1; i++) { // -1 to leave space for the null terminator.
                if (state.keys[i] == 0) {
                    state.keys[i] = ev->key_code;
                    state.keys[i+1] = 0;
                    break;
                }
            }
            break;
        case SAPP_EVENTTYPE_CHAR:
            for (int i=0; i<QUEUES_LEN-1; i++) { // -1 to leave space for the null terminator.
                if (state.chars[i] == 0) {
                    state.chars[i] = ev->char_code;
                    state.chars[i+1] = 0;
                    break;
                }
            }
            break;
        case SAPP_EVENTTYPE_MOUSE_DOWN:
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            for (int i=0; i<QUEUES_LEN-1; i++) {
                if (!state.mouseEvents[i].isActual) {
                    state.mouseEvents[i].isActual = true;
                    state.mouseEvents[i].isClick = ev->type == SAPP_EVENTTYPE_MOUSE_DOWN;
                    state.mouseEvents[i].button = ev->mouse_button;
                    state.mouseEvents[i].x = ev->mouse_x;
                    state.mouseEvents[i].y = ev->mouse_y;
                    state.mouseEvents[i].dx = ev->mouse_dx;
                    state.mouseEvents[i].dy = ev->mouse_dy;
                    state.mouseEvents[i+1].isActual = false;
                }
            }
            break;
        default:
            break;
    }
}

// Called when the app shuts down:
void cleanup(void) {
    game_deinit();
    free(state.framebuffer);
    state.framebuffer = NULL;
    sg_shutdown();
    saudio_shutdown();
}

// Application entry point configuration:
sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = SCREENWIDTH*2,
        .height = SCREENHEIGHT*2,
        .window_title = TITLE,
        .swap_interval = 1, // Enable VSync.
        .logger.func = slog_func,
    };
}

// Helpers for the game to call that abstract away the graphics library:

void lock_mouse(bool lock) {
    sapp_lock_mouse(lock);
}

bool is_mouse_locked() {
    return sapp_mouse_locked();
}
