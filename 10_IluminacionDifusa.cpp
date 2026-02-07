// -----------------------------------------------------------------------------
// 10_IluminacionDifusa.cpp
// Implementación: Iluminación Difusa + Cámara Orbital con Mouse
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

#include <math.h> // Necesario para sin/cos de la cámara

#include "10_IluminacionDifusa.glsl.h"

// --- STRUCTS DE UNIFORMS (Coinciden con el shader) ---
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
} fs_params;

// --- ESTADO GLOBAL ---
static struct {
    sg_pipeline pip_object;
    sg_pipeline pip_lamp;
    sg_bindings bind;
    sg_pass_action pass_action;
    HMM_Vec3 lightPos;

    // <--- VARIABLES DE LA CÁMARA ORBITAL
    float camYaw; // Rotación horizontal (grados)
    float camPitch; // Rotación vertical (grados)
    float camRadius; // Distancia al centro
    float lastMouseX;
    float lastMouseY;
    bool isDragging; // ¿Está el botón presionado?
} state;

// --- CALLBACK DE EVENTOS (MOUSE) ---
static void event(const sapp_event* ev)
{
    // Detectar clic izquierdo para iniciar arrastre
    if (ev->type == SAPP_EVENTTYPE_MOUSE_DOWN) {
        if (ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
            state.isDragging = true;
            state.lastMouseX = ev->mouse_x;
            state.lastMouseY = ev->mouse_y;
        }
    }
    // Soltar clic
    else if (ev->type == SAPP_EVENTTYPE_MOUSE_UP) {
        if (ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
            state.isDragging = false;
        }
    }
    // Mover mouse
    else if (ev->type == SAPP_EVENTTYPE_MOUSE_MOVE) {
        if (state.isDragging) {
            float dx = ev->mouse_x - state.lastMouseX;
            float dy = ev->mouse_y - state.lastMouseY;

            state.lastMouseX = ev->mouse_x;
            state.lastMouseY = ev->mouse_y;

            // Sensibilidad
            float sensitivity = 0.5f;

            // Actualizar ángulos
            state.camYaw -= dx * sensitivity;
            state.camPitch -= dy * sensitivity;

            // Limitar el ángulo vertical para que no de la vuelta completa (Gimbal Lock)
            if (state.camPitch > 89.0f)
                state.camPitch = 89.0f;
            if (state.camPitch < -89.0f)
                state.camPitch = -89.0f;
        }
    }
    // Zoom con la rueda del mouse (Opcional)
    else if (ev->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
        state.camRadius -= ev->scroll_y * 0.5f;
        if (state.camRadius < 1.0f)
            state.camRadius = 1.0f;
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

    // --- CONFIGURACIÓN INICIAL DE CÁMARA ---
    state.camYaw = -90.0f; // Mirando hacia -Z por defecto
    state.camPitch = 30.0f; // Un poco elevado
    state.camRadius = 5.0f; // Distancia inicial
    state.isDragging = false;

    // Vértices del Cubo
    float vertices[] = {
        // Posiciones          // Normales
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

    // Ajustamos los atributos según tu shader compilado
    sg_pipeline_desc pip = {};
    pip.layout.attrs[ATTR_lighting_aPos].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[ATTR_lighting_aNormal].format = SG_VERTEXFORMAT_FLOAT3;
    pip.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    pip.depth.write_enabled = true;

    // Pipeline Objeto
    pip.shader = sg_make_shader(lighting_shader_desc(sg_query_backend()));
    state.pip_object = sg_make_pipeline(&pip);

    // Pipeline Lámpara
    pip.shader = sg_make_shader(lamp_shader_desc(sg_query_backend()));
    state.pip_lamp = sg_make_pipeline(&pip);

    // Posición inicial de la luz
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

    // --- CÁLCULO DE POSICIÓN DE CÁMARA (Esféricas -> Cartesianas) ---
    float radYaw = HMM_ToRad(state.camYaw);
    float radPitch = HMM_ToRad(state.camPitch);

    // Fórmula para convertir coordenadas esféricas a cartesianas (Orbitar 0,0,0)
    float camX = state.camRadius * cosf(radPitch) * cosf(radYaw);
    float camY = state.camRadius * sinf(radPitch);
    float camZ = state.camRadius * cosf(radPitch) * sinf(radYaw);

    HMM_Vec3 camPos = HMM_V3(camX, camY, camZ);

    // LookAt siempre mira al origen (0,0,0)
    HMM_Mat4 view = HMM_LookAt_RH(camPos, HMM_V3(0.0f, 0.0f, 0.0f), HMM_V3(0.0f, 1.0f, 0.0f));

    HMM_Mat4 proj;
    if (sg_query_backend() == SG_BACKEND_GLCORE) {
        proj = HMM_Perspective_RH_NO(45.0f, w / h, 0.1f, 100.0f);
    } else {
        proj = HMM_Perspective_RH_ZO(45.0f, w / h, 0.1f, 100.0f);
    }

    // Movemos la luz (opcional, para ver efecto dinámico)
    state.lightPos.X = 1.0f + sinf(time) * 2.0f;
    state.lightPos.Y = sinf(time / 2.0f) * 1.0f;

    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);

    // 1. DIBUJAR OBJETO
    sg_apply_pipeline(state.pip_object);
    sg_apply_bindings(&state.bind);

    fs_params.objectColor = HMM_V3(1.0f, 0.5f, 0.31f);
    fs_params.lightColor = HMM_V3(1.0f, 1.0f, 1.0f);
    fs_params.lightPos = state.lightPos;

    // NOTA: Usamos UB_fs_params (generado por sokol-shdc)
    sg_apply_uniforms(UB_fs_params, SG_RANGE(fs_params));

    HMM_Mat4 model = HMM_Rotate_RH(30.0f, HMM_V3(0.0f, 1.0f, 0.0f));
    vs_params.model = model;
    vs_params.mvp = proj * view * model;

    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));
    sg_draw(0, 36, 1);

    // 2. DIBUJAR LÁMPARA
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
    desc.event_cb = event; // <--- ¡IMPORTANTE! Registrar el evento
    desc.cleanup_cb = cleanup;
    desc.width = 800;
    desc.height = 600;
    desc.window_title = "10 - Iluminacion Difusa + Camara Orbital";
    desc.logger.func = slog_func;
    return desc;
}