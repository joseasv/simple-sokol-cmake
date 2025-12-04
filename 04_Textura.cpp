#define SOKOL_IMPL
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"

// Incluimos el shader generado
#include "04_Textura.glsl.h"

#define CHECKERBOARD_SIZE 64

// Estructura para mantener el estado de nuestra aplicación
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    sg_image img;      // Handle de la imagen
    sg_sampler smp;    // Handle del sampler
    sg_view view;      // Handle de la VISTA de imagen
} state;

static void init(void) {
    // --- Configuración inicial de Sokol GFX ---
    sg_desc sgdesc = {};
    sgdesc.environment = sglue_environment();
    sgdesc.logger.func = slog_func;
    sg_setup(&sgdesc);

    // --- 1. Vértices y Buffer de Vértices ---
    typedef struct { float x, y; float u, v; } vertex_t;
    const vertex_t vertices[] = {
        { -0.5f, -0.5f,  0.0f, 0.0f }, {  0.5f, -0.5f,  1.0f, 0.0f },
        {  0.5f,  0.5f,  1.0f, 1.0f }, { -0.5f,  0.5f,  0.0f, 1.0f }
    };
    sg_buffer_desc vbuf_desc = {};
    vbuf_desc.data = SG_RANGE(vertices);
    vbuf_desc.label = "texture-vertices";
    state.bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

    // --- 2. Índices y Buffer de Índices ---
    const uint16_t indices[] = { 0, 1, 2,  0, 2, 3 };
    sg_buffer_desc ibuf_desc = {};
    ibuf_desc.data = SG_RANGE(indices);
    ibuf_desc.usage.index_buffer = true;
    ibuf_desc.label = "texture-indices";
    state.bind.index_buffer = sg_make_buffer(&ibuf_desc);

    // --- 3. Generar datos de la imagen ---
    uint32_t pixels[CHECKERBOARD_SIZE][CHECKERBOARD_SIZE];
    for (int y = 0; y < CHECKERBOARD_SIZE; y++) {
        for (int x = 0; x < CHECKERBOARD_SIZE; x++) {
            const bool is_white = ((x / 8) % 2) == ((y / 8) % 2);
            pixels[y][x] = is_white ? 0xFFFFFFFF : 0xFF000000;
        }
    }

    // --- 4. Crear objeto de imagen (texture) ---
    sg_image_desc img_desc = {};
    img_desc.width = CHECKERBOARD_SIZE;
    img_desc.height = CHECKERBOARD_SIZE;
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    img_desc.data.mip_levels[0] = SG_RANGE(pixels);
    img_desc.label = "checkerboard-texture";
    state.img = sg_make_image(&img_desc);

    // --- 5. Crear objeto sampler ---
    sg_sampler_desc smp_desc = {};
    smp_desc.min_filter = SG_FILTER_NEAREST;
    smp_desc.mag_filter = SG_FILTER_NEAREST;
    smp_desc.label = "checkerboard-sampler";
    state.smp = sg_make_sampler(&smp_desc);

    // --- 6. Crear una VISTA para la imagen ---
    sg_view_desc view_desc = {};
    view_desc.texture.image = state.img;
    state.view = sg_make_view(&view_desc);

    // --- 7. Vincular VISTA y sampler a los slots del shader ---
    state.bind.views[VIEW_tex].id = state.view.id;
    state.bind.samplers[SMP_smp].id = state.smp.id;

    // --- 8. Crear el Pipeline ---
    sg_pipeline_desc pip_desc = {};
    pip_desc.shader = sg_make_shader(textura_shader_desc(sg_query_backend()));
    pip_desc.index_type = SG_INDEXTYPE_UINT16;
    pip_desc.layout.attrs[ATTR_textura_pos].format = SG_VERTEXFORMAT_FLOAT2;
    pip_desc.layout.attrs[ATTR_textura_uv].format = SG_VERTEXFORMAT_FLOAT2;
    pip_desc.label = "texture-pipeline";
    state.pip = sg_make_pipeline(&pip_desc);

    // --- 9. Pass Action (color de fondo) ---
    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.3f, 0.3f, 0.3f, 1.0f };
}

void frame(void) {
    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();

    sg_begin_pass(&pass);
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_draw(0, 6, 1);
    sg_end_pass();
    sg_commit();
}

void cleanup(void) {
    // Destruir todos los recursos creados en orden inverso
    sg_destroy_pipeline(state.pip);
    sg_destroy_view(state.view);
    sg_destroy_sampler(state.smp);
    sg_destroy_image(state.img);
    sg_destroy_buffer(state.bind.index_buffer);
    sg_destroy_buffer(state.bind.vertex_buffers[0]);
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    sapp_desc desc = {};
    desc.init_cb = init;
    desc.frame_cb = frame;
    desc.cleanup_cb = cleanup;
    desc.width = 800;
    desc.height = 600;
    desc.window_title = "04 - Textura (Damero)";
    desc.icon.sokol_default = true;
    desc.logger.func = slog_func;
    return desc;
}