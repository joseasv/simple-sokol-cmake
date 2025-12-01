
#define SOKOL_IMPL
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"

// Reutilizamos el mismo shader compilado del ejemplo anterior
#include "03_Cuadrado2Triangulos.glsl.h"

static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
} state;

static void init(void)
{
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    // --- DEFINICIÓN DE VÉRTICES (Fuerza Bruta) ---
    // Total: 6 vértices (2 triángulos x 3 vértices)
    // Nota cómo repetimos datos para la diagonal.
    float vertices[] = {
        // TRIÁNGULO 1 (Arriba - Derecha)
        // X, Y, Z             R, G, B, A
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, // 1. Arriba-Izquierda
        0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, // 2. Arriba-Derecha
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, // 3. Abajo-Derecha

        // TRIÁNGULO 2 (Abajo - Izquierda)
        // X, Y, Z             R, G, B, A
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, // 1. Arriba-Izquierda (REPETIDO)
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, // 3. Abajo-Derecha    (REPETIDO)
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f // 4. Abajo-Izquierda
    };

    // 1. Crear el Vertex Buffer
    sg_buffer_desc vbuf_desc = {};
    vbuf_desc.data = SG_RANGE(vertices);
    vbuf_desc.label = "quad-raw-vertices";
    state.bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

    // 2. NO CREAMOS INDEX BUFFER
    // (Esta sección desaparece)

    // 3. Pipeline
    sg_pipeline_desc pip_desc = {};
    pip_desc.shader = sg_make_shader(cuadrado_shader_desc(sg_query_backend()));

    pip_desc.layout.attrs[ATTR_cuadrado_position].format = SG_VERTEXFORMAT_FLOAT3;
    pip_desc.layout.attrs[ATTR_cuadrado_color0].format = SG_VERTEXFORMAT_FLOAT4;

    // CAMBIO IMPORTANTE:
    // Eliminamos la línea: pip_desc.index_type = SG_INDEXTYPE_UINT16;
    // Al no definirla, Sokol asume que no usaremos índices.

    pip_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
    pip_desc.label = "quad-raw-pipeline";

    state.pip = sg_make_pipeline(&pip_desc);

    state.pass_action = {};
    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    // Usamos un color de fondo un poco distinto (verde oscuro) para diferenciar
    state.pass_action.colors[0].clear_value = { 0.1f, 0.4f, 0.1f, 1.0f };
}

void frame(void)
{
    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();

    sg_begin_pass(&pass);
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    // DIBUJAR
    // Argumentos: (vértice_inicial, cantidad_de_vértices, instancias)
    // Ahora le decimos "6" porque hay 6 vértices reales en el buffer.
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
    desc.window_title = "04 - Cuadrado Sin Indices (Vertices Repetidos)";
    desc.icon.sokol_default = true;
    desc.logger.func = slog_func;
    return desc;
}