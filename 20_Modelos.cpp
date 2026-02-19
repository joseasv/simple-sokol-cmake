// -----------------------------------------------------------------------------
// 20_Modelos.cpp
// -----------------------------------------------------------------------------

// 1. Matemáticas
#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_CPP_MODE
#define HANDMADE_MATH_USE_DEGREES
#include "libs/HandmadeMath.h"

// 2. Clases y Shaders PRIMERO
// (Como no hay "SOKOL_IMPL" aquí arriba, solo leen las declaraciones)

#include "Model.h"

// 3. Implementación de STB (Debe estar solo una vez)
#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"

// 4. Implementación de SOKOL AL FINAL DE LOS INCLUDES (Debe estar solo una vez)
#define SOKOL_IMPL
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"

// Shader Generado
#include "20_Modelos.glsl.h"

// --- VARIABLES GLOBALES ---
static struct {
    sg_pipeline pip;
    sg_sampler smp; // Sampler global

    // Puntero al modelo (Usamos puntero para control manual de creación/destrucción)
    Model* myModel = nullptr;

    // Cámara
    float cam_dist = 3.0f;
    float cam_yaw = -90.0f;
    float cam_pitch = 0.0f;
    bool dragging = false;
    float last_mouse_x, last_mouse_y;
} state;

// Uniforms
struct {
    HMM_Mat4 mvp;
    HMM_Mat4 model;
} vs_params;

struct {
    HMM_Vec3 viewPos;
    float _pad1;
    HMM_Vec3 lightPos;
    float _pad2;
    HMM_Vec3 lightColor;
    float _pad3;
} fs_params;

// --- INIT ---
void init(void)
{
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    // 1. Crear Sampler Global (Para las texturas del modelo)
    sg_sampler_desc smp_desc = {};
    smp_desc.min_filter = SG_FILTER_LINEAR;
    smp_desc.mag_filter = SG_FILTER_LINEAR;
    smp_desc.wrap_u = SG_WRAP_REPEAT;
    smp_desc.wrap_v = SG_WRAP_REPEAT;
    state.smp = sg_make_sampler(&smp_desc);

    // NOTA: Debes pasar este sampler a tu clase Mesh para que lo asigne a los bindings.
    // (O modificar Mesh::setupMesh para que acepte un sampler externo o cree uno).
    // Para este ejemplo, asumiremos que Mesh usa este sampler o uno por defecto.

    // 2. Configurar Pipeline
    sg_pipeline_desc pip = {};
    pip.shader = sg_make_shader(lighting_shader_desc(sg_query_backend()));

    // Layout debe coincidir con struct Vertex
    pip.layout.attrs[ATTR_lighting_aPos].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[ATTR_lighting_aNormal].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[ATTR_lighting_aTexCoords].format = SG_VERTEXFORMAT_FLOAT2;

    pip.depth.write_enabled = true;
    pip.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    pip.cull_mode = SG_CULLMODE_BACK;
    state.pip = sg_make_pipeline(&pip);

    // 3. CARGAR MODELO (Esto tarda un poco, en un motor real se hace en otro hilo)
    // Asegúrate de tener el archivo en la ruta correcta
    state.myModel = new Model("objects/backpack/backpack.obj");
}

// --- INPUT (Cámara Orbital) ---
void input(const sapp_event* ev)
{
    if (ev->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
        state.cam_dist -= ev->scroll_y * 0.5f;
        if (state.cam_dist < 1.0f)
            state.cam_dist = 1.0f;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_DOWN && ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        state.dragging = true;
        state.last_mouse_x = ev->mouse_x;
        state.last_mouse_y = ev->mouse_y;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_UP)
        state.dragging = false;
    else if (ev->type == SAPP_EVENTTYPE_MOUSE_MOVE && state.dragging) {
        state.cam_yaw -= (ev->mouse_x - state.last_mouse_x) * 0.5f;
        state.cam_pitch -= (ev->mouse_y - state.last_mouse_y) * 0.5f;
        state.last_mouse_x = ev->mouse_x;
        state.last_mouse_y = ev->mouse_y;
    }
}

// --- FRAME ---
void frame(void)
{
    // 1. Configurar Matrices
    float r_yaw = HMM_ToRad(state.cam_yaw);
    float r_pitch = HMM_ToRad(state.cam_pitch);
    HMM_Vec3 camPos = {
        state.cam_dist * cosf(r_pitch) * sinf(r_yaw),
        state.cam_dist * sinf(r_pitch),
        state.cam_dist * cosf(r_pitch) * cosf(r_yaw)
    };
    HMM_Mat4 view = HMM_LookAt_RH(camPos, HMM_V3(0.0f, 0.0f, 0.0f), HMM_V3(0.0f, 1.0f, 0.0f));
    HMM_Mat4 proj = HMM_Perspective_RH_NO(60.0f, (float)sapp_width() / sapp_height(), 0.1f, 100.0f);

    // 2. Configurar Modelo (Rotando en el centro)
    float time = (float)sapp_frame_count() * 0.01f;
    HMM_Mat4 model = HMM_Rotate_RH(time * 20.0f, HMM_V3(0.0f, 1.0f, 0.0f));

    sg_pass_action pass_action = {};
    pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    pass_action.colors[0].clear_value = { 0.1f, 0.1f, 0.1f, 1.0f };

    sg_begin_pass({ .action = pass_action, .swapchain = sglue_swapchain() });

    // A. Aplicar Pipeline
    sg_apply_pipeline(state.pip);

    // B. Configurar Uniforms Vertex
    vs_params.model = model;
    vs_params.mvp = proj * view * model;
    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));

    // C. Configurar Uniforms Fragment
    fs_params.viewPos = camPos;
    fs_params.lightPos = HMM_V3(2.0f, 2.0f, 2.0f);
    fs_params.lightColor = HMM_V3(1.0f, 1.0f, 1.0f);
    sg_apply_uniforms(UB_fs_params, SG_RANGE(fs_params));

    // D. Dibujar Modelo
    // Aquí es donde ocurre la magia: El modelo itera sus meshes y aplica sus texturas
    // PERO nosotros ya aplicamos el pipeline y los uniforms globales arriba.
    if (state.myModel) {
        state.myModel->Draw(state.pip);
    }

    sg_end_pass();
    sg_commit();
}

void cleanup(void)
{
    if (state.myModel)
        delete state.myModel;
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
    desc.event_cb = input;
    desc.width = 800;
    desc.height = 600;
    desc.window_title = "20 - Carga de Modelos (Assimp)";
    desc.logger.func = slog_func;
    return desc;
}