// 1. Definir la implementación
#define SOKOL_IMPL

// 3. Incluir headers
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"

// --- Estado Singleton---
struct {
    sg_pass_action pass_action;
    float b;
} state;

float b = 0;

// --- Inicialización ---
void init(void)
{
    // Configuración de Sokol GFX
    sg_desc desc = {};

    // En la API nueva, usamos 'environment' y 'sglue_environment()'
    desc.environment = sglue_environment();

    sg_setup(&desc);

    // Configurar la acción de limpieza
    state.pass_action = {};
    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.1f, 0.1f, 0.4f, 1.0f };

    state.b = 0;
}

// --- Bucle ---
void frame(void)
{
    state.b += 0.01f;
    if (state.b > 1.0f) {
        state.b = 0.0f;
    }

    state.pass_action.colors[0].clear_value.b = state.b;

    // Crear el pase usando el swapchain (API Nueva)
    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();

    sg_begin_pass(&pass);
    sg_end_pass();
    sg_commit();
}

// --- Limpieza ---
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

// --- Main ---
sapp_desc sokol_main(int argc, char* argv[])
{
    sapp_desc desc = {};
    desc.init_cb = init;
    desc.frame_cb = frame;
    desc.cleanup_cb = cleanup;
    desc.event_cb = event;
    desc.width = 800;
    desc.height = 600;
    desc.window_title = "Hola Sokol (API 2025)";
    return desc;
}