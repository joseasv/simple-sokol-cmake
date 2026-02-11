// -----------------------------------------------------------------------------
// 19_MultiplesLuces.cpp
// El ejemplo final: 1 Direccional, 4 Puntuales, 1 Flashlight
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

#include "19_MultiplesLuces.glsl.h"

// --- ESTRUCTURAS DE DATOS (ALINEACIÓN CRÍTICA) ---
// Nota: Usamos padding explícito para garantizar alineación std140

struct DirLight_t {
    HMM_Vec3 direction;
    float _pad0; // 16
    HMM_Vec3 ambient;
    float _pad1; // 32
    HMM_Vec3 diffuse;
    float _pad2; // 48
    HMM_Vec3 specular;
    float _pad3; // 64 bytes total
};

struct PointLight_t {
    HMM_Vec3 position;
    float constant; // 16 (Aprovechamos W)
    float linear;
    float quadratic;
    float _pad0[2]; // 16 bytes (padding manual)
    HMM_Vec3 ambient;
    float _pad1; // 16
    HMM_Vec3 diffuse;
    float _pad2; // 16
    HMM_Vec3 specular;
    float _pad3; // 16 -> Total 80 bytes
};
// Nota: Para simplificar el código de arriba en Sokol/Shader, a veces es mejor
// poner los floats sueltos (const, lin, quad) en los componentes W de los vec3
// para ahorrar espacio, pero aquí lo haremos explícito para claridad.

struct SpotLight_t {
    HMM_Vec3 position;
    float _pad0; // 16
    HMM_Vec3 direction;
    float cutOff; // 16
    float outerCutOff;
    float constant;
    float linear;
    float quadratic; // 16 (4 floats)
    HMM_Vec3 ambient;
    float _pad1; // 16
    HMM_Vec3 diffuse;
    float _pad2; // 16
    HMM_Vec3 specular;
    float _pad3; // 16 -> Total 96 bytes
};

// Uniform Block Principal
// --- ESTRUCTURA DE DATOS ALINEADA CON EL SHADER ---
// NOTA: Usamos padding manual para coincidir con el layout std140 del shader.
// Todo vec3 en GLSL ocupa 16 bytes (vec4) si no es el último elemento.

struct FS_Params {
    HMM_Vec3 viewPos;
    float mat_shininess; // Completa los 16 bytes

    // --- DirLight ---
    HMM_Vec3 dir_direction;
    float _pad0;
    HMM_Vec3 dir_ambient;
    float _pad1;
    HMM_Vec3 dir_diffuse;
    float _pad2;
    HMM_Vec3 dir_specular;
    float _pad3;

    // --- PointLights (Arrays) ---
    // En el shader son arrays de vec4 para evitar problemas de stride
    HMM_Vec4 pl_pos_const[4]; // xyz: pos, w: constant
    HMM_Vec4 pl_att[4]; // x: linear, y: quadratic, zw: pad
    HMM_Vec4 pl_ambient[4]; // rgb: color, w: pad
    HMM_Vec4 pl_diffuse[4];
    HMM_Vec4 pl_specular[4];

    // --- SpotLight ---
    HMM_Vec3 spot_position;
    float _pad4;
    HMM_Vec3 spot_direction;
    float _pad5;
    HMM_Vec3 spot_ambient;
    float _pad6;
    HMM_Vec3 spot_diffuse;
    float _pad7;
    HMM_Vec3 spot_specular;
    float _pad8;
    HMM_Vec3 spot_att;
    float _pad9; // x:const, y:lin, z:quad
    HMM_Vec2 spot_angles;
    float _pad10[2]; // x:cut, y:outerCut
} fs_params;

// Para los vertex shaders
struct {
    HMM_Mat4 mvp;
    HMM_Mat4 model;
} vs_params;

// Para el color de las lámparas pequeñas
struct {
    HMM_Vec3 light_color;
    float _pad;
} fs_lamp_params;

// --- DATOS DE LA ESCENA ---
HMM_Vec3 cubePositions[] = {
    { 0.0f, 0.0f, 0.0f }, { 2.0f, 5.0f, -15.0f }, { -1.5f, -2.2f, -2.5f },
    { -3.8f, -2.0f, -12.3f }, { 2.4f, -0.4f, -3.5f }, { -1.7f, 3.0f, -7.5f },
    { 1.3f, -2.0f, -2.5f }, { 1.5f, 2.0f, -2.5f }, { 1.5f, 0.2f, -1.5f },
    { -1.3f, 1.0f, -1.5f }
};

