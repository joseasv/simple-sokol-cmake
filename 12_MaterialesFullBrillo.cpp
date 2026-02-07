// -----------------------------------------------------------------------------
// 12_MaterialesFullBrillo.cpp
// Uso de Materiales (Ambient, Diffuse, Specular) con luz blanca simple
// -----------------------------------------------------------------------------

#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_CPP_MODE
#define HANDMADE_MATH_USE_DEGREES
#include "libs/HandmadeMath.h"

#define SOKOL_IMPL
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"
#include <math.h>

#include "12_MaterialesFullBrillo.glsl.h" // ¡RECUERDA COMPILAR EL SHADER!

// --- STRUCTS ---
struct {
    HMM_Mat4 mvp;
    HMM_Mat4 model;
} vs_params;

struct {
    HMM_Vec3 lightPos;
    float _pad1;
    HMM_Vec3 viewPos;
    float _pad2;
    HMM_Vec3 lightColor;
    float _pad3;

    // MATERIAL
    HMM_Vec3 mat_ambient;
    float _pad4;
    HMM_Vec3 mat_diffuse;
    float _pad5;
    HMM_Vec3 mat_specular;
    float mat_shininess; // Shininess calza justo al final (12+4=16)
} fs_params;

// --- ESTADO ---
static struct {
    sg_pipeline pip_object;
    sg_pipeline pip_lamp; // Opcional si dibujas la luz
    sg_bindings bind;
    sg_pass_action pass_action;
    HMM_Vec3 lightPos;

    // Cámara
    float camYaw, camPitch, camRadius;
    float lastMouseX, lastMouseY;
    bool isDragging;
} state;

// (Copiar aquí la función "event" de la cámara orbital del ejemplo anterior)
// ... [CÓDIGO DE EVENT IGUAL AL EJEMPLO 11] ...
static void event(const sapp_event* ev)
{
    if (ev->type == SAPP_EVENTTYPE_MOUSE_DOWN && ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        state.isDragging = true;
        state.lastMouseX = ev->mouse_x;
        state.lastMouseY = ev->mouse_y;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_UP && ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        state.isDragging = false;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_MOVE && state.isDragging) {
        state.camYaw -= (ev->mouse_x - state.lastMouseX) * 0.5f;
        state.camPitch -= (ev->mouse_y - state.lastMouseY) * 0.5f;
        if (state.camPitch > 89.0f)
            state.camPitch = 89.0f;
        if (state.camPitch < -89.0f)
            state.camPitch = -89.0f;
        state.lastMouseX = ev->mouse_x;
        state.lastMouseY = ev->mouse_y;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
        state.camRadius -= ev->scroll_y * 0.5f;
        if (state.camRadius < 2.0f)
            state.camRadius = 2.0f;
    }
}

static void init(void)
{
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    state.camYaw = -90.0f;
    state.camPitch = 30.0f;
    state.camRadius = 5.0f;

    // Vértices (Pos + Norm)
    float vertices[] = {
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,

        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,

        -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,

        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,

        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,

        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f
    };

    sg_buffer_desc vbuf = {};
    vbuf.data = SG_RANGE(vertices); // Asegúrate de tener los vértices completos
    state.bind.vertex_buffers[0] = sg_make_buffer(&vbuf);

    sg_pipeline_desc pip = {};
    pip.layout.attrs[ATTR_lighting_aPos].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[ATTR_lighting_aNormal].format = SG_VERTEXFORMAT_FLOAT3;
    pip.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    pip.depth.write_enabled = true;

    pip.shader = sg_make_shader(lighting_shader_desc(sg_query_backend()));
    state.pip_object = sg_make_pipeline(&pip);

    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.1f, 0.1f, 0.1f, 1.0f };

    // Pipeline Lámpara
    pip.shader = sg_make_shader(lamp_shader_desc(sg_query_backend()));
    state.pip_lamp = sg_make_pipeline(&pip);

    state.lightPos = HMM_V3(1.2f, 1.0f, 2.0f);

    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.1f, 0.1f, 0.1f, 1.0f };
    state.pass_action.depth.load_action = SG_LOADACTION_CLEAR;
    state.pass_action.depth.clear_value = 1.0f;
}

static void frame(void)
{
    float w = sapp_widthf();
    float h = sapp_heightf();

    // Cámara
    float rY = HMM_ToRad(state.camYaw);
    float rP = HMM_ToRad(state.camPitch);
    HMM_Vec3 camPos = HMM_V3(state.camRadius * cosf(rP) * cosf(rY), state.camRadius * sinf(rP), state.camRadius * cosf(rP) * sinf(rY));
    HMM_Mat4 view = HMM_LookAt_RH(camPos, HMM_V3(0.0f, 0.0f, 0.0f), HMM_V3(0.0f, 1.0f, 0.0f));
    HMM_Mat4 proj = HMM_Perspective_RH_NO(45.0f, w / h, 0.1f, 100.0f);

    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);

    sg_apply_pipeline(state.pip_object);
    sg_apply_bindings(&state.bind);

    // --- CONFIGURACIÓN DEL MATERIAL (ESMERALDA) ---
    // Valores sacados de tablas de materiales OpenGL
    fs_params.mat_ambient = HMM_V3(0.0215f, 0.1745f, 0.0215f);
    fs_params.mat_diffuse = HMM_V3(0.07568f, 0.61424f, 0.07568f);
    fs_params.mat_specular = HMM_V3(0.633f, 0.727811f, 0.633f);
    fs_params.mat_shininess = 76.8f; // 0.6 * 128

    fs_params.lightColor = HMM_V3(1.0f, 1.0f, 1.0f);
    fs_params.lightPos = HMM_V3(1.0f, 1.0f, 2.0f);
    fs_params.viewPos = camPos;

    sg_apply_uniforms(UB_fs_params, SG_RANGE(fs_params));

    HMM_Mat4 model = HMM_Rotate_RH(30.0f, HMM_V3(0.0f, 1.0f, 0.0f));
    vs_params.model = model;
    vs_params.mvp = proj * view * model;
    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));

    sg_draw(0, 36, 1);

    sg_apply_pipeline(state.pip_lamp);
    sg_apply_bindings(&state.bind);

    HMM_Mat4 model_lamp = HMM_Translate(state.lightPos);
    model_lamp = model_lamp * HMM_Scale(HMM_V3(0.2f, 0.2f, 0.2f));

    vs_params.model = model_lamp;
    vs_params.mvp = proj * view * model_lamp;
    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));
    sg_draw(0, 36, 1);

    sg_end_pass();
    sg_commit();
}

static void cleanup(void) { sg_shutdown(); }

sapp_desc sokol_main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    sapp_desc desc = {};
    desc.init_cb = init;
    desc.frame_cb = frame;
    desc.cleanup_cb = cleanup;
    desc.event_cb = event;
    desc.width = 800;
    desc.height = 600;
    desc.window_title = "12 - Materiales Full";
    desc.logger.func = slog_func;
    return desc;
}