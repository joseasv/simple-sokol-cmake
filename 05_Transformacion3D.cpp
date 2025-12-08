#define SOKOL_IMPL
#define STB_IMAGE_IMPLEMENTATION
#define SOKOL_TIME_IMPL

// Configuración de HMM
#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_CPP_MODE
#define HANDMADE_MATH_USE_DEGREES
#include "libs/HandmadeMath.h"

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "sokol_time.h"
#include "stb_image.h"
#include <string.h> // <--- [NUEVO] Necesario para memcpy

#include "05_Transformacion3D.glsl.h"

// ... (Tu struct 'state' y función 'init' NO cambian, déjalos igual) ...
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    sg_image img;
    sg_sampler smp;
    sg_view view;
} state;

static void init(void)
{
    // ... (Copia aquí el contenido de tu init anterior que ya funcionaba) ...
    // Para no alargar la respuesta, asumo que usas el mismo init() que
    // me pasaste antes, que estaba correcto.
    sg_desc sgdesc = {};
    sgdesc.environment = sglue_environment();
    sgdesc.logger.func = slog_func;
    sg_setup(&sgdesc);
    stm_setup();

    // Vértices
    float vertices[] = {
        -0.5f, -0.5f, 0.0f, 0.0f,
        0.5f, -0.5f, 1.0f, 0.0f,
        0.5f, 0.5f, 1.0f, 1.0f,
        -0.5f, 0.5f, 0.0f, 1.0f
    };
    sg_buffer_desc vbuf_desc = { 0 };
    vbuf_desc.data = SG_RANGE(vertices);
    state.bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

    // Índices
    const uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };
    sg_buffer_desc ibuf_desc = {};
    ibuf_desc.data = SG_RANGE(indices);
    ibuf_desc.usage.index_buffer = true;
    ibuf_desc.label = "quad-indices";
    state.bind.index_buffer = sg_make_buffer(&ibuf_desc);

    // Textura
    int w, h, c;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* pixels = stbi_load("texturas/textura.png", &w, &h, &c, 4);

    sg_image_desc img_desc = {};
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    img_desc.width = w;
    img_desc.height = h;
    if (pixels)
        img_desc.data.mip_levels[0] = (sg_range) { pixels, (size_t)(w * h * 4) };

    state.img = sg_make_image(&img_desc);
    if (pixels)
        stbi_image_free(pixels);

    // Sampler y Vista
    sg_sampler_desc smp_desc = { 0 };
    smp_desc.min_filter = SG_FILTER_LINEAR;
    smp_desc.mag_filter = SG_FILTER_LINEAR;
    state.smp = sg_make_sampler(&smp_desc);
    sg_view_desc view_desc = {};
    view_desc.texture.image = state.img;
    state.view = sg_make_view(&view_desc);

    state.bind.views[VIEW_tex].id = state.view.id;
    state.bind.samplers[SMP_smp].id = state.smp.id;

    // Pipeline
    sg_pipeline_desc pip_desc = {};
    pip_desc.shader = sg_make_shader(transformacion_shader_desc(sg_query_backend()));
    pip_desc.index_type = SG_INDEXTYPE_UINT16;
    pip_desc.layout.attrs[ATTR_transformacion_pos].format = SG_VERTEXFORMAT_FLOAT2;
    pip_desc.layout.attrs[ATTR_transformacion_uv].format = SG_VERTEXFORMAT_FLOAT2;
    pip_desc.colors[0].blend.enabled = true;
    pip_desc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
    pip_desc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    state.pip = sg_make_pipeline(&pip_desc);

    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.2f, 0.2f, 0.2f, 1.0f };
}
// ... (Fin del init resumen) ...

void frame()
{
    // 1. Configuración de dimensiones
    float width = (float)sapp_width();
    float height = (float)sapp_height();
    float aspect = width / height;

    // 2. Definir Matrices (Usando tu lógica 3D nueva)
    // MODELO: Rotación en X para inclinar el plano (-55 grados)
    HMM_Mat4 model = HMM_Rotate_RH(-55.0f, HMM_V3(1.0f, 0.0f, 0.0f));

    // VISTA: Mover la cámara/mundo hacia atrás
    HMM_Mat4 view = HMM_Translate(HMM_V3(0.0f, 0.0f, -3.0f));

    // PROYECCIÓN: Perspectiva
    HMM_Mat4 projection = HMM_Perspective_RH_NO(45.0f, aspect, 0.1f, 100.0f);

    // MVP FINAL
    HMM_Mat4 mvp = projection * view * model;

    // 3. Preparar datos para el Shader
    vs_params_t vs_params;
    // CORRECCIÓN 1: Usamos .transform porque así se llama en tu GLSL
    memcpy(vs_params.transform, &mvp, sizeof(mvp));

    // 4. Iniciar Pass (Renderizado)
    // CORRECCIÓN 2: Crear variable explícita para sg_pass (Sintaxis C++)
    sg_pass pass_desc = { 0 };
    pass_desc.action = state.pass_action;
    pass_desc.swapchain = sglue_swapchain();
    sg_begin_pass(&pass_desc);

    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    // 5. Aplicar Uniformes
    // CORRECCIÓN 3: Crear variable explícita para el rango
    sg_range params_range = SG_RANGE(vs_params);

    // CORRECCIÓN 4: Usar SG_SHADERSTAGE_VERTEX
    // El '0' es el binding=0 que pusiste en el shader layout(binding=0)
    sg_apply_uniforms(0, &params_range);

    sg_draw(0, 6, 1);
    sg_end_pass();
    sg_commit();
}

void cleanup(void)
{
    sg_destroy_pipeline(state.pip);
    sg_destroy_view(state.view);
    sg_destroy_sampler(state.smp);
    sg_destroy_image(state.img);
    sg_destroy_buffer(state.bind.index_buffer);
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
    desc.window_title = "05 - Transformacion 2D (HMM)";
    desc.logger.func = slog_func;
    return desc;
}