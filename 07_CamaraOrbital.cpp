// -----------------------------------------------------------------------------
// 07_CamaraOrbital.cpp
// Cámara automática girando alrededor de 10 cubos
// -----------------------------------------------------------------------------

#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_CPP_MODE
#define HANDMADE_MATH_USE_DEGREES
#include "libs/HandmadeMath.h"

#define SOKOL_IMPL
#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "07_CamaraOrbital.glsl.h"

// Estado de la aplicación
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    sg_image img1, img2;
    sg_sampler smp;
    sg_view view1, view2;
    float acc_time;
} state;

// Struct de Uniforms para asignación directa
struct {
    HMM_Mat4 mvp;
} vs_params;

// Posiciones de los 10 cubos
static HMM_Vec3 cubePositions[] = {
    HMM_V3(0.0f, 0.0f, 0.0f), HMM_V3(2.0f, 5.0f, -15.0f),
    HMM_V3(-1.5f, -2.2f, -2.5f), HMM_V3(-3.8f, -2.0f, -12.3f),
    HMM_V3(2.4f, -0.4f, -3.5f), HMM_V3(-1.7f, 3.0f, -7.5f),
    HMM_V3(1.3f, -2.0f, -2.5f), HMM_V3(1.5f, 2.0f, -2.5f),
    HMM_V3(1.5f, 0.2f, -1.5f), HMM_V3(-1.3f, 1.0f, -1.5f)
};

// Función de carga de textura idéntica a tus ejemplos funcionales
sg_image load_texture(const char* path)
{
    int w, h, c;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &c, 4);
    sg_image img = { 0 };
    if (data) {
        sg_image_desc desc = {};
        desc.width = w;
        desc.height = h;
        desc.pixel_format = SG_PIXELFORMAT_RGBA8;
        desc.data.mip_levels[0] = (sg_range) { data, (size_t)(w * h * 4) };
        img = sg_make_image(&desc);
        stbi_image_free(data);
    }
    return img;
}

void init(void)
{
    sg_desc desc = {}; // Limpiamos la estructura
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    // 1. Vértices del Cubo
    float vertices[] = {
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f, -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 1.0f, -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, -0.5f, 0.5f, -0.5f, 0.0f, 1.0f
    };
    sg_buffer_desc vbuf = { .data = SG_RANGE(vertices) };
    state.bind.vertex_buffers[0] = sg_make_buffer(&vbuf);

    // 2. Recursos de Texturas
    state.img1 = load_texture("texturas/container.jpg");
    state.img2 = load_texture("texturas/textura.png");

    sg_sampler_desc smp_d = { .min_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_LINEAR };
    state.smp = sg_make_sampler(&smp_d);

    sg_view_desc v1 = {};
    v1.texture.image = state.img1;
    state.view1 = sg_make_view(&v1);

    sg_view_desc v2 = {};
    v2.texture.image = state.img2;
    state.view2 = sg_make_view(&v2);

    state.bind.views[0] = state.view1;
    state.bind.views[1] = state.view2;
    state.bind.samplers[0] = state.smp;
    state.bind.samplers[1] = state.smp;

    // 3. Pipeline
    sg_pipeline_desc pip = {};
    pip.shader = sg_make_shader(camara_orbital_shader_desc(sg_query_backend()));
    pip.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3; // pos
    pip.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2; // uv
    pip.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    pip.depth.write_enabled = true;
    state.pip = sg_make_pipeline(&pip);

    // CORRECCIÓN DE PASS ACTION
    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.2f, 0.3f, 0.3f, 1.0f };
    state.pass_action.depth.load_action = SG_LOADACTION_CLEAR;
    state.pass_action.depth.clear_value = 1.0f;
}

void frame(void)
{
    state.acc_time += (float)sapp_frame_duration();

    float radius = 10.0f;
    float camX = sinf(state.acc_time) * radius;
    float camZ = cosf(state.acc_time) * radius;

    // CORRECCIÓN: HMM_LookAt_RH (Mano Derecha)
    HMM_Mat4 view = HMM_LookAt_RH(HMM_V3(camX, 0.0f, camZ), HMM_V3(0.0f, 0.0f, 0.0f), HMM_V3(0.0f, 1.0f, 0.0f));
    HMM_Mat4 proj = HMM_Perspective_RH_NO(45.0f, (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);

    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    for (int i = 0; i < 10; i++) {
        HMM_Mat4 model = HMM_Translate(cubePositions[i]);
        float angle = 20.0f * i;
        model = model * HMM_Rotate_RH(angle, HMM_V3(1.0f, 0.3f, 0.5f));

        vs_params.mvp = proj * view * model;

        sg_range uniforms = SG_RANGE(vs_params);
        sg_apply_uniforms(0, &uniforms);
        sg_draw(0, 36, 1);
    }

    sg_end_pass();
    sg_commit();
}

void cleanup(void) { sg_shutdown(); }

sapp_desc sokol_main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    sapp_desc adesc = {};
    adesc.init_cb = init;
    adesc.frame_cb = frame;
    adesc.cleanup_cb = cleanup;
    adesc.width = 800;
    adesc.height = 600;
    adesc.window_title = "07 - Camara Orbital";
    adesc.logger.func = slog_func;
    return adesc;
}