// 01_TrianguloAzul.cpp
#define SOKOL_IMPL
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"

// --- Shaders (GLSL 300 ES) ---
const char* vs_src = "#version 300 es\n"
                     "layout(location=0) in vec4 position;\n"
                     "layout(location=1) in vec4 color0;\n"
                     "out vec4 color;\n"
                     "void main() {\n"
                     "  gl_Position = position;\n"
                     "  color = color0;\n"
                     "}\n";

const char* fs_src = "#version 300 es\n"
                     "precision mediump float;\n"
                     "in vec4 color;\n"
                     "out vec4 frag_color;\n"
                     "void main() {\n"
                     "  frag_color = color;\n"
                     "}\n";

// --- Estado ---
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
} state;

// --- Implementación ---

static void init(void)
{
    // Configuración básica
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    // Datos de Vértices
    float vertices[] = {
        // x, y, z,      r, g, b, a
        0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f
    };

    // 1. Crear Buffer
    sg_buffer_desc vbuf_desc = {};
    vbuf_desc.data = SG_RANGE(vertices);
    vbuf_desc.label = "triangle-vertices";
    state.bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

    // 2. Crear Shader
    sg_shader_desc shd_desc = {};

    shd_desc.vertex_func.source = vs_src;
    shd_desc.fragment_func.source = fs_src;

    // Atributos
    shd_desc.attrs[0].glsl_name = "position";
    shd_desc.attrs[1].glsl_name = "color0";
    shd_desc.label = "triangle-shader";

    sg_shader shd = sg_make_shader(&shd_desc);

    // 3. Crear Pipeline
    sg_pipeline_desc pip_desc = {};
    pip_desc.shader = shd;
    pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3; // x,y,z
    pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4; // r,g,b,a
    pip_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    pip_desc.label = "triangle-pipeline";

    state.pip = sg_make_pipeline(&pip_desc);

    // 4. Pass Action
    state.pass_action = {};
    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.0f, 0.0f, 0.0f, 1.0f };
}

void frame(void)
{
    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();

    sg_begin_pass(&pass);
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_draw(0, 3, 1);
    sg_end_pass();
    sg_commit();
}

void cleanup(void)
{
    sg_shutdown();
}

void event(const sapp_event* e)
{
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_ESCAPE) {
            sapp_request_quit();
        }
    }
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
    desc.width = 640;
    desc.height = 480;
    desc.window_title = "Triangulo Sokol 2025";
    desc.icon.sokol_default = true;
    desc.logger.func = slog_func;
    return desc;
}