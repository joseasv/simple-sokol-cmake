#define SOKOL_IMPL
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdio.h> // Para printf
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"

// Incluimos el NUEVO shader generado
#include "04TexturaColor.glsl.h"

#define CHECKERBOARD_SIZE 64

// Estructura para mantener el estado de nuestra aplicación
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    sg_image img;
    sg_sampler smp;
    sg_view view;
} state;

static void init(void) {
    // --- Configuración inicial de Sokol GFX ---
    sg_desc sgdesc = {};
    sgdesc.environment = sglue_environment();
    sgdesc.logger.func = slog_func;
    sg_setup(&sgdesc);

    // --- 1. Vértices y Buffer de Vértices (con color) ---
    typedef struct { float x, y; float u, v; float r, g, b; } vertex_t;
    const vertex_t vertices[] = {
        // Posición      // Coordenada UV  // Color
        { -0.5f, -0.5f,  0.0f, 0.0f,       1.0f, 0.0f, 0.0f }, // Abajo-izq, Rojo
        {  0.5f, -0.5f,  1.0f, 0.0f,       0.0f, 1.0f, 0.0f }, // Abajo-der, Verde
        {  0.5f,  0.5f,  1.0f, 1.0f,       0.0f, 0.0f, 1.0f }, // Arriba-der, Azul
        { -0.5f,  0.5f,  0.0f, 1.0f,       1.0f, 1.0f, 0.0f }  // Arriba-izq, Amarillo
    };
    sg_buffer_desc vbuf_desc = {};
    vbuf_desc.data = SG_RANGE(vertices);
    vbuf_desc.label = "texture-color-vertices";
    state.bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

    // --- 2. Índices y Buffer de Índices ---
    const uint16_t indices[] = { 0, 1, 2,  0, 2, 3 };
    sg_buffer_desc ibuf_desc = {};
    ibuf_desc.data = SG_RANGE(indices);
    ibuf_desc.usage.index_buffer = true;
    ibuf_desc.label = "texture-color-indices";
    state.bind.index_buffer = sg_make_buffer(&ibuf_desc);

    // --- 3. Cargar imagen con STB_image o usar fallback ---
    int img_width, img_height, num_channels;
    const int desired_channels = 4;
    stbi_set_flip_vertically_on_load(true);
    stbi_uc* pixels = stbi_load("texturas/textura.png", &img_width, &img_height, &num_channels, desired_channels);

    sg_image_desc img_desc = {};
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;

    if (pixels) {
        printf("Imagen 'texturas/textura.png' cargada con éxito (%d x %d)\n", img_width, img_height);
        img_desc.width = img_width;
        img_desc.height = img_height;
        img_desc.data.mip_levels[0] = (sg_range){ .ptr = pixels, .size = (size_t)(img_width * img_height * 4) };
        img_desc.label = "texture-from-file";
    } else {
        printf("ADVERTENCIA: No se pudo cargar 'texturas/textura.png'. Usando textura de damero de fallback.\n");
        static uint32_t fallback_pixels[CHECKERBOARD_SIZE][CHECKERBOARD_SIZE];
        for (int y = 0; y < CHECKERBOARD_SIZE; y++) {
            for (int x = 0; x < CHECKERBOARD_SIZE; x++) {
                const bool is_white = ((x / 8) % 2) == ((y / 8) % 2);
                fallback_pixels[y][x] = is_white ? 0xFFFFFFFF : 0xFF000000;
            }
        }
        img_desc.width = CHECKERBOARD_SIZE;
        img_desc.height = CHECKERBOARD_SIZE;
        img_desc.data.mip_levels[0] = SG_RANGE(fallback_pixels);
        img_desc.label = "checkerboard-texture";
    }
    
    state.img = sg_make_image(&img_desc);

    if (pixels) {
        stbi_image_free(pixels);
    }

    // --- 4. Crear objeto sampler ---
    sg_sampler_desc smp_desc = {};
    smp_desc.min_filter = SG_FILTER_LINEAR;
    smp_desc.mag_filter = SG_FILTER_LINEAR;
    smp_desc.wrap_u = SG_WRAP_REPEAT;
    smp_desc.wrap_v = SG_WRAP_REPEAT;
    smp_desc.label = "texture-sampler";
    state.smp = sg_make_sampler(&smp_desc);
    
    // --- 5. Crear una VISTA para la imagen ---
    sg_view_desc view_desc = {};
    view_desc.texture.image = state.img;
    state.view = sg_make_view(&view_desc);

    // --- 6. Vincular VISTA y sampler a los slots del shader ---
    // NOTA: Los slots VIEW_tex y SMP_smp son generados por el shader.
    // Si el shader se llama 'textura_color', los slots podrían ser diferentes.
    // Se asume que el shader nuevo generará los mismos nombres de slot para tex y smp.
    state.bind.views[VIEW_tex].id = state.view.id;
    state.bind.samplers[SMP_smp].id = state.smp.id;

    // --- 7. Crear el Pipeline ---
    sg_pipeline_desc pip_desc = {};
    // Usamos la descripción del NUEVO shader
    pip_desc.shader = sg_make_shader(textura_color_shader_desc(sg_query_backend()));
    pip_desc.index_type = SG_INDEXTYPE_UINT16;
    // El layout ahora incluye el color. Los nombres de atributos vienen del shader.
    pip_desc.layout.attrs[ATTR_textura_color_pos].format = SG_VERTEXFORMAT_FLOAT2;
    pip_desc.layout.attrs[ATTR_textura_color_uv].format = SG_VERTEXFORMAT_FLOAT2;
    pip_desc.layout.attrs[ATTR_textura_color_color].format = SG_VERTEXFORMAT_FLOAT3;
    
    pip_desc.colors[0].blend.enabled = true;
    pip_desc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    pip_desc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pip_desc.label = "texture-color-pipeline";
    state.pip = sg_make_pipeline(&pip_desc);

    // --- 8. Pass Action (color de fondo) ---
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
    desc.window_title = "04 - Textura con Color";
    desc.icon.sokol_default = true;
    desc.logger.func = slog_func;
    return desc;
}