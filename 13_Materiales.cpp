// -----------------------------------------------------------------------------
// 13_Materiales.cpp
// Materiales + Propiedades de Luz (Intensidades separadas)
// Con cubo de lámpara y lógica de luz dinámica opcional.
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

#include "13_Materiales.glsl.h"

// --- STRUCTS DE UNIFORMS ---
struct {
    HMM_Mat4 mvp;
    HMM_Mat4 model;
} vs_params;

// Struct alineado a std140 (16 bytes por vec3)
struct {
    HMM_Vec3 viewPos;
    float _pad1;
    HMM_Vec3 lightPos;
    float _pad2;

    // Propiedades LUZ
    HMM_Vec3 light_ambient;
    float _pad3;
    HMM_Vec3 light_diffuse;
    float _pad4;
    HMM_Vec3 light_specular;
    float _pad5;

    // Propiedades MATERIAL
    HMM_Vec3 mat_ambient;
    float _pad6;
    HMM_Vec3 mat_diffuse;
    float _pad7;
    HMM_Vec3 mat_specular;
    float mat_shininess; // Alineado al final
} fs_params;

// --- ESTADO GLOBAL ---
static struct {
    sg_pipeline pip_object; // Pipeline para el objeto iluminado
    sg_pipeline pip_lamp; // Pipeline para el cubo de luz
    sg_bindings bind;
    sg_pass_action pass_action;

    // Variables de Cámara
    float camYaw, camPitch, camRadius;
    float lastMouseX, lastMouseY;
    bool isDragging;
} state;

// --- CALLBACK DE EVENTOS (Cámara Orbital) ---
static void event(const sapp_event* ev)
{
    if (ev->type == SAPP_EVENTTYPE_MOUSE_DOWN && ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        state.isDragging = true;
        state.lastMouseX = ev->mouse_x;
        state.lastMouseY = ev->mouse_y;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_UP && ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        state.isDragging = false;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_MOVE && state.isDragging) {
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

    // Configuración inicial cámara
    state.camYaw = -90.0f;
    state.camPitch = 30.0f;
    state.camRadius = 5.0f;

    // Vértices del Cubo (36 vértices, Pos + Normal)
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

    sg_pipeline_desc pip = {};
    pip.layout.attrs[ATTR_lighting_aPos].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[ATTR_lighting_aNormal].format = SG_VERTEXFORMAT_FLOAT3;
    pip.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    pip.depth.write_enabled = true;

    // Pipeline 1: Objeto (Usa iluminación compleja)
    pip.shader = sg_make_shader(lighting_shader_desc(sg_query_backend()));
    state.pip_object = sg_make_pipeline(&pip);

    // Pipeline 2: Lámpara (Usa shader simple blanco)
    pip.shader = sg_make_shader(lamp_shader_desc(sg_query_backend()));
    state.pip_lamp = sg_make_pipeline(&pip);

    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.1f, 0.1f, 0.1f, 1.0f };
}

