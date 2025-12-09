// 1. Matemáticas
#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_CPP_MODE
#define HANDMADE_MATH_USE_DEGREES
#include "libs/HandmadeMath.h"

// 2. Librerías
#define SOKOL_IMPL
#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"

#include <math.h>
#include <stdio.h>

// 3. Shader
#include "06_10Cubos.glsl.h"

// Estado Global
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;

    // Recursos para 2 texturas
    sg_image img_container;
    sg_image img_face;
    sg_sampler smp; // Usaremos el mismo sampler para ambas
    sg_view view_container;
    sg_view view_face;
} state;

// Estructura de Uniforms (Limpia)
struct {
    HMM_Mat4 mvp;
} vs_params;

// Posiciones de los 10 cubos (Copiado de LearnOpenGL)
HMM_Vec3 cubePositions[] = {
    HMM_V3(0.0f, 0.0f, 0.0f),
    HMM_V3(2.0f, 5.0f, -15.0f),
    HMM_V3(-1.5f, -2.2f, -2.5f),
    HMM_V3(-3.8f, -2.0f, -12.3f),
    HMM_V3(2.4f, -0.4f, -3.5f),
    HMM_V3(-1.7f, 3.0f, -7.5f),
    HMM_V3(1.3f, -2.0f, -2.5f),
    HMM_V3(1.5f, 2.0f, -2.5f),
    HMM_V3(1.5f, 0.2f, -1.5f),
    HMM_V3(-1.3f, 1.0f, -1.5f)
};

// Función auxiliar para cargar textura
sg_image load_texture(const char* filename)
{
    int w, h, c;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* pixels = stbi_load(filename, &w, &h, &c, 4);

    sg_image_desc desc = {};
    desc.pixel_format = SG_PIXELFORMAT_RGBA8;

    if (pixels) {
        printf("Cargada: %s (%dx%d)\n", filename, w, h);
        desc.width = w;
        desc.height = h;
        desc.data.mip_levels[0] = (sg_range) { pixels, (size_t)(w * h * 4) };
    } else {
        // Fallback negro
        printf("FALLO: %s\n", filename);
        uint32_t negro = 0xFF000000;
        desc.width = 1;
        desc.height = 1;
        desc.data.mip_levels[0] = SG_RANGE(negro);
    }

    sg_image img = sg_make_image(&desc);
    if (pixels)
        stbi_image_free(pixels);
    return img;
}

void init(void)
{
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    // 1. Vértices del Cubo (36 vértices)
    float vertices[] = {
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,

        -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,

        0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f
    };

    sg_buffer_desc vbuf = {};
    vbuf.data = SG_RANGE(vertices);
    state.bind.vertex_buffers[0] = sg_make_buffer(&vbuf);

    // 2. Cargar Texturas
    state.img_container = load_texture("texturas/container.jpg");
    state.img_face = load_texture("texturas/textura.png");

    // 3. Crear Sampler (Uno para las dos)
    sg_sampler_desc smp_desc = {};
    smp_desc.min_filter = SG_FILTER_LINEAR;
    smp_desc.mag_filter = SG_FILTER_LINEAR;
    smp_desc.wrap_u = SG_WRAP_REPEAT;
    smp_desc.wrap_v = SG_WRAP_REPEAT;
    state.smp = sg_make_sampler(&smp_desc);

    // 4. Crear Vistas (Legacy API)
    sg_view_desc view1 = {};
    view1.texture.image = state.img_container;
    state.view_container = sg_make_view(&view1);

    sg_view_desc view2 = {};
    view2.texture.image = state.img_face;
    state.view_face = sg_make_view(&view2);

    // 5. Vincular a los Slots del Shader
    state.bind.views[VIEW_tex1].id = state.view_container.id; // Slot 0
    state.bind.views[VIEW_tex2].id = state.view_face.id; // Slot 1

    state.bind.samplers[SMP_smp1].id = state.smp.id;

    // 6. Pipeline
    sg_pipeline_desc pip = {};
    pip.shader = sg_make_shader(cubos_multiples_shader_desc(sg_query_backend()));
    pip.layout.attrs[ATTR_cubos_multiples_pos].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[ATTR_cubos_multiples_uv].format = SG_VERTEXFORMAT_FLOAT2;

    // Z-BUFFER ACTIVADO
    pip.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    pip.depth.write_enabled = true;

    state.pip = sg_make_pipeline(&pip);

    // 7. Clear
    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value = { 0.2f, 0.3f, 0.3f, 1.0f };
    state.pass_action.depth.load_action = SG_LOADACTION_CLEAR;
    state.pass_action.depth.clear_value = 1.0f;
}

void frame(void)
{
    float w = (float)sapp_width();
    float h = (float)sapp_height();
    float t = (float)sapp_frame_duration();
    static float time_acc = 0.0f;
    time_acc += t;

    // 1. Proyección y Vista (Comunes para todos los cubos)
    HMM_Mat4 proj = HMM_Perspective_RH_NO(45.0f, w / h, 0.1f, 100.0f);
    HMM_Mat4 view = HMM_Translate(HMM_V3(0.0f, 0.0f, -3.0f));

    // 2. Inicio del Render Pass
    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);

    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    // 3. Bucle para dibujar 10 cubos
    for (int i = 0; i < 10; i++) {
        // A. Trasladar a la posición del array
        HMM_Mat4 model = HMM_Translate(cubePositions[i]);

        // B. Rotar cada cubo diferente
        // El ángulo depende del índice 'i'
        float angle = 20.0f * i;

        // LearnOpenGL rota cada 3er cubo con el tiempo, los demás fijos
        if (i % 3 == 0) {
            angle += time_acc * 50.0f; // Rotación animada
        }

        // Aplicamos rotación en eje arbitrario (1.0, 0.3, 0.5) como el tutorial
        model = model * HMM_Rotate_RH(angle, HMM_V3(1.0f, 0.3f, 0.5f));

        // C. Calcular MVP Final (Sobrecarga de operador *)
        HMM_Mat4 mvp = proj * view * model;

        // D. Enviar Uniforms (Struct + Asignación)
        vs_params.mvp = mvp;

        sg_range params_range = SG_RANGE(vs_params);
        sg_apply_uniforms(0, &params_range);

        // E. Dibujar el cubo actual
        sg_draw(0, 36, 1);
    }

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
    desc.width = 800;
    desc.height = 600;
    desc.window_title = "06 - 10 Cubos (LearnOpenGL)";
    desc.logger.func = slog_func;
    return desc;
}