HMM_Vec3 pointLightPositions[] = {
    { 0.7f, 0.2f, 2.0f },
    { 2.3f, -3.3f, -4.0f },
    { -4.0f, 2.0f, -12.0f },
    { 0.0f, 0.0f, -3.0f }
};

HMM_Vec3 pointLightColors[] = {
    { 1.0f, 0.0f, 0.0f }, // Roja
    { 0.0f, 1.0f, 0.0f }, // Verde
    { 0.0f, 0.0f, 1.0f }, // Azul
    { 1.0f, 1.0f, 0.0f } // Amarilla
};

static struct {
    sg_pipeline pip;
    sg_pipeline pip_lamp;
    sg_bindings bind;
    sg_pass_action pass_action;

    sg_image img_diffuse;
    sg_image img_specular;
    sg_sampler smp;
    sg_view view_diffuse;
    sg_view view_specular;

    // Cámara
    float cam_dist;
    float cam_yaw;
    float cam_pitch;
    bool dragging;
    float last_mouse_x;
    float last_mouse_y;
} state;

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
        img_desc.width = 1;
        img_desc.height = 1;
        uint32_t p = 0xFF00FFFF;
        img_desc.data.mip_levels[0] = SG_RANGE(p);
    }
    sg_image img = sg_make_image(&img_desc);
    if (data)
        stbi_image_free(data);
    sg_view_desc v = {};
    v.texture.image = img;
    return sg_make_view(&v);
}

void init(void)
{
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    state.cam_dist = 6.0f;
    state.cam_yaw = -90.0f;
    state.cam_pitch = 0.0f;

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

    state.view_diffuse = load_texture_view("texturas/container2.png");
    state.view_specular = load_texture_view("texturas/container2_specular.png");

    sg_sampler_desc smp_desc = {};
    smp_desc.min_filter = SG_FILTER_LINEAR;
    smp_desc.mag_filter = SG_FILTER_LINEAR;
    state.smp = sg_make_sampler(&smp_desc);

    state.bind.views[VIEW_material_diffuse].id = state.view_diffuse.id;
    state.bind.views[VIEW_material_specular].id = state.view_specular.id;
    state.bind.samplers[SMP_smp].id = state.smp.id;

    // Pipeline Principal (Iluminación)
    sg_pipeline_desc pip = {};
    pip.shader = sg_make_shader(lighting_shader_desc(sg_query_backend()));
    pip.layout.attrs[ATTR_lighting_aPos].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[ATTR_lighting_aNormal].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[ATTR_lighting_aTexCoords].format = SG_VERTEXFORMAT_FLOAT2;
    pip.depth.write_enabled = true;
    pip.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    state.pip = sg_make_pipeline(&pip);

    // Pipeline Lámparas (Sin iluminación)
    sg_pipeline_desc pip_lamp = {};
    pip_lamp.shader = sg_make_shader(lamp_shader_desc(sg_query_backend()));
    pip_lamp.layout.attrs[ATTR_lamp_aPos].format = SG_VERTEXFORMAT_FLOAT3;
    pip_lamp.layout.attrs[ATTR_lamp_aNormal].format = SG_VERTEXFORMAT_FLOAT3; // Ignorado
    pip_lamp.layout.attrs[ATTR_lamp_aTexCoords].format = SG_VERTEXFORMAT_FLOAT2; // Ignorado
    pip_lamp.depth.write_enabled = true;
    pip_lamp.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    state.pip_lamp = sg_make_pipeline(&pip_lamp);

    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.1f, 0.1f, 0.1f, 1.0f };
}

void input(const sapp_event* ev)
{
    if (ev->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
        state.cam_dist -= ev->scroll_y;
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
        if (state.cam_pitch > 89.0f)
            state.cam_pitch = 89.0f;
        if (state.cam_pitch < -89.0f)
            state.cam_pitch = -89.0f;
        state.last_mouse_x = ev->mouse_x;
        state.last_mouse_y = ev->mouse_y;
    }
}