static void frame(void)
{
    float w = sapp_widthf();
    float h = sapp_heightf();
    float time = (float)sapp_frame_count() * 0.016f;

    // --- CÁLCULO DE CÁMARA ORBITAL ---
    float rY = HMM_ToRad(state.camYaw);
    float rP = HMM_ToRad(state.camPitch);
    HMM_Vec3 camPos = HMM_V3(
        state.camRadius * cosf(rP) * cosf(rY),
        state.camRadius * sinf(rP),
        state.camRadius * cosf(rP) * sinf(rY));
    HMM_Mat4 view = HMM_LookAt_RH(camPos, HMM_V3(0.0f, 0.0f, 0.0f), HMM_V3(0.0f, 1.0f, 0.0f));

    sg_backend backend = sg_query_backend();
    HMM_Mat4 proj;
    if (backend == SG_BACKEND_GLCORE) {
        // Linux / Mac antiguo (OpenGL): Usa rango -1 a 1
        proj = HMM_Perspective_RH_NO(45.0f, (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);
    } else {
        // Windows (D3D11) / Mac nuevo (Metal): Usa rango 0 a 1
        proj = HMM_Perspective_RH_ZO(45.0f, (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);
    }

    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);

    // -------------------------------------------------------------------------
    // 1. CONFIGURACIÓN DE ESCENA
    // -------------------------------------------------------------------------
    HMM_Vec3 lightPos = HMM_V3(1.2f, 1.0f, 2.0f);

    // Valores por defecto (Luz blanca estática)
    HMM_Vec3 lightAmbient = HMM_V3(0.2f, 0.2f, 0.2f);
    HMM_Vec3 lightDiffuse = HMM_V3(0.5f, 0.5f, 0.5f);
    HMM_Vec3 lightSpecular = HMM_V3(1.0f, 1.0f, 1.0f);

    // -------------------------------------------------------------------------
    // BLOQUE DINÁMICO (Descomentar para ver luces cambiantes como en LearnOpenGL)
    // -------------------------------------------------------------------------

    /*HMM_Vec3 lightColor;
    lightColor.X = sinf(time * 2.0f);
    lightColor.Y = sinf(time * 0.7f);
    lightColor.Z = sinf(time * 1.3f);

    // CORRECCIÓN 1: Usamos el operador '*' directo (gracias a HANDMADE_MATH_CPP_MODE)
    // CORRECCIÓN 2: Usamos 'lightDiffuse' para el siguiente cálculo, no 'diffuseColor'

    lightDiffuse = lightColor * 0.5f; // Disminuimos intensidad
    lightAmbient = lightDiffuse * 0.2f; // Ambient derivada de la difusa


    // Asignamos al struct de parámetros
    fs_params.light_ambient = lightAmbient;
    fs_params.light_diffuse = lightDiffuse;

    // El especular lo dejamos blanco puro para que el brillo destaque
    fs_params.light_specular = HMM_V3(1.0f, 1.0f, 1.0f);*/

    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    // 2. DIBUJAR OBJETO (ESMERALDA)
    // -------------------------------------------------------------------------
    sg_apply_pipeline(state.pip_object);
    sg_apply_bindings(&state.bind);

    // Llenar Uniforms
    fs_params.viewPos = camPos;
    fs_params.lightPos = lightPos;

    // Asignar Luz
    fs_params.light_ambient = lightAmbient;
    fs_params.light_diffuse = lightDiffuse;
    fs_params.light_specular = lightSpecular;

    // Asignar Material (Ej. Esmeralda)
    fs_params.mat_ambient = HMM_V3(0.0215f, 0.1745f, 0.0215f);
    fs_params.mat_diffuse = HMM_V3(0.07568f, 0.61424f, 0.07568f);
    fs_params.mat_specular = HMM_V3(0.633f, 0.727811f, 0.633f);
    fs_params.mat_shininess = 76.8f;

    sg_apply_uniforms(UB_fs_params, SG_RANGE(fs_params));

    HMM_Mat4 model = HMM_Rotate_RH(30.0f, HMM_V3(0.0f, 1.0f, 0.0f));
    vs_params.model = model;
    vs_params.mvp = proj * view * model;
    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));

    sg_draw(0, 36, 1);

    // -------------------------------------------------------------------------
    // 3. DIBUJAR LÁMPARA (CUBO PEQUEÑO)
    // -------------------------------------------------------------------------
    sg_apply_pipeline(state.pip_lamp);
    sg_apply_bindings(&state.bind);

    HMM_Mat4 model_lamp = HMM_Translate(lightPos);
    model_lamp = model_lamp * HMM_Scale(HMM_V3(0.2f, 0.2f, 0.2f));

    vs_params.model = model_lamp; // Aunque el shader de lámpara no usa 'model' para luz, enviamos el bloque igual
    vs_params.mvp = proj * view * model_lamp;

    // El shader de lámpara usa el MISMO bloque 'vs_params' (Binding 0)
    // por lo que debemos actualizarlo con la nueva MVP de la lámpara.
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
    desc.cleanup_cb = cleanup;
    desc.event_cb = event;
    desc.width = 800;
    desc.height = 600;
    desc.window_title = "13 - Materiales + Propiedades Luz";
    desc.logger.func = slog_func;
    return desc;
}