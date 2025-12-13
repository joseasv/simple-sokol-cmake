// -----------------------------------------------------------------------------
// 08_CaminataVistaFija.cpp
// Movimiento WASD por la escena de 10 cubos
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

#include "08_CaminataVistaFija.glsl.h"

// --- ESTADO DE LA CÁMARA ---
static HMM_Vec3 cameraPos = HMM_V3(0.0f, 0.0f, 3.0f);
static HMM_Vec3 cameraFront = HMM_V3(0.0f, 0.0f, -1.0f);
static HMM_Vec3 cameraUp = HMM_V3(0.0f, 1.0f, 0.0f);

// Flags para el teclado
static bool keys[SAPP_MAX_KEYCODES] = { 0 };

static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    sg_image img1, img2;
    sg_sampler smp;
    sg_view view1, view2;
} state;

struct {
    HMM_Mat4 mvp;
} vs_params;

static HMM_Vec3 cubePositions[] = {
    HMM_V3(0.0f, 0.0f, 0.0f), HMM_V3(2.0f, 5.0f, -15.0f),
    HMM_V3(-1.5f, -2.2f, -2.5f), HMM_V3(-3.8f, -2.0f, -12.3f),
    HMM_V3(2.4f, -0.4f, -3.5f), HMM_V3(-1.7f, 3.0f, -7.5f),
    HMM_V3(1.3f, -2.0f, -2.5f), HMM_V3(1.5f, 2.0f, -2.5f),
    HMM_V3(1.5f, 0.2f, -1.5f), HMM_V3(-1.3f, 1.0f, -1.5f)
};

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
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

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

    sg_pipeline_desc pip = {};
    pip.shader = sg_make_shader(caminata_shader_desc(sg_query_backend()));
    pip.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
    pip.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    pip.depth.write_enabled = true;
    state.pip = sg_make_pipeline(&pip);

    state.pass_action.colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.2f, 0.3f, 0.3f, 1.0f } };
    state.pass_action.depth = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 1.0f };
}

void event(const sapp_event* ev)
{
    if (ev->type == SAPP_EVENTTYPE_KEY_DOWN) {
        keys[ev->key_code] = true;
    } else if (ev->type == SAPP_EVENTTYPE_KEY_UP) {
        keys[ev->key_code] = false;
    }
}

void frame(void)
{
    // --- MANEJO DE MOVIMIENTO ---
    float dt = (float)sapp_frame_duration();
    float cameraSpeed = 2.5f * dt;

    if (keys[SAPP_KEYCODE_W])
        cameraPos = cameraPos + (cameraFront * cameraSpeed);
    if (keys[SAPP_KEYCODE_S])
        cameraPos = cameraPos - (cameraFront * cameraSpeed);
    if (keys[SAPP_KEYCODE_A]) {
        HMM_Vec3 right = HMM_NormV3(HMM_Cross(cameraFront, cameraUp));
        cameraPos = cameraPos - (right * cameraSpeed);
    }
    if (keys[SAPP_KEYCODE_D]) {
        HMM_Vec3 right = HMM_NormV3(HMM_Cross(cameraFront, cameraUp));
        cameraPos = cameraPos + (right * cameraSpeed);
    }

    // --- MATRICES ---
    // Target = Posición + Dirección (siempre miramos hacia donde apunta cameraFront)
    HMM_Mat4 view = HMM_LookAt_RH(cameraPos, cameraPos + cameraFront, cameraUp);
    HMM_Mat4 proj = HMM_Perspective_RH_NO(45.0f, (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    sg_pass pass = { .action = state.pass_action, .swapchain = sglue_swapchain() };
    sg_begin_pass(&pass);
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    for (int i = 0; i < 10; i++) {
        HMM_Mat4 model = HMM_Translate(cubePositions[i]);
        model = model * HMM_Rotate_RH(20.0f * i, HMM_V3(1.0f, 0.3f, 0.5f));

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
    adesc.event_cb = event; // <--- Callback de eventos activado
    adesc.cleanup_cb = cleanup;
    adesc.width = 800;
    adesc.height = 600;
    adesc.window_title = "08 - Caminata (WASD)";
    adesc.logger.func = slog_func;
    return adesc;
}