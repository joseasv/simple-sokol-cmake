// -----------------------------------------------------------------------------
// 15_MapaEspecular.cpp
// -----------------------------------------------------------------------------

#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_CPP_MODE
#define HANDMADE_MATH_USE_DEGREES
#include "libs/HandmadeMath.h"

#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"

#define SOKOL_IMPL
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"
#include <math.h>

// Shader generado (Asegúrate de haber limpiado el build para regenerarlo)
#include "15_MapaEspecular.glsl.h"

// --- STRUCTS ---
struct {
    HMM_Mat4 mvp;
    HMM_Mat4 model;
} vs_params;

struct {
    HMM_Vec3 viewPos;
    float _pad1; // 16 bytes
    HMM_Vec3 lightPos;
    float _pad2; // 16 bytes
    HMM_Vec3 light_ambient;
    float _pad3; // 16 bytes
    HMM_Vec3 light_diffuse;
    float _pad4; // 16 bytes

    // AQUÍ ESTÁ EL TRUCO:
    // En lugar de padding, ponemos la variable mat_shininess directamente
    // para que ocupe el 4to componente (W) de este vector.
    HMM_Vec3 light_specular;
    float mat_shininess; // Ocupa los 4 bytes que sobraban aquí

    // Total exacto: 5 * 16 = 80 bytes.
    // Borra cualquier otro padding o variable debajo de esto.
} fs_params;

static struct {
    sg_pipeline pip_object;
    sg_pipeline pip_lamp;
    sg_bindings bind;
    sg_pass_action pass_action;

    // RECURSOS
    sg_image img_diffuse;
    sg_image img_specular;
    sg_sampler smp;

    // VISTAS (Legacy API)
    sg_view view_diffuse;
    sg_view view_specular;

    float camYaw, camPitch, camRadius;
    float lastMouseX, lastMouseY;
    bool isDragging;
} state;

// --- EVENTOS CÁMARA ---
static void event(const sapp_event* ev)
{
    if (ev->type == SAPP_EVENTTYPE_MOUSE_DOWN && ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        state.isDragging = true;
        state.lastMouseX = ev->mouse_x;
        state.lastMouseY = ev->mouse_y;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_UP && ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        state.isDragging = false;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_MOVE && state.isDragging) {
        state.camYaw -= (ev->mouse_x - state.lastMouseX) * 0.5f;
        state.camPitch -= (ev->mouse_y - state.lastMouseY) * 0.5f;

        if (state.camPitch > 89.0f)
            state.camPitch = 89.0f;
        if (state.camPitch < -89.0f)
            state.camPitch = -89.0f;

        state.lastMouseX = ev->mouse_x;
        state.lastMouseY = ev->mouse_y;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
        state.camRadius -= ev->scroll_y * 0.5f;
        if (state.camRadius < 2.0f)
            state.camRadius = 2.0f;
    }
}

// --- CARGAR TEXTURA (FIX: Usando mip_levels) ---
static sg_image load_texture(const char* filename)
{
    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename, &w, &h, &channels, 4);

    sg_image_desc desc = {};
    desc.pixel_format = SG_PIXELFORMAT_RGBA8;

    if (data) {
        desc.width = w;
        desc.height = h;
        // CORRECCIÓN: Usamos mip_levels[0] con casting a sg_range manual
        desc.data.mip_levels[0] = (sg_range) { .ptr = data, .size = (size_t)(w * h * 4) };
    } else {
        // Fallback: pixel magenta para notar errores
        uint32_t pixel = 0xFF00FFFF;
        desc.width = 1;
        desc.height = 1;
        // CORRECCIÓN: Usamos SG_RANGE para el fallback
        desc.data.mip_levels[0] = SG_RANGE(pixel);
    }

    sg_image img = sg_make_image(&desc);

    if (data)
        stbi_image_free(data);
    return img;
}

