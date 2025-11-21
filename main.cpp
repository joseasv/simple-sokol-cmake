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

const char* backend_name(sg_backend b)
{
    switch (b) {
    case SG_BACKEND_GLCORE:
        return "OpenGL Core (Linux/Windows)";
    case SG_BACKEND_GLES3:
        return "OpenGLES 3 (Web/Android)";
    case SG_BACKEND_D3D11:
        return "Direct3D 11 (Windows)";
    case SG_BACKEND_METAL_IOS:
        return "Metal (macOS/iOS)";
    case SG_BACKEND_WGPU:
        return "WebGPU";
    case SG_BACKEND_DUMMY:
        return "Dummy (No Graphics)";
    default:
        return "Unknown Backend";
    }
}

// --- Inicialización ---
void init(void)
{
    // Configuración de Sokol GFX
    sg_desc desc = {};

    // En la API nueva, usamos 'environment' y 'sglue_environment()'
    desc.environment = sglue_environment();

    sg_setup(&desc);

    sg_backend backend = sg_query_backend();
    printf("------------------------------------------------\n");
    printf("SOKOL INICIADO EXITOSAMENTE\n");
    printf("Backend Gráfico: %s\n", backend_name(backend));
    printf("------------------------------------------------\n");

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
    (void)argc;
    (void)argv;
    sapp_desc desc = {};
    desc.init_cb = init;
    desc.frame_cb = frame;
    desc.cleanup_cb = cleanup;
    desc.event_cb = event;
    desc.width = 640;
    desc.height = 480;
    desc.window_title = "Hola Mundo Sokol 2025";
    desc.icon.sokol_default = true;
    return desc;
}