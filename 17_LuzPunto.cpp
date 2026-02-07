// -----------------------------------------------------------------------------
// 17_LuzPunto.cpp
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

// Shader generado
#include "17_LuzPunto.glsl.h"

// --- STRUCTS ---
struct {
    HMM_Mat4 mvp;
    HMM_Mat4 model;
} vs_params;

// Struct optimizado (80 bytes) - Empaquetando floats en los huecos
struct {
    HMM_Vec3 viewPos;
    float light_constant; // Offset 0
    HMM_Vec3 lightPos;
    float light_linear; // Offset 16
    HMM_Vec3 light_ambient;
    float light_quadratic; // Offset 32
    HMM_Vec3 light_diffuse;
    float mat_shininess; // Offset 48
    HMM_Vec3 light_specular;
    float _pad; // Offset 64
} fs_params;

// Posiciones para 10 cubos
HMM_Vec3 cubePositions[] = {
    { 0.0f, 0.0f, 0.0f },
    { 2.0f, 5.0f, -15.0f },
    { -1.5f, -2.2f, -2.5f },
    { -3.8f, -2.0f, -12.3f },
    { 2.4f, -0.4f, -3.5f },
    { -1.7f, 3.0f, -7.5f },
    { 1.3f, -2.0f, -2.5f },
    { 1.5f, 2.0f, -2.5f },
    { 1.5f, 0.2f, -1.5f },
    { -1.3f, 1.0f, -1.5f }
};

static struct {
    sg_pipeline pip_lighting; // Pipeline para objetos
    sg_pipeline pip_lamp; // Pipeline para la lámpara
    sg_bindings bind;
    sg_pass_action pass_action;

    sg_image img_diffuse;
    sg_image img_specular;
    sg_view view_diffuse;
    sg_view view_specular;
    sg_sampler smp;

    // Cámara
    float cam_dist;
    float cam_yaw;
    float cam_pitch;
    bool dragging;
    float last_mouse_x;
    float last_mouse_y;
} state;

// --- UTILS ---
sg_view load_texture_view(const char* filename)
{
    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename, &w, &h, &channels, 4);

    sg_image_desc img_desc = {};
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;

    if (data) {
        img_desc.width = w;
        img_desc.height = h;
        img_desc.data.mip_levels[0] = (sg_range) { .ptr = data, .size = (size_t)(w * h * 4) };
    } else {
        uint32_t pixel = 0xFF00FFFF; // Magenta error
        img_desc.width = 1;
        img_desc.height = 1;
        img_desc.data.mip_levels[0] = SG_RANGE(pixel);
    }

    sg_image img = sg_make_image(&img_desc);
    if (data)
        stbi_image_free(data);

    sg_view_desc view_desc = {};
    view_desc.texture.image = img;
    return sg_make_view(&view_desc);
}

// --- INIT ---
void init(void)
{
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    state.cam_dist = 10.0f;
    state.cam_yaw = 0.0f;
    state.cam_pitch = 20.0f;

    // Vértices (Cubo)
    float vertices[] = {
        // Pos                 // Normals           // UVs
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

    // Texturas
    state.view_diffuse = load_texture_view("texturas/container2.png");
    state.view_specular = load_texture_view("texturas/container2_specular.png");

    sg_sampler_desc smp_desc = {};
    smp_desc.min_filter = SG_FILTER_LINEAR;
    smp_desc.mag_filter = SG_FILTER_LINEAR;
    smp_desc.wrap_u = SG_WRAP_REPEAT;
    smp_desc.wrap_v = SG_WRAP_REPEAT;
    state.smp = sg_make_sampler(&smp_desc);

    state.bind.views[VIEW_material_diffuse].id = state.view_diffuse.id;
    state.bind.views[VIEW_material_specular].id = state.view_specular.id;
    state.bind.samplers[SMP_smp].id = state.smp.id;

    // --- PIPELINE DE ILUMINACIÓN ---
    sg_pipeline_desc pip = {};
    pip.shader = sg_make_shader(lighting_shader_desc(sg_query_backend()));
    pip.layout.attrs[ATTR_lighting_aPos].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[ATTR_lighting_aNormal].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[ATTR_lighting_aTexCoords].format = SG_VERTEXFORMAT_FLOAT2;
    pip.depth.write_enabled = true;
    pip.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;

    state.pip_lighting = sg_make_pipeline(&pip);

    // --- PIPELINE DE LÁMPARA ---
    sg_pipeline_desc pip2 = {};
    pip2.shader = sg_make_shader(lamp_shader_desc(sg_query_backend()));
    // Nota: El shader 'lamp' solo usa posición (aPos), pero como usamos el mismo buffer con stride completo,
    // debemos "saltar" los atributos que no usamos o definirlos.
    // Lo más fácil es definirlos igual aunque no se usen en el shader de la lámpara.
    pip2.layout.attrs[ATTR_lamp_aPos].format = SG_VERTEXFORMAT_FLOAT3;
    pip2.layout.attrs[ATTR_lamp_aNormal].format = SG_VERTEXFORMAT_FLOAT3;
    pip2.layout.attrs[ATTR_lamp_aTexCoords].format = SG_VERTEXFORMAT_FLOAT2;
    pip2.depth.write_enabled = true;
    pip2.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;

    state.pip_lamp = sg_make_pipeline(&pip2);

    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.1f, 0.1f, 0.1f, 1.0f };
}

