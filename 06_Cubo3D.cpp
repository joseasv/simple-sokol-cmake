// -----------------------------------------------------------------------------
// 06_Cubo3D.cpp
// Carga de textura "texturas/container.jpg" + Cubo 3D + Legacy API
// -----------------------------------------------------------------------------

// 1. Matemáticas primero (para evitar conflictos de macros en Linux)
#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_CPP_MODE
#define HANDMADE_MATH_USE_DEGREES
#include "HandmadeMath.h"

// 2. Implementación de librerías
#define SOKOL_IMPL
#define STB_IMAGE_IMPLEMENTATION // <--- Necesario para cargar imágenes
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "stb_image.h" // <--- Librería de carga de imágenes

#include <math.h>
#include <stdio.h>

// 3. Shader generado
#include "06_Cubo3D.glsl.h"

// Estado de la aplicación
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    sg_image img;
    sg_sampler smp;
    sg_view view; // Legacy API
    float rx, ry;
} state;

void init(void)
{
    // --- Configuración de Sokol ---
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    // --- 1. Vértices del Cubo (X, Y, Z, U, V) ---
    float vertices[] = {
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,

        -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,

        0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f
    };

    sg_buffer_desc vbuf_desc = {};
    vbuf_desc.data = SG_RANGE(vertices);
    state.bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

    // --- 2. Cargar Textura (container.jpg) ---
    int width, height, channels;
    const int desired_channels = 4; // Queremos RGBA

    // Invertimos la textura al cargar para que coincida con coordenadas OpenGL
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load("texturas/container.jpg", &width, &height, &channels, desired_channels);

    sg_image_desc img_desc = {};
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;

    if (data) {
        printf("Textura cargada: %dx%d canales: %d\n", width, height, channels);
        img_desc.width = width;
        img_desc.height = height;
        // API Legacy: Usamos mip_levels[0]
        img_desc.data.mip_levels[0] = (sg_range) { .ptr = data, .size = (size_t)(width * height * 4) };
        state.img = sg_make_image(&img_desc);

        // Liberamos la memoria de la CPU ya que se subió a la GPU
        stbi_image_free(data);
    } else {
        printf("ERROR: No se pudo cargar 'texturas/container.jpg'. Creando textura negra fallback.\n");
        // Fallback: 1 pixel negro
        uint32_t black_pixel = 0xFF000000;
        img_desc.width = 1;
        img_desc.height = 1;
        img_desc.data.mip_levels[0] = SG_RANGE(black_pixel);
        state.img = sg_make_image(&img_desc);
    }

    // --- 3. Crear Sampler ---
    sg_sampler_desc smp_desc = {};
    smp_desc.min_filter = SG_FILTER_LINEAR;
    smp_desc.mag_filter = SG_FILTER_LINEAR;
    smp_desc.wrap_u = SG_WRAP_REPEAT; // Repetir si las UV se salen
    smp_desc.wrap_v = SG_WRAP_REPEAT;
    state.smp = sg_make_sampler(&smp_desc);

    // --- 4. Crear Vista (API Legacy) ---
    sg_view_desc view_desc = {};
    view_desc.texture.image = state.img;
    state.view = sg_make_view(&view_desc);

    // --- 5. Vincular ---
    state.bind.views[VIEW_tex].id = state.view.id;
    state.bind.samplers[SMP_smp].id = state.smp.id;

    // --- 6. Pipeline con Z-Buffer ---
    sg_pipeline_desc pip_desc = {};
    pip_desc.shader = sg_make_shader(transformacion_shader_desc(sg_query_backend()));

    pip_desc.layout.buffers[0].stride = 5 * sizeof(float);
    pip_desc.layout.attrs[0].offset = 0;
    pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
    pip_desc.layout.attrs[1].offset = 3 * sizeof(float);
    pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;

    // Activar Depth Test
    // [DESCOMENTAR ESTO PARA ACTIVAR Z-BUFFER]
    // pip_desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    // pip_desc.depth.write_enabled = true;

    state.pip = sg_make_pipeline(&pip_desc);

    // --- 7. Clear Action ---
    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.2f, 0.3f, 0.3f, 1.0f };
    state.pass_action.depth.load_action = SG_LOADACTION_CLEAR;
    state.pass_action.depth.clear_value = 1.0f;
}

void frame(void)
{
    float t = (float)sapp_frame_duration();
    state.rx += 30.0f * t;
    state.ry += 50.0f * t;

    float width = (float)sapp_width();
    float height = (float)sapp_height();
    float aspect = width / height;

    HMM_Mat4 proj = HMM_Perspective_RH_NO(60.0f, aspect, 0.01f, 100.0f);
    HMM_Mat4 view = HMM_Translate(HMM_V3(0.0f, 0.0f, -3.0f));

    HMM_Mat4 model = HMM_Rotate_RH(state.rx, HMM_V3(1.0f, 0.0f, 0.0f));
    model = model * HMM_Rotate_RH(state.ry, HMM_V3(0.0f, 1.0f, 0.0f));

    HMM_Mat4 mvp = proj * view * model;

    // Usamos struct anónimo compatible con el shader si no quieres definir tipos
    struct {
        HMM_Mat4 mvp;
    } vs_params;
    vs_params.mvp = mvp;

    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);

    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    sg_range params_range = SG_RANGE(vs_params);
    sg_apply_uniforms(0, &params_range);

    sg_draw(0, 36, 1);
    sg_end_pass();
    sg_commit();
}

void cleanup(void)
{
    sg_destroy_pipeline(state.pip);
    sg_destroy_view(state.view);
    sg_destroy_sampler(state.smp);
    sg_destroy_image(state.img);
    sg_destroy_buffer(state.bind.vertex_buffers[0]);
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    sapp_desc desc = {};
    desc.init_cb = init;
    desc.frame_cb = frame;
    desc.cleanup_cb = cleanup;
    desc.width = 800;
    desc.height = 600;
    desc.window_title = "Cubo con Container.jpg";
    desc.logger.func = slog_func;
    return desc;
}