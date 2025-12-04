#define SOKOL_IMPL
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdio.h> // Para printf
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"

// Incluimos el shader generado para este ejemplo
#include "04_2Texturas.glsl.h"

#define CHECKERBOARD_SIZE 64

// El estado ahora necesita manejar dos de (casi) todo
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    sg_image img1, img2;
    sg_sampler smp1, smp2;
    sg_view view1, view2;
} state;

static void init(void) {
    sg_desc sgdesc = {};
    sgdesc.environment = sglue_environment();
    sgdesc.logger.func = slog_func;
    sg_setup(&sgdesc);

    // --- Buffers de vértices e índices (sin cambios) ---
    typedef struct { float x, y; float u, v; } vertex_t;
    const vertex_t vertices[] = {
        { -0.5f, -0.5f,  0.0f, 0.0f }, {  0.5f, -0.5f,  1.0f, 0.0f },
        {  0.5f,  0.5f,  1.0f, 1.0f }, { -0.5f,  0.5f,  0.0f, 1.0f }
    };
    sg_buffer_desc vbuf_desc = {};
    vbuf_desc.data = SG_RANGE(vertices);
    vbuf_desc.label = "quad-vertices";
    state.bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

    const uint16_t indices[] = { 0, 1, 2,  0, 2, 3 };
    sg_buffer_desc ibuf_desc = {};
    ibuf_desc.data = SG_RANGE(indices);
    ibuf_desc.usage.index_buffer = true;
    ibuf_desc.label = "quad-indices";
    state.bind.index_buffer = sg_make_buffer(&ibuf_desc);

    // --- Textura 1: Cargar desde archivo ---
    int img_width, img_height, num_channels;
    stbi_set_flip_vertically_on_load(true);
    stbi_uc* pixels1 = stbi_load("texturas/textura.png", &img_width, &img_height, &num_channels, 4);
    sg_image_desc img1_desc = {};
    img1_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    if (pixels1) {
        printf("Imagen 1 ('texturas/textura.png') cargada con éxito.\n");
        img1_desc.width = img_width;
        img1_desc.height = img_height;
        img1_desc.data.mip_levels[0] = (sg_range){ .ptr = pixels1, .size = (size_t)(img_width * img_height * 4) };
        img1_desc.label = "texture-1";
    } else {
        // Si falla, usamos un damero para la textura 1 también
        printf("ADVERTENCIA: No se pudo cargar 'texturas/textura.png'.\n");
        img1_desc.width = CHECKERBOARD_SIZE;
        img1_desc.height = CHECKERBOARD_SIZE;
        static uint32_t fallback_pixels[CHECKERBOARD_SIZE][CHECKERBOARD_SIZE];
        for (int y=0; y<CHECKERBOARD_SIZE; y++) for (int x=0; x<CHECKERBOARD_SIZE; x++) fallback_pixels[y][x] = 0xFF0000FF; // Azul
        img1_desc.data.mip_levels[0] = SG_RANGE(fallback_pixels);
    }
    state.img1 = sg_make_image(&img1_desc);
    if (pixels1) {
        stbi_image_free(pixels1);
    }

    // --- Textura 2: Cargar 'container.jpg' o usar fallback ---
    stbi_uc* pixels2 = stbi_load("texturas/container.jpg", &img_width, &img_height, &num_channels, 4);
    sg_image_desc img2_desc = {};
    img2_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    if (pixels2) {
        printf("Imagen 2 ('texturas/container.jpg') cargada con éxito.\n");
        img2_desc.width = img_width;
        img2_desc.height = img_height;
        img2_desc.data.mip_levels[0] = (sg_range){ .ptr = pixels2, .size = (size_t)(img_width * img_height * 4) };
        img2_desc.label = "texture-2-container";
    } else {
        printf("ADVERTENCIA: No se pudo cargar 'texturas/container.jpg'. Usando textura de damero de fallback para la segunda textura.\n");
        img2_desc.width = CHECKERBOARD_SIZE;
        img2_desc.height = CHECKERBOARD_SIZE;
        static uint32_t fallback_pixels2[CHECKERBOARD_SIZE][CHECKERBOARD_SIZE];
        for (int y = 0; y < CHECKERBOARD_SIZE; y++) {
            for (int x = 0; x < CHECKERBOARD_SIZE; x++) {
                const bool is_white = ((x / 8) % 2) == ((y / 8) % 2);
                fallback_pixels2[y][x] = is_white ? 0xFFFFFFFF : 0xFF000000;
            }
        }
        img2_desc.data.mip_levels[0] = SG_RANGE(fallback_pixels2);
        img2_desc.label = "texture-2-checkerboard-fallback";
    }
    state.img2 = sg_make_image(&img2_desc);
    if (pixels2) {
        stbi_image_free(pixels2);
    }

    // --- Samplers y Vistas ---
    sg_sampler_desc smp_desc = { .min_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_LINEAR };
    state.smp1 = sg_make_sampler(&smp_desc);
    state.smp2 = sg_make_sampler(&smp_desc);

    // Inicialización C++ compatible para sg_view_desc
    sg_view_desc view1_desc = {};
    view1_desc.texture.image = state.img1;
    sg_view_desc view2_desc = {};
    view2_desc.texture.image = state.img2;
    state.view1 = sg_make_view(&view1_desc);
    state.view2 = sg_make_view(&view2_desc);

    // --- Vinculación (Bindings) ---
    // Usamos los nombres de slot correctos generados por sokol-shdc
    state.bind.views[VIEW_tex1].id = state.view1.id;
    state.bind.samplers[SMP_smp1].id = state.smp1.id;
    state.bind.views[VIEW_tex2].id = state.view2.id;
    state.bind.samplers[SMP_smp2].id = state.smp2.id;

    // --- Pipeline ---
    sg_pipeline_desc pip_desc = {};
    pip_desc.shader = sg_make_shader(dos_texturas_shader_desc(sg_query_backend()));
    pip_desc.index_type = SG_INDEXTYPE_UINT16;
    pip_desc.layout.attrs[ATTR_dos_texturas_pos].format = SG_VERTEXFORMAT_FLOAT2;
    pip_desc.layout.attrs[ATTR_dos_texturas_uv].format = SG_VERTEXFORMAT_FLOAT2;
    pip_desc.colors[0].blend.enabled = true;
    pip_desc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    pip_desc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pip_desc.label = "two-textures-pipeline";
    state.pip = sg_make_pipeline(&pip_desc);

    // --- Pass Action ---
    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.3f, 0.3f, 0.3f, 1.0f };
}

void frame(void) {
    sg_pass pass = { .action = state.pass_action, .swapchain = sglue_swapchain() };
    sg_begin_pass(&pass);
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_draw(0, 6, 1);
    sg_end_pass();
    sg_commit();
}

void cleanup(void) {
    sg_destroy_pipeline(state.pip);
    sg_destroy_view(state.view1);
    sg_destroy_view(state.view2);
    sg_destroy_sampler(state.smp1);
    sg_destroy_sampler(state.smp2);
    sg_destroy_image(state.img1);
    sg_destroy_image(state.img2);
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
    desc.window_title = "04 - Dos Texturas";
    desc.icon.sokol_default = true;
    desc.logger.func = slog_func;
    return desc;
}