// --- INPUT ---
void input(const sapp_event* ev)
{
    if (ev->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
        state.cam_dist -= ev->scroll_y;
        if (state.cam_dist < 2.0f)
            state.cam_dist = 2.0f;
        if (state.cam_dist > 50.0f)
            state.cam_dist = 50.0f;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_DOWN && ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        state.dragging = true;
        state.last_mouse_x = ev->mouse_x;
        state.last_mouse_y = ev->mouse_y;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_UP && ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        state.dragging = false;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_MOVE && state.dragging) {
        float dx = ev->mouse_x - state.last_mouse_x;
        float dy = ev->mouse_y - state.last_mouse_y;
        state.cam_yaw -= dx * 0.5f;
        state.cam_pitch -= dy * 0.5f;
        if (state.cam_pitch > 89.0f)
            state.cam_pitch = 89.0f;
        if (state.cam_pitch < -89.0f)
            state.cam_pitch = -89.0f;
        state.last_mouse_x = ev->mouse_x;
        state.last_mouse_y = ev->mouse_y;
    }
}

// --- FRAME ---
void frame(void)
{
    // 1. Calcular Cámara
    float r_yaw = HMM_ToRad(state.cam_yaw);
    float r_pitch = HMM_ToRad(state.cam_pitch);
    HMM_Vec3 camPos;
    camPos.X = state.cam_dist * cosf(r_pitch) * sinf(r_yaw);
    camPos.Y = state.cam_dist * sinf(r_pitch);
    camPos.Z = state.cam_dist * cosf(r_pitch) * cosf(r_yaw);

    HMM_Mat4 view = HMM_LookAt_RH(camPos, HMM_V3(0.0f, 0.0f, 0.0f), HMM_V3(0.0f, 1.0f, 0.0f));
    HMM_Mat4 proj = HMM_Perspective_RH_NO(60.0f, (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    // 2. Mover la luz (Orbita suavemente en el tiempo)
    float time = (float)sapp_frame_count() * 0.01f;
    HMM_Vec3 lightPos = HMM_V3(sinf(time) * 3.0f, 2.0f, cosf(time) * 3.0f);

    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);

    // --- DIBUJAR LOS 10 CUBOS ---
    sg_apply_pipeline(state.pip_lighting);
    sg_apply_bindings(&state.bind);

    fs_params.viewPos = camPos;
    fs_params.lightPos = lightPos;
    fs_params.light_ambient = HMM_V3(0.2f, 0.2f, 0.2f);
    fs_params.light_diffuse = HMM_V3(0.5f, 0.5f, 0.5f);
    fs_params.light_specular = HMM_V3(1.0f, 1.0f, 1.0f);

    // Parámetros de atenuación (valores típicos para distancia ~50)
    fs_params.light_constant = 1.0f;
    fs_params.light_linear = 0.09f;
    fs_params.light_quadratic = 0.032f;
    fs_params.mat_shininess = 32.0f;

    sg_apply_uniforms(UB_fs_params, SG_RANGE(fs_params));

    for (unsigned int i = 0; i < 10; i++) {
        HMM_Mat4 model = HMM_Translate(cubePositions[i]);
        float angle = 20.0f * i;
        model = model * HMM_Rotate_RH(angle, HMM_V3(1.0f, 0.3f, 0.5f));

        vs_params.model = model;
        vs_params.mvp = proj * view * model;
        sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));
        sg_draw(0, 36, 1);
    }

    // --- DIBUJAR LA LÁMPARA ---
    sg_apply_pipeline(state.pip_lamp);
    sg_apply_bindings(&state.bind);

    HMM_Mat4 model_lamp = HMM_Translate(lightPos) * HMM_Scale(HMM_V3(0.2f, 0.2f, 0.2f));
    vs_params.model = model_lamp;
    vs_params.mvp = proj * view * model_lamp;

    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));
    sg_draw(0, 36, 1);

    sg_end_pass();
    sg_commit();
}

void cleanup(void)
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
    desc.event_cb = input;
    desc.width = 800;
    desc.height = 600;
    desc.window_title = "17 - Luz de Punto (Point Light)";
    desc.icon.sokol_default = true;
    desc.logger.func = slog_func;
    return desc;
}