void frame(void)
{
    // --- 1. MATRICES ---
    float r_yaw = HMM_ToRad(state.cam_yaw);
    float r_pitch = HMM_ToRad(state.cam_pitch);
    HMM_Vec3 camPos = {
        state.cam_dist * cosf(r_pitch) * sinf(r_yaw),
        state.cam_dist * sinf(r_pitch),
        state.cam_dist * cosf(r_pitch) * cosf(r_yaw)
    };
    HMM_Vec3 camTarget = { 0.0f, 0.0f, 0.0f };
    HMM_Vec3 camFront = HMM_NormV3(camTarget - camPos);

    HMM_Mat4 view = HMM_LookAt_RH(camPos, camTarget, HMM_V3(0.0f, 1.0f, 0.0f));
    sg_backend backend = sg_query_backend();
    HMM_Mat4 proj;
    if (backend == SG_BACKEND_GLCORE) {
        // Linux / Mac antiguo (OpenGL): Usa rango -1 a 1
        proj = HMM_Perspective_RH_NO(45.0f, (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);
    } else {
        // Windows (D3D11) / Mac nuevo (Metal): Usa rango 0 a 1
        proj = HMM_Perspective_RH_ZO(45.0f, (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);
    }

    // --- 2. CONFIGURAR LUCES (UNIFORM BLOCK APLANADO) ---
    fs_params.viewPos = camPos;
    fs_params.mat_shininess = 32.0f;

    // A) Luz Direccional
    fs_params.dir_direction = HMM_V3(-0.2f, -1.0f, -0.3f);
    fs_params.dir_ambient = HMM_V3(0.05f, 0.05f, 0.05f);
    fs_params.dir_diffuse = HMM_V3(0.4f, 0.4f, 0.4f);
    fs_params.dir_specular = HMM_V3(0.5f, 0.5f, 0.5f);

    // B) Luces Puntuales (Llenamos los arrays)
    for (int i = 0; i < 4; i++) {
        // Empaquetamos Posición + Constant
        fs_params.pl_pos_const[i] = HMM_V4(
            pointLightPositions[i].X, pointLightPositions[i].Y, pointLightPositions[i].Z,
            1.0f // Constant
        );

        // Empaquetamos Linear + Quadratic
        fs_params.pl_att[i] = HMM_V4(0.09f, 0.032f, 0.0f, 0.0f);

        // Colores (Convertimos V3 a V4 poniendo w=0)
        fs_params.pl_ambient[i] = HMM_V4V(pointLightColors[i] * 0.1f, 0.0f);
        fs_params.pl_diffuse[i] = HMM_V4V(pointLightColors[i], 0.0f);
        fs_params.pl_specular[i] = HMM_V4V(pointLightColors[i], 0.0f);
    }

    // C) Spotlight
    fs_params.spot_position = camPos;
    fs_params.spot_direction = camFront;
    fs_params.spot_ambient = HMM_V3(0.0f, 0.0f, 0.0f);
    fs_params.spot_diffuse = HMM_V3(1.0f, 1.0f, 1.0f);
    fs_params.spot_specular = HMM_V3(1.0f, 1.0f, 1.0f);
    // Const, Linear, Quadratic
    fs_params.spot_att = HMM_V3(1.0f, 0.09f, 0.032f);
    // CutOff, OuterCutOff
    fs_params.spot_angles = HMM_V2(
        cosf(HMM_ToRad(12.5f)),
        cosf(HMM_ToRad(15.0f)));
    // --- RENDER ---
    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);

    // 1. Dibujar los 10 Cubos
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_apply_uniforms(UB_fs_params, SG_RANGE(fs_params));

    for (int i = 0; i < 10; i++) {
        HMM_Mat4 model = HMM_Translate(cubePositions[i]);
        float angle = 20.0f * i;
        model = model * HMM_Rotate_RH(angle, HMM_V3(1.0f, 0.3f, 0.5f));

        vs_params.model = model;
        vs_params.mvp = proj * view * model;
        sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));
        sg_draw(0, 36, 1);
    }

    // 2. Dibujar las 4 Lámparas (Pequeñas)
    sg_apply_pipeline(state.pip_lamp);
    sg_apply_bindings(&state.bind);

    for (int i = 0; i < 4; i++) {
        HMM_Mat4 model = HMM_Translate(pointLightPositions[i]);
        model = model * HMM_Scale(HMM_V3(0.2f, 0.2f, 0.2f));

        vs_params.model = model;
        vs_params.mvp = proj * view * model;
        sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params)); // Vertex uniform es igual

        fs_lamp_params.light_color = pointLightColors[i];
        sg_apply_uniforms(UB_fs_params_lamp, SG_RANGE(fs_lamp_params));

        sg_draw(0, 36, 1);
    }

    sg_end_pass();
    sg_commit();
}

void cleanup(void) { sg_shutdown(); }

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
    desc.window_title = "19 - Multiples Luces";
    desc.icon.sokol_default = true;
    desc.logger.func = slog_func;
    return desc;
}