// -----------------------------------------------------------------------------
// 11_IluminacionEspecular.cpp
// Modelo Phong Completo (Ambient + Diffuse + Specular) + Cámara Orbital
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

#include <math.h> // Necesario para sin/cos

// Asegúrate de tener el header generado correctamente
#include "11_IluminacionEspecular.glsl.h"

// --- STRUCTS (Deben coincidir con el GLSL) ---
struct {
    HMM_Mat4 mvp;
    HMM_Mat4 model;
} vs_params;

struct {
    HMM_Vec3 objectColor;
    float _pad1;
    HMM_Vec3 lightColor;
    float _pad2;
    HMM_Vec3 lightPos;
    float _pad3;
    HMM_Vec3 viewPos;
    float _pad4; // ¡Necesario para el brillo especular!
} fs_params;

// --- ESTADO GLOBAL ---
static struct {
    sg_pipeline pip_object;
    sg_pipeline pip_lamp;
    sg_bindings bind;
    sg_pass_action pass_action;
    HMM_Vec3 lightPos;

    // Variables de la Cámara Orbital
    float camYaw;
    float camPitch;
    float camRadius;
    float lastMouseX;
    float lastMouseY;
    bool isDragging;
} state;

// --- CONTROL DEL MOUSE ---
static void event(const sapp_event* ev)
{
    if (ev->type == SAPP_EVENTTYPE_MOUSE_DOWN) {
        if (ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
            state.isDragging = true;
            state.lastMouseX = ev->mouse_x;
            state.lastMouseY = ev->mouse_y;
        }
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_UP) {
        if (ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
            state.isDragging = false;
        }
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_MOVE) {
        if (state.isDragging) {
            float dx = ev->mouse_x - state.lastMouseX;
            float dy = ev->mouse_y - state.lastMouseY;
            state.lastMouseX = ev->mouse_x;
            state.lastMouseY = ev->mouse_y;

            state.camYaw -= dx * 0.5f;
            state.camPitch -= dy * 0.5f;

            if (state.camPitch > 89.0f)
                state.camPitch = 89.0f;
            if (state.camPitch < -89.0f)
                state.camPitch = -89.0f;
        }
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
        state.camRadius -= ev->scroll_y * 0.5f;
        if (state.camRadius < 2.0f)
            state.camRadius = 2.0f;
        if (state.camRadius > 20.0f)
            state.camRadius = 20.0f;
    }
}

static void init(void)
{
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    // Inicializar Cámara
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
    vbuf.data = SG_RANGE(vertices);
    state.bind.vertex_buffers[0] = sg_make_buffer(&vbuf);

    // Pipeline Objeto
    sg_pipeline_desc pip = {};
    pip.layout.attrs[ATTR_lighting_aPos].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[ATTR_lighting_aNormal].format = SG_VERTEXFORMAT_FLOAT3;
    pip.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    pip.depth.write_enabled = true;

    pip.shader = sg_make_shader(lighting_shader_desc(sg_query_backend()));
    state.pip_object = sg_make_pipeline(&pip);

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
    float w = (float)sapp_width();
    float h = (float)sapp_height();
    float time = (float)sapp_frame_count() * 0.01f;

    // --- CÁLCULO DE CÁMARA ORBITAL ---
    float radYaw = HMM_ToRad(state.camYaw);
    float radPitch = HMM_ToRad(state.camPitch);

    float camX = state.camRadius * cosf(radPitch) * cosf(radYaw);
    float camY = state.camRadius * sinf(radPitch);
    float camZ = state.camRadius * cosf(radPitch) * sinf(radYaw);

    HMM_Vec3 camPos = HMM_V3(camX, camY, camZ);
    HMM_Mat4 view = HMM_LookAt_RH(camPos, HMM_V3(0.0f, 0.0f, 0.0f), HMM_V3(0.0f, 1.0f, 0.0f));

    HMM_Mat4 proj;
    if (sg_query_backend() == SG_BACKEND_GLCORE) {
        proj = HMM_Perspective_RH_NO(45.0f, w / h, 0.1f, 100.0f);
    } else {
        proj = HMM_Perspective_RH_ZO(45.0f, w / h, 0.1f, 100.0f);
    }

    // Mover luz
    state.lightPos.X = 1.0f + sinf(time) * 2.0f;
    state.lightPos.Y = sinf(time / 2.0f) * 1.0f;

    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);

    // --- DIBUJAR OBJETO ---
    sg_apply_pipeline(state.pip_object);
    sg_apply_bindings(&state.bind);

    fs_params.objectColor = HMM_V3(1.0f, 0.5f, 0.31f);
    fs_params.lightColor = HMM_V3(1.0f, 1.0f, 1.0f);
    fs_params.lightPos = state.lightPos;
    fs_params.viewPos = camPos; // <--- ¡CRUCIAL PARA EL BRILLO ESPECULAR!

    sg_apply_uniforms(UB_fs_params, SG_RANGE(fs_params));

    HMM_Mat4 model = HMM_Rotate_RH(30.0f, HMM_V3(0.0f, 1.0f, 0.0f));
    vs_params.model = model;
    vs_params.mvp = proj * view * model;
    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));
    sg_draw(0, 36, 1);

    // --- DIBUJAR LÁMPARA ---
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

static void cleanup(void)
{
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    sapp_desc desc = {};
    desc.init_cb = init;
    desc.frame_cb = frame;
    desc.event_cb = event; // <--- ¡AQUÍ ESTABA EL PROBLEMA!
    desc.cleanup_cb = cleanup;
    desc.width = 800;
    desc.height = 600;
    desc.window_title = "11 - Iluminacion Especular + Camara";
    desc.logger.func = slog_func;
    return desc;
}