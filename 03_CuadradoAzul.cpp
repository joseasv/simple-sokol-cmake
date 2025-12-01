
#define SOKOL_IMPL
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"

// Incluimos el shader generado
#include "03_CuadradoAzul.glsl.h"

static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    int escenaId;
} state;

static void init(void)
{
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    // --- 1. Definir los 4 Vértices Únicos ---
    // Posiciones (XY) y Color (RGBA - Todo Azul)
    float vertices[] = {
        // Posición (X, Y)    // Color (R, G, B, A)
        -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, // 0. Arriba-Izquierda
        0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, // 1. Arriba-Derecha
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, // 2. Abajo-Derecha
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f // 3. Abajo-Izquierda
    };

    // Crear el Vertex Buffer
    sg_buffer_desc vbuf_desc = {};
    vbuf_desc.data = SG_RANGE(vertices);
    vbuf_desc.label = "quad-vertices";
    state.bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

    // --- 2. Definir los Índices (Conectando los puntos) ---
    // Un cuadrado son 2 triángulos.
    // Triángulo 1: 0 -> 1 -> 2
    // Triángulo 2: 0 -> 2 -> 3
    uint16_t indices[] = {
        0, 1, 2, // Primer triángulo
        0, 2, 3 // Segundo triángulo
    };

    // Crear el Index Buffer
    // NOTA: El tipo es SG_BUFFERTYPE_INDEXBUFFER
    sg_buffer_desc ibuf_desc = {};
    ibuf_desc.data = SG_RANGE(indices);
    ibuf_desc.label = "quad-indices";
    ibuf_desc.usage.index_buffer = true;
    state.bind.index_buffer = sg_make_buffer(&ibuf_desc);

    // --- 3. Pipeline ---
    sg_pipeline_desc pip_desc = {};
    pip_desc.shader = sg_make_shader(cuadrado_shader_desc(sg_query_backend()));

    // Layout
    pip_desc.layout.attrs[ATTR_cuadrado_position].format = SG_VERTEXFORMAT_FLOAT2;
    pip_desc.layout.attrs[ATTR_cuadrado_color0].format = SG_VERTEXFORMAT_FLOAT4;

    // IMPORTANTE: Decirle al pipeline qué tipo de índices usamos
    // Como nuestro array es uint16_t, usamos SG_INDEXTYPE_UINT16
    pip_desc.index_type = SG_INDEXTYPE_UINT16;

    pip_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    pip_desc.label = "quad-pipeline";

    state.pip = sg_make_pipeline(&pip_desc);

    // Pass Action (Fondo gris oscuro para resaltar el azul)
    state.pass_action = {};
    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.1f, 0.1f, 0.1f, 1.0f };
}

void frame(void)
{
    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();

    sg_begin_pass(&pass);
    sg_apply_pipeline(state.pip);

    // Aquí se bindean AMBOS buffers (Vertex e Index) porque los guardamos en state.bind
    sg_apply_bindings(&state.bind);

    // DIBUJAR
    // Argumentos: (base_element, number_of_elements, number_of_instances)
    // Dibujamos 6 elementos (los 6 índices), no 4 vértices.
    sg_draw(0, 6, 1);

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
    desc.width = 800;
    desc.height = 600;
    desc.window_title = "03 - Cuadrado Azul (Index Buffer)";
    desc.icon.sokol_default = true;
    desc.logger.func = slog_func;
    return desc;
}