static void init(void)
{
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    state.camYaw = -90.0f;
    state.camPitch = 0.0f;
    state.camRadius = 5.0f;

    float vertices[] = {
        // Pos                 // Norm              // UV
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

        -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,

        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
    };

    sg_buffer_desc vbuf = {};
    vbuf.data = SG_RANGE(vertices);
    state.bind.vertex_buffers[0] = sg_make_buffer(&vbuf);

    // 1. Cargar Imágenes
    state.img_diffuse = load_texture("texturas/container2.png");
    state.img_specular = load_texture("texturas/container2_specular.png");

    // 2. Crear Sampler (Compartido)
    sg_sampler_desc smp_desc = {};
    smp_desc.min_filter = SG_FILTER_LINEAR;
    smp_desc.mag_filter = SG_FILTER_LINEAR;
    state.smp = sg_make_sampler(&smp_desc);

    // 3. CREAR VISTAS (Legacy API)
    sg_view_desc view_desc = {};

    // Vista Difusa
    view_desc.texture.image = state.img_diffuse;
    state.view_diffuse = sg_make_view(&view_desc);

    // Vista Especular (Reutilizamos struct, cambiamos imagen)
    view_desc.texture.image = state.img_specular;
    state.view_specular = sg_make_view(&view_desc);

    // 4. ASIGNAR BINDINGS (Macros de shader: VIEW_... y SMP_...)
    state.bind.views[VIEW_material_diffuse].id = state.view_diffuse.id;
    state.bind.views[VIEW_material_specular].id = state.view_specular.id;
    state.bind.samplers[SMP_smp].id = state.smp.id;

    // --- PIPELINE OBJETO ---
    sg_pipeline_desc pip = {};
    pip.layout.attrs[ATTR_lighting_aPos].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[ATTR_lighting_aNormal].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[ATTR_lighting_aTexCoords].format = SG_VERTEXFORMAT_FLOAT2;

    pip.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    pip.depth.write_enabled = true;

    // CULL MODE ELIMINADO

    pip.shader = sg_make_shader(lighting_shader_desc(sg_query_backend()));
    state.pip_object = sg_make_pipeline(&pip);

    // --- PIPELINE LAMPARA ---
    pip.shader = sg_make_shader(lamp_shader_desc(sg_query_backend()));
    state.pip_lamp = sg_make_pipeline(&pip);

    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.1f, 0.1f, 0.1f, 1.0f };
}

static void frame(void)
{
    float w = sapp_widthf();
    float h = sapp_heightf();

    // FIX: HMM_ToRad
    float rY = HMM_ToRad(state.camYaw);
    float rP = HMM_ToRad(state.camPitch);

    HMM_Vec3 camPos = HMM_V3(state.camRadius * cosf(rP) * cosf(rY), state.camRadius * sinf(rP), state.camRadius * cosf(rP) * sinf(rY));
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

    // --- OBJETO ---
    sg_apply_pipeline(state.pip_object);
    sg_apply_bindings(&state.bind); // Envía ambas vistas

    fs_params.viewPos = camPos;
    fs_params.lightPos = HMM_V3(1.2f, 1.0f, 2.0f);
    fs_params.light_ambient = HMM_V3(0.2f, 0.2f, 0.2f);
    fs_params.light_diffuse = HMM_V3(0.5f, 0.5f, 0.5f);
    fs_params.light_specular = HMM_V3(1.0f, 1.0f, 1.0f);
    fs_params.mat_shininess = 32.0f;

    // FIX: Solo 2 argumentos
    sg_apply_uniforms(UB_fs_params, SG_RANGE(fs_params));

    HMM_Mat4 model = HMM_Rotate_RH(30.0f, HMM_V3(0.0f, 1.0f, 0.0f));
    vs_params.model = model;
    vs_params.mvp = proj * view * model;

    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));

    sg_draw(0, 36, 1);

    // --- LAMPARA ---
    sg_apply_pipeline(state.pip_lamp);
    sg_apply_bindings(&state.bind);
    HMM_Mat4 model_lamp = HMM_Translate(HMM_V3(1.2f, 1.0f, 2.0f)) * HMM_Scale(HMM_V3(0.2f, 0.2f, 0.2f));
    vs_params.model = model_lamp;
    vs_params.mvp = proj * view * model_lamp;

    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));

    sg_draw(0, 36, 1);

    sg_end_pass();
    sg_commit();
}

static void cleanup(void)
{
    sg_destroy_image(state.img_diffuse);
    sg_destroy_image(state.img_specular);
    sg_destroy_view(state.view_diffuse);
    sg_destroy_view(state.view_specular);
    sg_destroy_sampler(state.smp);
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
    desc.window_title = "15 - Mapa Especular";
    desc.logger.func = slog_func;
    return desc;
}