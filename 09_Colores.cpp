// -----------------------------------------------------------------------------
// 09_Colores.cpp
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

#include "09_Colores.glsl.h"

// --- ESTRUCTURAS DE DATOS LOCALES ---
// Definimos estas estructuras manualmente para poder usar tipos de HMM
// y evitar el uso de memcpy o arrays crudos.

// Coincide con: uniform vs_params { mat4 mvp; }
struct {
    HMM_Mat4 mvp;
} vs_params;

// Coincide con: uniform fs_params { vec3 objectColor; vec3 lightColor; }
// IMPORTANTE: En bloques uniform (std140), un vec3 base se alinea a 16 bytes.
// HMM_Vec3 son 12 bytes, así que añadimos 4 bytes de relleno (padding)
// para que coincida exactamente con lo que espera la GPU.
struct {
    HMM_Vec3 objectColor;
    float _pad1; // 12 bytes + 4 bytes = 16 bytes
    HMM_Vec3 lightColor;
    float _pad2; // 12 bytes + 4 bytes = 16 bytes
} fs_params;

// --- ESTADO GLOBAL ---
static struct {
    sg_pipeline pip_object;
    sg_pipeline pip_lamp;
    sg_bindings bind;
    sg_pass_action pass_action;
    HMM_Vec3 lightPos;
} state;

static void init(void)
{
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    // Vértices del cubo
    float vertices[] = {
        -0.5f,
        -0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        -0.5f,
        0.5f,
        0.5f,
        -0.5f,
        0.5f,
        0.5f,
        -0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        -0.5f,
        -0.5f,
        -0.5f,
        -0.5f,
        -0.5f,
        0.5f,
        0.5f,
        -0.5f,
        0.5f,
        0.5f,
        0.5f,
        0.5f,
        0.5f,
        0.5f,
        0.5f,
        -0.5f,
        0.5f,
        0.5f,
        -0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        0.5f,
        0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        -0.5f,
        -0.5f,
        -0.5f,
        -0.5f,
        -0.5f,
        -0.5f,
        -0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        0.5f,
        0.5f,
        0.5f,
        0.5f,
        0.5f,
        0.5f,
        0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        0.5f,
        0.5f,
        0.5f,
        0.5f,
        -0.5f,
        -0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        0.5f,
        0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        -0.5f,
        -0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        0.5f,
        0.5f,
        -0.5f,
        0.5f,
        0.5f,
        0.5f,
        0.5f,
        0.5f,
        0.5f,
        -0.5f,
        0.5f,
        0.5f,
        -0.5f,
        0.5f,
        -0.5f,
    };

    sg_buffer_desc vbuf = {};
    vbuf.data = SG_RANGE(vertices);
    state.bind.vertex_buffers[0] = sg_make_buffer(&vbuf);

    // Configuración común del Pipeline
    sg_pipeline_desc pip = {};
    pip.layout.attrs[ATTR_lamp_position].format = SG_VERTEXFORMAT_FLOAT3;
    pip.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    pip.depth.write_enabled = true;

    // Pipeline Objeto
    pip.shader = sg_make_shader(lighting_shader_desc(sg_query_backend()));
    pip.label = "object-pipeline";
    state.pip_object = sg_make_pipeline(&pip);

    // Pipeline Lámpara
    pip.shader = sg_make_shader(lamp_shader_desc(sg_query_backend()));
    pip.label = "lamp-pipeline";
    state.pip_lamp = sg_make_pipeline(&pip);

    state.lightPos = HMM_V3(1.2f, 1.0f, 2.0f);

    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.1f, 0.1f, 0.1f, 1.0f };
}

static void frame(void)
{
    float w = (float)sapp_width();
    float h = (float)sapp_height();
    float aspect = w / h;

    HMM_Mat4 proj;
    if (sg_query_backend() == SG_BACKEND_GLCORE) {
        proj = HMM_Perspective_RH_NO(45.0f, aspect, 0.1f, 100.0f);
    } else {
        proj = HMM_Perspective_RH_ZO(45.0f, aspect, 0.1f, 100.0f);
    }

    HMM_Mat4 view = HMM_Translate(HMM_V3(0.0f, 0.0f, -5.0f));

    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);

    // ---------------------
    // 1. DIBUJAR OBJETO
    // ---------------------
    sg_apply_pipeline(state.pip_object);
    sg_apply_bindings(&state.bind);

    // Uniforms Fragmento (Colores)
    // Asignación directa gracias a nuestra estructura manual
    fs_params.objectColor = HMM_V3(1.0f, 0.5f, 0.31f);
    fs_params.lightColor = HMM_V3(1.0f, 1.0f, 1.0f);

    sg_apply_uniforms(UB_fs_params, SG_RANGE(fs_params));

    // Uniforms Vértice (MVP)
    HMM_Mat4 model = HMM_Rotate_RH(30.0f, HMM_V3(0.0f, 1.0f, 0.0f));
    vs_params.mvp = proj * view * model;

    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));

    sg_draw(0, 36, 1);

    // ---------------------
    // 2. DIBUJAR LÁMPARA
    // ---------------------
    sg_apply_pipeline(state.pip_lamp);
    sg_apply_bindings(&state.bind);

    HMM_Mat4 model_lamp = HMM_Translate(state.lightPos);
    model_lamp = model_lamp * HMM_Scale(HMM_V3(0.2f, 0.2f, 0.2f));

    vs_params.mvp = proj * view * model_lamp;
    // Reutilizamos UB_vs_params porque el bloque VS es el mismo
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
    desc.width = 800;
    desc.height = 600;
    desc.window_title = "09 - Iluminacion: Colores";
    desc.logger.func = slog_func;
    return desc;
}