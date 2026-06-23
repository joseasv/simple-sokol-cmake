// -----------------------------------------------------------------------------
// ROGUELIKE 3D - Juan Rivas
// -----------------------------------------------------------------------------

#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_CPP_MODE
#define HANDMADE_MATH_USE_DEGREES
#include "libs/HandmadeMath.h"
#include "libs/mapa.h"
#include <iostream>
#include <map>
#include <random>

#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"
#include <vector>

#define SOKOL_IMPL
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"
#include <math.h>

// Shader generado
#include "roguelike.glsl.h"

// --- CONSTANTES ---
const float CELDA_SIZE = 30.0f;
const float PARED_ALTURA = 30.0f;
const float ALTURA_OJOS = 6.5f;
const float PARED_OFFSET = 0.1f;
const float RADIO_JUGADOR = 1.5f;
const float CUERDA_ALTURA = 15.0f; // Altura de la cuerda
const float CUERDA_ANCHO = 2.0f; // Ancho de la cuerda

// --- NIVELES ---
const int NIVEL1_SIZE = 20;
const int NIVEL2_SIZE = 30;
const int NIVEL3_SIZE = 40;

const int HABITACIONES_NIVEL1 = 4;
const int HABITACIONES_NIVEL2 = 6;
const int HABITACIONES_NIVEL3 = 7;

const int ITEMS_MIN = 4;
const int ITEMS_MAX = 7;

// Estructura AABB para colisiones
struct AABB {
    HMM_Vec3 min;
    HMM_Vec3 max;

    AABB()
        : min({ 0, 0, 0 })
        , max({ 0, 0, 0 })
    {
    }
    AABB(HMM_Vec3 min, HMM_Vec3 max)
        : min(min)
        , max(max)
    {
    }

    bool colisionaCon(HMM_Vec3 punto, float radio) const
    {
        return (punto.X + radio > min.X && punto.X - radio < max.X && punto.Y + radio > min.Y && punto.Y - radio < max.Y && punto.Z + radio > min.Z && punto.Z - radio < max.Z);
    }
};

// Estructura para ítems
struct Item {
    HMM_Vec3 posicion;
    bool recogido;
    int idHabitacion;

    Item()
        : recogido(false)
        , idHabitacion(-1)
    {
    }
    Item(float x, float y, float z, int id)
        : posicion({ x, y, z })
        , recogido(false)
        , idHabitacion(id)
    {
    }
};

// Estructura para la cuerda
struct Cuerda {
    HMM_Vec3 posicion;
    bool activa;
    int idHabitacion;

    Cuerda()
        : activa(false)
        , idHabitacion(-1)
    {
    }
};

// Estructura para almacenar un nivel completo
struct Nivel {
    int size;
    int numHabitaciones;
    vector<vector<char>> mapa;
    vector<PuntoBorde> bordes;
    vector<PuntoPasillo> pasillos;
    vector<PosicionHabitacion> habitacionesInfo;
    vector<AABB> paredesAABB;
    vector<Item> items;
    Cuerda cuerda; // Cambiado de escalera a cuerda
    bool generado;
    int itemsRecogidos;
    HMM_Vec3 camaraPos;

    Nivel()
        : size(0)
        , numHabitaciones(0)
        , generado(false)
        , itemsRecogidos(0)
    {
    }

    void limpiarAABB()
    {
        paredesAABB.clear();
    }

    void generarItems(int numItems)
    {
        items.clear();
        itemsRecogidos = 0;

        if (habitacionesInfo.empty())
            return;

        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> habDist(0, habitacionesInfo.size() - 1);

        for (int i = 0; i < numItems; i++) {
            // Seleccionar habitación aleatoria
            int habIndex = habDist(gen);
            const auto& hab = habitacionesInfo[habIndex];

            // Posición aleatoria dentro de la habitación
            uniform_int_distribution<> filaDist(hab.arriba, hab.abajo);
            uniform_int_distribution<> colDist(hab.izquierda, hab.derecha);

            int fila = filaDist(gen);
            int col = colDist(gen);

            float offsetX = -(size * CELDA_SIZE) / 2.0f;
            float offsetZ = -(size * CELDA_SIZE) / 2.0f;

            float x = offsetX + col * CELDA_SIZE + CELDA_SIZE / 2;
            float z = offsetZ + fila * CELDA_SIZE + CELDA_SIZE / 2;

            items.push_back(Item(x, 2.0f, z, habIndex));
        }
    }

    void generarCuerda()
    { // Cambiado de generarEscalera a generarCuerda
        cuerda.activa = false;
        if (habitacionesInfo.empty())
            return;

        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> habDist(0, habitacionesInfo.size() - 1);

        int habIndex = habDist(gen);
        const auto& hab = habitacionesInfo[habIndex];

        float offsetX = -(size * CELDA_SIZE) / 2.0f;
        float offsetZ = -(size * CELDA_SIZE) / 2.0f;

        float x = offsetX + hab.centro_col * CELDA_SIZE + CELDA_SIZE / 2;
        float z = offsetZ + hab.centro_fila * CELDA_SIZE + CELDA_SIZE / 2;

        cuerda.posicion = HMM_V3(x, 0.0f, z); // La base en el suelo
        cuerda.idHabitacion = habIndex;
    }

    void recogerItem(HMM_Vec3 posJugador)
    {
        for (auto& item : items) {
            if (!item.recogido) {
                float dist = sqrt(pow(posJugador.X - item.posicion.X, 2) + pow(posJugador.Z - item.posicion.Z, 2));
                if (dist < CELDA_SIZE / 2) {
                    item.recogido = true;
                    itemsRecogidos++;
                    printf("Item recogido! %d/%d\n", itemsRecogidos, (int)items.size());

                    // Activar cuerda si todos los items han sido recogidos
                    if (itemsRecogidos == items.size()) {
                        cuerda.activa = true;
                        printf("¡Cuerda activada en habitacion %d!\n", cuerda.idHabitacion + 1);
                    }
                    break;
                }
            }
        }
    }
};

// --- STRUCTS ---
struct {
    HMM_Mat4 mvp;
    HMM_Mat4 model;
} vs_params;

struct {
    HMM_Vec3 viewPos;
    float _pad1;
    HMM_Vec3 lightPos;
    float _pad2;
    HMM_Vec3 light_ambient;
    float _pad3;
    HMM_Vec3 light_diffuse;
    float _pad4;
    HMM_Vec3 light_specular;
    float mat_shininess;
} fs_params;

// --- ESTRUCTURA PARA MATERIALES ---
struct Material {
    sg_image img_diffuse;
    sg_image img_specular;
    sg_view view_diffuse;
    sg_view view_specular;
    const char* name;
};

// --- VÉRTICES PARA UN CUBO PEQUEÑO (ITEM) ---
float cubo_vertices[] = {
    // Cara frontal
    -0.5f,
    -0.5f,
    0.5f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    0.5f,
    -0.5f,
    0.5f,
    0.0f,
    0.0f,
    1.0f,
    1.0f,
    0.0f,
    0.5f,
    0.5f,
    0.5f,
    0.0f,
    0.0f,
    1.0f,
    1.0f,
    1.0f,
    0.5f,
    0.5f,
    0.5f,
    0.0f,
    0.0f,
    1.0f,
    1.0f,
    1.0f,
    -0.5f,
    0.5f,
    0.5f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    1.0f,
    -0.5f,
    -0.5f,
    0.5f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,

    // Cara trasera
    -0.5f,
    -0.5f,
    -0.5f,
    0.0f,
    0.0f,
    -1.0f,
    0.0f,
    0.0f,
    0.5f,
    -0.5f,
    -0.5f,
    0.0f,
    0.0f,
    -1.0f,
    1.0f,
    0.0f,
    0.5f,
    0.5f,
    -0.5f,
    0.0f,
    0.0f,
    -1.0f,
    1.0f,
    1.0f,
    0.5f,
    0.5f,
    -0.5f,
    0.0f,
    0.0f,
    -1.0f,
    1.0f,
    1.0f,
    -0.5f,
    0.5f,
    -0.5f,
    0.0f,
    0.0f,
    -1.0f,
    0.0f,
    1.0f,
    -0.5f,
    -0.5f,
    -0.5f,
    0.0f,
    0.0f,
    -1.0f,
    0.0f,
    0.0f,

    // Cara izquierda
    -0.5f,
    -0.5f,
    -0.5f,
    -1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    -0.5f,
    -0.5f,
    0.5f,
    -1.0f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    -0.5f,
    0.5f,
    0.5f,
    -1.0f,
    0.0f,
    0.0f,
    1.0f,
    1.0f,
    -0.5f,
    0.5f,
    0.5f,
    -1.0f,
    0.0f,
    0.0f,
    1.0f,
    1.0f,
    -0.5f,
    0.5f,
    -0.5f,
    -1.0f,
    0.0f,
    0.0f,
    0.0f,
    1.0f,
    -0.5f,
    -0.5f,
    -0.5f,
    -1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,

    // Cara derecha
    0.5f,
    -0.5f,
    -0.5f,
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.5f,
    -0.5f,
    0.5f,
    1.0f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    0.5f,
    0.5f,
    0.5f,
    1.0f,
    0.0f,
    0.0f,
    1.0f,
    1.0f,
    0.5f,
    0.5f,
    0.5f,
    1.0f,
    0.0f,
    0.0f,
    1.0f,
    1.0f,
    0.5f,
    0.5f,
    -0.5f,
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    1.0f,
    0.5f,
    -0.5f,
    -0.5f,
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,

    // Cara inferior
    -0.5f,
    -0.5f,
    -0.5f,
    0.0f,
    -1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.5f,
    -0.5f,
    -0.5f,
    0.0f,
    -1.0f,
    0.0f,
    1.0f,
    0.0f,
    0.5f,
    -0.5f,
    0.5f,
    0.0f,
    -1.0f,
    0.0f,
    1.0f,
    1.0f,
    0.5f,
    -0.5f,
    0.5f,
    0.0f,
    -1.0f,
    0.0f,
    1.0f,
    1.0f,
    -0.5f,
    -0.5f,
    0.5f,
    0.0f,
    -1.0f,
    0.0f,
    0.0f,
    1.0f,
    -0.5f,
    -0.5f,
    -0.5f,
    0.0f,
    -1.0f,
    0.0f,
    0.0f,
    0.0f,

    // Cara superior
    -0.5f,
    0.5f,
    -0.5f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.5f,
    0.5f,
    -0.5f,
    0.0f,
    1.0f,
    0.0f,
    1.0f,
    0.0f,
    0.5f,
    0.5f,
    0.5f,
    0.0f,
    1.0f,
    0.0f,
    1.0f,
    1.0f,
    0.5f,
    0.5f,
    0.5f,
    0.0f,
    1.0f,
    0.0f,
    1.0f,
    1.0f,
    -0.5f,
    0.5f,
    0.5f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    1.0f,
    -0.5f,
    0.5f,
    -0.5f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    0.0f,
};

static struct {
    sg_pipeline pip_object;
    sg_pipeline pip_lamp;
    sg_bindings bind;
    sg_pass_action pass_action;
    sg_sampler smp;

    // MATERIALES
    Material material_pared;
    Material material_piso;
    Material material_item; // Textura para ítems (también para la cuerda)
    // Eliminado material_escalera

    // BUFFERS
    sg_buffer buf_piso;
    sg_buffer buf_pared_z;
    sg_buffer buf_pared_x;
    sg_buffer buf_cubo;

    // LUZ ESTÁTICA
    float yaw, pitch;
    float lastMouseX, lastMouseY;
    bool isDragging;
    HMM_Vec3 camPos;
    bool wPressed, aPressed, sPressed, dPressed;

    // NIVELES
    Nivel nivel1;
    Nivel nivel2;
    Nivel nivel3;
    int nivelActual;

    bool nivelesGenerados;
} state;

// --- PROTOTIPOS ---
static sg_image load_texture(const char* filename);
static void init_materials(void);
static void dibujarPiso(float x, float y, float z, HMM_Mat4 proj, HMM_Mat4 view);
static void dibujarParedZ(float x, float y, float z, HMM_Mat4 proj, HMM_Mat4 view);
static void dibujarParedX(float x, float y, float z, HMM_Mat4 proj, HMM_Mat4 view);
static void dibujarItem(float x, float y, float z, float tamaño, HMM_Mat4 proj, HMM_Mat4 view);
static void dibujarCuerda(float x, float y, float z, HMM_Mat4 proj, HMM_Mat4 view); // Nueva función
static void generarNiveles(void);
static void cambiarNivel(int nuevoNivel);
static void dibujarNivelActual(HMM_Mat4 proj, HMM_Mat4 view);
static bool puedeMoverseA(HMM_Vec3 nuevaPos, Nivel* nivel);
static bool posicionEsValida(HMM_Vec3 pos, Nivel* nivel);
static void asegurarPosicionCamara(Nivel* nivel);

// --- EVENTOS CÁMARA FPS ---
static void event(const sapp_event* ev)
{
    if (ev->type == SAPP_EVENTTYPE_MOUSE_DOWN && ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        state.isDragging = true;
        state.lastMouseX = ev->mouse_x;
        state.lastMouseY = ev->mouse_y;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_UP && ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        state.isDragging = false;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_MOVE && state.isDragging) {
        state.pitch -= (ev->mouse_y - state.lastMouseY) * 0.3f;
        state.yaw += (ev->mouse_x - state.lastMouseX) * 0.3f;

        if (state.pitch > 89.0f)
            state.pitch = 89.0f;
        if (state.pitch < -89.0f)
            state.pitch = -89.0f;

        state.lastMouseX = ev->mouse_x;
        state.lastMouseY = ev->mouse_y;
    }

    else if (ev->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (ev->key_code == SAPP_KEYCODE_W)
            state.wPressed = true;
        else if (ev->key_code == SAPP_KEYCODE_A)
            state.aPressed = true;
        else if (ev->key_code == SAPP_KEYCODE_S)
            state.sPressed = true;
        else if (ev->key_code == SAPP_KEYCODE_D)
            state.dPressed = true;
        else if (ev->key_code == SAPP_KEYCODE_1) {
            cambiarNivel(1);
        } else if (ev->key_code == SAPP_KEYCODE_2) {
            cambiarNivel(2);
        } else if (ev->key_code == SAPP_KEYCODE_3) {
            cambiarNivel(3);
        }
    } else if (ev->type == SAPP_EVENTTYPE_KEY_UP) {
        if (ev->key_code == SAPP_KEYCODE_W)
            state.wPressed = false;
        else if (ev->key_code == SAPP_KEYCODE_A)
            state.aPressed = false;
        else if (ev->key_code == SAPP_KEYCODE_S)
            state.sPressed = false;
        else if (ev->key_code == SAPP_KEYCODE_D)
            state.dPressed = false;
    }
}

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
        desc.data.mip_levels[0].ptr = data;
        desc.data.mip_levels[0].size = (size_t)(w * h * 4);
        // printf("Cargada textura: %s (%dx%d)\n", filename, w, h);
    } else {
        uint32_t pixel = 0xFF00FFFF;
        desc.width = 1;
        desc.height = 1;
        desc.data.mip_levels[0] = SG_RANGE(pixel);
        printf("ERROR: No se pudo cargar textura: %s\n", filename);
    }

    sg_image img = sg_make_image(&desc);
    if (data)
        stbi_image_free(data);
    return img;
}

static void init_materials(void)
{
    // --- MATERIAL PARA PARED (Textura 1) ---
    state.material_pared.img_diffuse = load_texture("texturas/basecolor1.png");
    state.material_pared.img_specular = load_texture("texturas/specular1.png");
    state.material_pared.name = "Wall";

    sg_view_desc view_desc = {};
    view_desc.texture.image = state.material_pared.img_diffuse;
    state.material_pared.view_diffuse = sg_make_view(&view_desc);
    view_desc.texture.image = state.material_pared.img_specular;
    state.material_pared.view_specular = sg_make_view(&view_desc);

    // --- MATERIAL PARA PISO (Textura 2) ---
    state.material_piso.img_diffuse = load_texture("texturas/basecolor2.png");
    state.material_piso.img_specular = load_texture("texturas/specular2.png");
    state.material_piso.name = "Floor";

    view_desc.texture.image = state.material_piso.img_diffuse;
    state.material_piso.view_diffuse = sg_make_view(&view_desc);
    view_desc.texture.image = state.material_piso.img_specular;
    state.material_piso.view_specular = sg_make_view(&view_desc);

    // --- MATERIAL PARA ITEM (Textura 3) - También usado para la cuerda ---
    state.material_item.img_diffuse = load_texture("texturas/basecolor3.png");
    state.material_item.img_specular = load_texture("texturas/specular3.png");
    state.material_item.name = "Item";

    view_desc.texture.image = state.material_item.img_diffuse;
    state.material_item.view_diffuse = sg_make_view(&view_desc);
    view_desc.texture.image = state.material_item.img_specular;
    state.material_item.view_specular = sg_make_view(&view_desc);
}

static void generarNiveles(void)
{
    printf("\n=== GENERANDO NIVELES ===\n");

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> itemDist(ITEMS_MIN, ITEMS_MAX);

    // ===== NIVEL 1 =====
    state.nivel1.size = NIVEL1_SIZE;
    state.nivel1.numHabitaciones = HABITACIONES_NIVEL1;
    generarMapaCompleto(state.nivel1.size, state.nivel1.size, state.nivel1.numHabitaciones,
        state.nivel1.mapa, state.nivel1.bordes,
        state.nivel1.pasillos, state.nivel1.habitacionesInfo);
    state.nivel1.generado = true;

    state.nivel1.generarItems(itemDist(gen));
    state.nivel1.generarCuerda();

    // Mostrar informacion del NIVEL 1
    printf("\n======================================");
    printf("\n=           NIVEL 1 (%dx%d)            =", state.nivel1.size, state.nivel1.size);
    printf("\n======================================\n");
    mostrarMapa(state.nivel1.mapa);
    mostrarInfoBordes(state.nivel1.bordes);
    mostrarInfoPasillos(state.nivel1.pasillos);
    mostrarInfoHabitaciones(state.nivel1.habitacionesInfo);
    printf("\nItems en nivel 1: %d\n", (int)state.nivel1.items.size());

    // Calcular posicion de camara para nivel 1
    for (const auto& hab : state.nivel1.habitacionesInfo) {
        if (hab.id == 0) {
            float offsetX = -(state.nivel1.size * CELDA_SIZE) / 2.0f;
            float offsetZ = -(state.nivel1.size * CELDA_SIZE) / 2.0f;
            state.nivel1.camaraPos.X = offsetX + hab.centro_col * CELDA_SIZE + CELDA_SIZE / 2;
            state.nivel1.camaraPos.Y = ALTURA_OJOS;
            state.nivel1.camaraPos.Z = offsetZ + hab.centro_fila * CELDA_SIZE + CELDA_SIZE / 2;
            break;
        }
    }

    // ===== NIVEL 2 =====
    state.nivel2.size = NIVEL2_SIZE;
    state.nivel2.numHabitaciones = HABITACIONES_NIVEL2;
    generarMapaCompleto(state.nivel2.size, state.nivel2.size, state.nivel2.numHabitaciones,
        state.nivel2.mapa, state.nivel2.bordes,
        state.nivel2.pasillos, state.nivel2.habitacionesInfo);
    state.nivel2.generado = true;

    state.nivel2.generarItems(itemDist(gen));
    state.nivel2.generarCuerda();

    // Mostrar informacion del NIVEL 2
    printf("\n======================================");
    printf("\n║           NIVEL 2 (%dx%d)          =", state.nivel2.size, state.nivel2.size);
    printf("\n======================================\n");
    mostrarMapa(state.nivel2.mapa);
    mostrarInfoBordes(state.nivel2.bordes);
    mostrarInfoPasillos(state.nivel2.pasillos);
    mostrarInfoHabitaciones(state.nivel2.habitacionesInfo);
    printf("\nItems en nivel 2: %d\n", (int)state.nivel2.items.size());

    // Calcular posicion de camara para nivel 2
    for (const auto& hab : state.nivel2.habitacionesInfo) {
        if (hab.id == 0) {
            float offsetX = -(state.nivel2.size * CELDA_SIZE) / 2.0f;
            float offsetZ = -(state.nivel2.size * CELDA_SIZE) / 2.0f;
            state.nivel2.camaraPos.X = offsetX + hab.centro_col * CELDA_SIZE + CELDA_SIZE / 2;
            state.nivel2.camaraPos.Y = ALTURA_OJOS;
            state.nivel2.camaraPos.Z = offsetZ + hab.centro_fila * CELDA_SIZE + CELDA_SIZE / 2;
            break;
        }
    }

    // ===== NIVEL 3 =====
    state.nivel3.size = NIVEL3_SIZE;
    state.nivel3.numHabitaciones = HABITACIONES_NIVEL3;
    generarMapaCompleto(state.nivel3.size, state.nivel3.size, state.nivel3.numHabitaciones,
        state.nivel3.mapa, state.nivel3.bordes,
        state.nivel3.pasillos, state.nivel3.habitacionesInfo);
    state.nivel3.generado = true;

    state.nivel3.generarItems(itemDist(gen));
    state.nivel3.generarCuerda();

    // Mostrar informacion del NIVEL 3
    printf("\n======================================");
    printf("\n=           NIVEL 3 (%dx%d)          =", state.nivel3.size, state.nivel3.size);
    printf("\n======================================\n");
    mostrarMapa(state.nivel3.mapa);
    mostrarInfoBordes(state.nivel3.bordes);
    mostrarInfoPasillos(state.nivel3.pasillos);
    mostrarInfoHabitaciones(state.nivel3.habitacionesInfo);
    printf("\nItems en nivel 3: %d\n", (int)state.nivel3.items.size());

    // Calcular posicion de camara para nivel 3
    for (const auto& hab : state.nivel3.habitacionesInfo) {
        if (hab.id == 0) {
            float offsetX = -(state.nivel3.size * CELDA_SIZE) / 2.0f;
            float offsetZ = -(state.nivel3.size * CELDA_SIZE) / 2.0f;
            state.nivel3.camaraPos.X = offsetX + hab.centro_col * CELDA_SIZE + CELDA_SIZE / 2;
            state.nivel3.camaraPos.Y = ALTURA_OJOS;
            state.nivel3.camaraPos.Z = offsetZ + hab.centro_fila * CELDA_SIZE + CELDA_SIZE / 2;
            break;
        }
    }
}

static bool posicionEsValida(HMM_Vec3 pos, Nivel* nivel)
{
    if (!nivel)
        return false;

    float offsetX = -(nivel->size * CELDA_SIZE) / 2.0f;
    float offsetZ = -(nivel->size * CELDA_SIZE) / 2.0f;

    int col = (int)((pos.X - offsetX) / CELDA_SIZE);
    int fila = (int)((pos.Z - offsetZ) / CELDA_SIZE);

    if (col < 0 || col >= nivel->size || fila < 0 || fila >= nivel->size) {
        return false;
    }

    // Verificar que no sea una pared
    return (nivel->mapa[fila][col] != '|');
}

static void asegurarPosicionCamara(Nivel* nivel)
{
    if (!nivel)
        return;

    // Verificar si la posición actual es válida
    if (!posicionEsValida(state.camPos, nivel)) {
        // Si no es válida, usar la posición guardada del nivel
        state.camPos = nivel->camaraPos;
        printf("Camara reubicada en posicion segura: (%.2f, %.2f, %.2f)\n",
            state.camPos.X, state.camPos.Y, state.camPos.Z);
    }
}

static void cambiarNivel(int nuevoNivel)
{
    if (nuevoNivel == state.nivelActual)
        return;

    state.nivelActual = nuevoNivel;

    if (nuevoNivel == 1 && state.nivel1.generado) {
        state.camPos = state.nivel1.camaraPos;
        asegurarPosicionCamara(&state.nivel1);
        printf("Cambiado a NIVEL 1 (%d ítems restantes)\n",
            (int)(state.nivel1.items.size() - state.nivel1.itemsRecogidos));
    } else if (nuevoNivel == 2 && state.nivel2.generado) {
        state.camPos = state.nivel2.camaraPos;
        asegurarPosicionCamara(&state.nivel2);
        printf("Cambiado a NIVEL 2 (%d ítems restantes)\n",
            (int)(state.nivel2.items.size() - state.nivel2.itemsRecogidos));
    } else if (nuevoNivel == 3 && state.nivel3.generado) {
        state.camPos = state.nivel3.camaraPos;
        asegurarPosicionCamara(&state.nivel3);
        printf("Cambiado a NIVEL 3 (%d ítems restantes)\n",
            (int)(state.nivel3.items.size() - state.nivel3.itemsRecogidos));
    }

    state.yaw = -90.0f;
    state.pitch = 0.0f;
}

static void init(void)
{
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    sg_setup(&desc);

    // Valores iniciales (temporales)
    state.yaw = -90.0f;
    state.pitch = 0.0f;
    state.camPos = HMM_V3(0.0f, ALTURA_OJOS, 0.0f);
    state.wPressed = false;
    state.aPressed = false;
    state.sPressed = false;
    state.dPressed = false;
    state.nivelActual = 1;
    state.nivelesGenerados = false;

    // --- VÉRTICES ---
    float piso_vertices[] = {
        -0.5f,
        0.0f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.5f,
        0.0f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        0.0f,
        0.5f,
        0.0f,
        0.5f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        1.0f,
        0.5f,
        0.0f,
        0.5f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        1.0f,
        -0.5f,
        0.0f,
        0.5f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        -0.5f,
        0.0f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
    };

    float pared_z_vertices[] = {
        -0.5f,
        -0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.5f,
        -0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        0.0f,
        0.5f,
        0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        1.0f,
        0.5f,
        0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        1.0f,
        -0.5f,
        0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        -0.5f,
        -0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,

        -0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f,
        0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        1.0f,
        0.0f,
        0.5f,
        0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        1.0f,
        1.0f,
        0.5f,
        0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        1.0f,
        1.0f,
        -0.5f,
        0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        0.0f,
        1.0f,
        -0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f,
    };

    float pared_x_vertices[] = {
        -0.5f,
        -0.5f,
        -0.5f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        -0.5f,
        -0.5f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        -0.5f,
        0.5f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        -0.5f,
        0.5f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        -0.5f,
        0.5f,
        -0.5f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        -0.5f,
        -0.5f,
        -0.5f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,

        0.5f,
        -0.5f,
        -0.5f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.5f,
        -0.5f,
        0.5f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.5f,
        0.5f,
        0.5f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        0.5f,
        0.5f,
        0.5f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        0.5f,
        0.5f,
        -0.5f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        0.5f,
        -0.5f,
        -0.5f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
    };

    // Crear buffers
    sg_buffer_desc vbuf_piso = {};
    vbuf_piso.data = SG_RANGE(piso_vertices);
    state.buf_piso = sg_make_buffer(&vbuf_piso);

    sg_buffer_desc vbuf_pared_z = {};
    vbuf_pared_z.data = SG_RANGE(pared_z_vertices);
    state.buf_pared_z = sg_make_buffer(&vbuf_pared_z);

    sg_buffer_desc vbuf_pared_x = {};
    vbuf_pared_x.data = SG_RANGE(pared_x_vertices);
    state.buf_pared_x = sg_make_buffer(&vbuf_pared_x);

    sg_buffer_desc vbuf_cubo = {};
    vbuf_cubo.data = SG_RANGE(cubo_vertices);
    state.buf_cubo = sg_make_buffer(&vbuf_cubo);

    init_materials();

    sg_sampler_desc smp_desc = {};
    smp_desc.min_filter = SG_FILTER_LINEAR;
    smp_desc.mag_filter = SG_FILTER_LINEAR;
    smp_desc.wrap_u = SG_WRAP_REPEAT;
    smp_desc.wrap_v = SG_WRAP_REPEAT;
    state.smp = sg_make_sampler(&smp_desc);
    state.bind.samplers[SMP_smp].id = state.smp.id;

    // Pipeline para objetos
    sg_pipeline_desc pip = {};
    pip.layout.buffers[0].stride = 32;
    pip.layout.attrs[ATTR_lighting_aPos].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[ATTR_lighting_aPos].offset = 0;
    pip.layout.attrs[ATTR_lighting_aNormal].format = SG_VERTEXFORMAT_FLOAT3;
    pip.layout.attrs[ATTR_lighting_aNormal].offset = 12;
    pip.layout.attrs[ATTR_lighting_aTexCoords].format = SG_VERTEXFORMAT_FLOAT2;
    pip.layout.attrs[ATTR_lighting_aTexCoords].offset = 24;

    pip.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    pip.depth.write_enabled = true;
    pip.shader = sg_make_shader(lighting_shader_desc(sg_query_backend()));
    state.pip_object = sg_make_pipeline(&pip);

    // Pipeline para lámpara
    sg_pipeline_desc pip_lamp = {};
    pip_lamp.layout.buffers[0].stride = 32;
    pip_lamp.layout.attrs[ATTR_lighting_aPos].format = SG_VERTEXFORMAT_FLOAT3;
    pip_lamp.layout.attrs[ATTR_lighting_aPos].offset = 0;
    pip_lamp.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    pip_lamp.depth.write_enabled = true;
    pip_lamp.shader = sg_make_shader(lamp_shader_desc(sg_query_backend()));
    state.pip_lamp = sg_make_pipeline(&pip_lamp);

    state.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    state.pass_action.colors[0].clear_value.r = 0.1f;
    state.pass_action.colors[0].clear_value.g = 0.1f;
    state.pass_action.colors[0].clear_value.b = 0.1f;
    state.pass_action.colors[0].clear_value.a = 1.0f;
    state.pass_action.depth.load_action = SG_LOADACTION_CLEAR;
    state.pass_action.depth.clear_value = 1.0f;
}

void dibujarPiso(float x, float y, float z, HMM_Mat4 proj, HMM_Mat4 view)
{
    state.bind.views[VIEW_material_diffuse].id = state.material_piso.view_diffuse.id;
    state.bind.views[VIEW_material_specular].id = state.material_piso.view_specular.id;
    state.bind.vertex_buffers[0] = state.buf_piso;
    sg_apply_bindings(&state.bind);

    HMM_Mat4 model = HMM_Translate(HMM_V3(x, y, z));
    model = HMM_MulM4(model, HMM_Scale(HMM_V3(CELDA_SIZE, 1.0f, CELDA_SIZE)));

    vs_params.model = model;
    vs_params.mvp = proj * view * model;
    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));
    sg_draw(0, 6, 1);
}

void dibujarParedZ(float x, float y, float z, HMM_Mat4 proj, HMM_Mat4 view)
{
    state.bind.views[VIEW_material_diffuse].id = state.material_pared.view_diffuse.id;
    state.bind.views[VIEW_material_specular].id = state.material_pared.view_specular.id;
    state.bind.vertex_buffers[0] = state.buf_pared_z;
    sg_apply_bindings(&state.bind);

    HMM_Mat4 model = HMM_Translate(HMM_V3(x, y, z));
    model = HMM_MulM4(model, HMM_Scale(HMM_V3(CELDA_SIZE, PARED_ALTURA, 1.0f)));

    vs_params.model = model;
    vs_params.mvp = proj * view * model;
    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));
    sg_draw(0, 12, 1);
}

void dibujarParedX(float x, float y, float z, HMM_Mat4 proj, HMM_Mat4 view)
{
    state.bind.views[VIEW_material_diffuse].id = state.material_pared.view_diffuse.id;
    state.bind.views[VIEW_material_specular].id = state.material_pared.view_specular.id;
    state.bind.vertex_buffers[0] = state.buf_pared_x;
    sg_apply_bindings(&state.bind);

    HMM_Mat4 model = HMM_Translate(HMM_V3(x, y, z));
    model = HMM_MulM4(model, HMM_Scale(HMM_V3(1.0f, PARED_ALTURA, CELDA_SIZE)));

    vs_params.model = model;
    vs_params.mvp = proj * view * model;
    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));
    sg_draw(0, 12, 1);
}

void dibujarItem(float x, float y, float z, float tamaño, HMM_Mat4 proj, HMM_Mat4 view)
{
    state.bind.views[VIEW_material_diffuse].id = state.material_item.view_diffuse.id;
    state.bind.views[VIEW_material_specular].id = state.material_item.view_specular.id;
    state.bind.vertex_buffers[0] = state.buf_cubo;
    sg_apply_bindings(&state.bind);

    HMM_Mat4 model = HMM_Translate(HMM_V3(x, y, z));
    model = HMM_MulM4(model, HMM_Scale(HMM_V3(tamaño, tamaño, tamaño)));

    vs_params.model = model;
    vs_params.mvp = proj * view * model;
    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));
    sg_draw(0, 36, 1);
}

// Nueva función para dibujar la cuerda (cubo alargado)
void dibujarCuerda(float x, float y, float z, HMM_Mat4 proj, HMM_Mat4 view)
{
    state.bind.views[VIEW_material_diffuse].id = state.material_item.view_diffuse.id;
    state.bind.views[VIEW_material_specular].id = state.material_item.view_specular.id;
    state.bind.vertex_buffers[0] = state.buf_cubo;
    sg_apply_bindings(&state.bind);

    // Crear un cubo alargado verticalmente
    // La posición y se ajusta para que la base esté en el suelo y el cubo se eleve
    HMM_Mat4 model = HMM_Translate(HMM_V3(x, y + CUERDA_ALTURA / 2, z));
    model = HMM_MulM4(model, HMM_Scale(HMM_V3(CUERDA_ANCHO, CUERDA_ALTURA, CUERDA_ANCHO)));

    vs_params.model = model;
    vs_params.mvp = proj * view * model;
    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));
    sg_draw(0, 36, 1);
}

void dibujarNivelActual(HMM_Mat4 proj, HMM_Mat4 view)
{
    Nivel* nivel = nullptr;

    if (state.nivelActual == 1)
        nivel = &state.nivel1;
    else if (state.nivelActual == 2)
        nivel = &state.nivel2;
    else if (state.nivelActual == 3)
        nivel = &state.nivel3;
    else
        return;

    if (!nivel->generado)
        return;

    nivel->limpiarAABB();

    float offsetX = -(nivel->size * CELDA_SIZE) / 2.0f;
    float offsetZ = -(nivel->size * CELDA_SIZE) / 2.0f;

    // DIBUJAR HABITACIONES (pisos y techos)
    auto coordsHabitaciones = obtenerCoordenadasHabitaciones(nivel->habitacionesInfo);
    for (int i = 0; i < coordsHabitaciones.size(); i++) {
        int fila = coordsHabitaciones[i].first;
        int col = coordsHabitaciones[i].second;

        float x = offsetX + col * CELDA_SIZE + CELDA_SIZE / 2;
        float z = offsetZ + fila * CELDA_SIZE + CELDA_SIZE / 2;

        dibujarPiso(x, 0.0f, z, proj, view);
        dibujarPiso(x, PARED_ALTURA, z, proj, view);
    }

    // DIBUJAR PASILLOS (pisos y techos)
    for (int i = 0; i < nivel->pasillos.size(); i++) {
        int fila = nivel->pasillos[i].fila;
        int col = nivel->pasillos[i].col;

        float x = offsetX + col * CELDA_SIZE + CELDA_SIZE / 2;
        float z = offsetZ + fila * CELDA_SIZE + CELDA_SIZE / 2;

        dibujarPiso(x, 0.0f, z, proj, view);
        dibujarPiso(x, PARED_ALTURA, z, proj, view);
    }

    // DIBUJAR PAREDES DE PASILLOS Y CREAR AABBS
    int filas = nivel->size;
    int cols = nivel->size;

    for (int i = 0; i < nivel->pasillos.size(); i++) {
        int fila = nivel->pasillos[i].fila;
        int col = nivel->pasillos[i].col;

        float x = offsetX + col * CELDA_SIZE + CELDA_SIZE / 2;
        float z = offsetZ + fila * CELDA_SIZE + CELDA_SIZE / 2;

        // Norte
        if (fila > 0) {
            char celdaNorte = nivel->mapa[fila - 1][col];
            if (celdaNorte == '*' || celdaNorte == '|') {
                bool esHabitacion = false;
                for (const auto& hab : nivel->habitacionesInfo) {
                    if (fila - 1 >= hab.arriba && fila - 1 <= hab.abajo && col >= hab.izquierda && col <= hab.derecha) {
                        esHabitacion = true;
                        break;
                    }
                }
                if (!esHabitacion) {
                    float px = x;
                    float pz = z - CELDA_SIZE / 2 + PARED_OFFSET;
                    dibujarParedZ(px, PARED_ALTURA / 2.0f, pz, proj, view);

                    AABB pared;
                    pared.min = HMM_V3(px - CELDA_SIZE / 2, 0, pz - 0.5f);
                    pared.max = HMM_V3(px + CELDA_SIZE / 2, PARED_ALTURA, pz + 0.5f);
                    nivel->paredesAABB.push_back(pared);
                }
            }
        }

        // Sur
        if (fila < filas - 1) {
            char celdaSur = nivel->mapa[fila + 1][col];
            if (celdaSur == '*' || celdaSur == '|') {
                bool esHabitacion = false;
                for (const auto& hab : nivel->habitacionesInfo) {
                    if (fila + 1 >= hab.arriba && fila + 1 <= hab.abajo && col >= hab.izquierda && col <= hab.derecha) {
                        esHabitacion = true;
                        break;
                    }
                }
                if (!esHabitacion) {
                    float px = x;
                    float pz = z + CELDA_SIZE / 2 - PARED_OFFSET;
                    dibujarParedZ(px, PARED_ALTURA / 2.0f, pz, proj, view);

                    AABB pared;
                    pared.min = HMM_V3(px - CELDA_SIZE / 2, 0, pz - 0.5f);
                    pared.max = HMM_V3(px + CELDA_SIZE / 2, PARED_ALTURA, pz + 0.5f);
                    nivel->paredesAABB.push_back(pared);
                }
            }
        }

        // Este
        if (col < cols - 1) {
            char celdaEste = nivel->mapa[fila][col + 1];
            if (celdaEste == '*' || celdaEste == '|') {
                bool esHabitacion = false;
                for (const auto& hab : nivel->habitacionesInfo) {
                    if (fila >= hab.arriba && fila <= hab.abajo && col + 1 >= hab.izquierda && col + 1 <= hab.derecha) {
                        esHabitacion = true;
                        break;
                    }
                }
                if (!esHabitacion) {
                    float px = x + CELDA_SIZE / 2 - PARED_OFFSET;
                    float pz = z;
                    dibujarParedX(px, PARED_ALTURA / 2.0f, pz, proj, view);

                    AABB pared;
                    pared.min = HMM_V3(px - 0.5f, 0, pz - CELDA_SIZE / 2);
                    pared.max = HMM_V3(px + 0.5f, PARED_ALTURA, pz + CELDA_SIZE / 2);
                    nivel->paredesAABB.push_back(pared);
                }
            }
        }

        // Oeste
        if (col > 0) {
            char celdaOeste = nivel->mapa[fila][col - 1];
            if (celdaOeste == '*' || celdaOeste == '|') {
                bool esHabitacion = false;
                for (const auto& hab : nivel->habitacionesInfo) {
                    if (fila >= hab.arriba && fila <= hab.abajo && col - 1 >= hab.izquierda && col - 1 <= hab.derecha) {
                        esHabitacion = true;
                        break;
                    }
                }
                if (!esHabitacion) {
                    float px = x - CELDA_SIZE / 2 + PARED_OFFSET;
                    float pz = z;
                    dibujarParedX(px, PARED_ALTURA / 2.0f, pz, proj, view);

                    AABB pared;
                    pared.min = HMM_V3(px - 0.5f, 0, pz - CELDA_SIZE / 2);
                    pared.max = HMM_V3(px + 0.5f, PARED_ALTURA, pz + CELDA_SIZE / 2);
                    nivel->paredesAABB.push_back(pared);
                }
            }
        }
    }

    // DIBUJAR BORDES DE HABITACIONES
    for (int i = 0; i < nivel->bordes.size(); i++) {
        int fila = nivel->bordes[i].fila;
        int col = nivel->bordes[i].col;
        char dir = nivel->bordes[i].direccion;

        float x = offsetX + col * CELDA_SIZE + CELDA_SIZE / 2;
        float z = offsetZ + fila * CELDA_SIZE + CELDA_SIZE / 2;

        bool esInicioFin = false;
        for (int j = 0; j < nivel->pasillos.size(); j++) {
            if (nivel->pasillos[j].fila == fila && nivel->pasillos[j].col == col) {
                esInicioFin = true;
                break;
            }
        }

        if (!esInicioFin) {
            AABB pared;
            if (dir == 'N') {
                dibujarParedZ(x, PARED_ALTURA / 2.0f, z + CELDA_SIZE / 2, proj, view);
                pared.min = HMM_V3(x - CELDA_SIZE / 2, 0, z + CELDA_SIZE / 2 - 0.5f);
                pared.max = HMM_V3(x + CELDA_SIZE / 2, PARED_ALTURA, z + CELDA_SIZE / 2 + 0.5f);
            } else if (dir == 'S') {
                dibujarParedZ(x, PARED_ALTURA / 2.0f, z - CELDA_SIZE / 2, proj, view);
                pared.min = HMM_V3(x - CELDA_SIZE / 2, 0, z - CELDA_SIZE / 2 - 0.5f);
                pared.max = HMM_V3(x + CELDA_SIZE / 2, PARED_ALTURA, z - CELDA_SIZE / 2 + 0.5f);
            } else if (dir == 'E') {
                dibujarParedX(x - CELDA_SIZE / 2, PARED_ALTURA / 2.0f, z, proj, view);
                pared.min = HMM_V3(x - CELDA_SIZE / 2 - 0.5f, 0, z - CELDA_SIZE / 2);
                pared.max = HMM_V3(x - CELDA_SIZE / 2 + 0.5f, PARED_ALTURA, z + CELDA_SIZE / 2);
            } else if (dir == 'O') {
                dibujarParedX(x + CELDA_SIZE / 2, PARED_ALTURA / 2.0f, z, proj, view);
                pared.min = HMM_V3(x + CELDA_SIZE / 2 - 0.5f, 0, z - CELDA_SIZE / 2);
                pared.max = HMM_V3(x + CELDA_SIZE / 2 + 0.5f, PARED_ALTURA, z + CELDA_SIZE / 2);
            }
            nivel->paredesAABB.push_back(pared);
        }
    }

    // DIBUJAR ÍTEMS
    for (const auto& item : nivel->items) {
        if (!item.recogido) {
            dibujarItem(item.posicion.X, item.posicion.Y, item.posicion.Z, 2.0f, proj, view);
        }
    }

    // DIBUJAR CUERDA SI ESTÁ ACTIVA (reemplaza a la escalera)
    if (nivel->cuerda.activa) {
        dibujarCuerda(nivel->cuerda.posicion.X, nivel->cuerda.posicion.Y,
            nivel->cuerda.posicion.Z, proj, view);
    }
}

static bool puedeMoverseA(HMM_Vec3 nuevaPos, Nivel* nivel)
{
    if (!nivel || !nivel->generado)
        return false;

    float offsetX = -(nivel->size * CELDA_SIZE) / 2.0f;
    float offsetZ = -(nivel->size * CELDA_SIZE) / 2.0f;
    float limiteMinX = offsetX;
    float limiteMaxX = offsetX + nivel->size * CELDA_SIZE;
    float limiteMinZ = offsetZ;
    float limiteMaxZ = offsetZ + nivel->size * CELDA_SIZE;

    if (nuevaPos.X - RADIO_JUGADOR < limiteMinX || nuevaPos.X + RADIO_JUGADOR > limiteMaxX || nuevaPos.Z - RADIO_JUGADOR < limiteMinZ || nuevaPos.Z + RADIO_JUGADOR > limiteMaxZ) {
        return false;
    }

    for (const auto& pared : nivel->paredesAABB) {
        if (pared.colisionaCon(nuevaPos, RADIO_JUGADOR)) {
            return false;
        }
    }

    return true;
}

static void frame(void)
{
    float w = sapp_widthf();
    float h = sapp_heightf();

    // Generar niveles solo una vez
    if (!state.nivelesGenerados) {
        generarNiveles();
        state.nivelesGenerados = true;

        // Posicionar cámara en nivel 1 ANTES del primer frame
        state.camPos = state.nivel1.camaraPos;
        state.nivelActual = 1;

        // Verificar que la posición sea válida
        asegurarPosicionCamara(&state.nivel1);

        printf("Posicion inicial de camara: (%.2f, %.2f, %.2f)\n",
            state.camPos.X, state.camPos.Y, state.camPos.Z);
    }

    // LUZ ESTÁTICA
    HMM_Vec3 lightPos = HMM_V3(10.0f, 15.0f, 20.0f);

    // Obtener nivel actual
    Nivel* nivelActual = nullptr;
    if (state.nivelActual == 1)
        nivelActual = &state.nivel1;
    else if (state.nivelActual == 2)
        nivelActual = &state.nivel2;
    else if (state.nivelActual == 3)
        nivelActual = &state.nivel3;

    // Movimiento y detección de colisiones
    float rY = HMM_ToRad(state.yaw);
    float moveSpeed = 0.5f;

    HMM_Vec3 front = { cosf(rY), 0.0f, sinf(rY) };
    front = HMM_NormV3(front);
    HMM_Vec3 right = { -front.Z, 0.0f, front.X };

    // Guardar posición anterior para detección de ítems
    HMM_Vec3 oldPos = state.camPos;

    if (state.wPressed && nivelActual) {
        HMM_Vec3 newPos = HMM_AddV3(state.camPos, HMM_MulV3F(front, moveSpeed));
        if (puedeMoverseA(newPos, nivelActual)) {
            state.camPos = newPos;
        }
    }
    if (state.sPressed && nivelActual) {
        HMM_Vec3 newPos = HMM_SubV3(state.camPos, HMM_MulV3F(front, moveSpeed));
        if (puedeMoverseA(newPos, nivelActual)) {
            state.camPos = newPos;
        }
    }
    if (state.dPressed && nivelActual) {
        HMM_Vec3 newPos = HMM_AddV3(state.camPos, HMM_MulV3F(right, moveSpeed));
        if (puedeMoverseA(newPos, nivelActual)) {
            state.camPos = newPos;
        }
    }
    if (state.aPressed && nivelActual) {
        HMM_Vec3 newPos = HMM_SubV3(state.camPos, HMM_MulV3F(right, moveSpeed));
        if (puedeMoverseA(newPos, nivelActual)) {
            state.camPos = newPos;
        }
    }

    state.camPos.Y = ALTURA_OJOS;

    // Detección de recogida de ítems
    if (nivelActual && (state.camPos.X != oldPos.X || state.camPos.Z != oldPos.Z)) {
        nivelActual->recogerItem(state.camPos);

        // Detección de cuerda para cambiar de nivel (reemplaza a escalera)
        if (nivelActual->cuerda.activa) {
            float distCuerda = sqrt(pow(state.camPos.X - nivelActual->cuerda.posicion.X, 2) + pow(state.camPos.Z - nivelActual->cuerda.posicion.Z, 2));
            if (distCuerda < CELDA_SIZE / 2 && state.nivelActual < 3) {
                cambiarNivel(state.nivelActual + 1);
            }
        }
    }

    // Cámara FPS
    float rP = HMM_ToRad(state.pitch);
    HMM_Vec3 lookDir = { cosf(rP) * cosf(rY), sinf(rP), cosf(rP) * sinf(rY) };
    lookDir = HMM_NormV3(lookDir);
    HMM_Vec3 target = HMM_AddV3(state.camPos, lookDir);
    HMM_Vec3 up = { 0.0f, 1.0f, 0.0f };

    HMM_Mat4 view = HMM_LookAt_RH(state.camPos, target, up);

    sg_backend backend = sg_query_backend();
    HMM_Mat4 proj = (backend == SG_BACKEND_GLCORE) ? HMM_Perspective_RH_NO(45.0f, w / h, 0.1f, 1000.0f) : HMM_Perspective_RH_ZO(45.0f, w / h, 0.1f, 1000.0f);

    sg_pass pass = {};
    pass.action = state.pass_action;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);

    // Uniforms de iluminación
    fs_params.viewPos = state.camPos;
    fs_params.lightPos = lightPos;
    fs_params.light_ambient = HMM_V3(0.3f, 0.3f, 0.3f);
    fs_params.light_diffuse = HMM_V3(1.0f, 0.5f, 0.2f);
    fs_params.light_specular = HMM_V3(1.0f, 1.0f, 1.0f);
    fs_params.mat_shininess = 32.0f;

    sg_apply_pipeline(state.pip_object);
    sg_apply_uniforms(UB_fs_params, SG_RANGE(fs_params));

    // Dibujar nivel actual
    dibujarNivelActual(proj, view);

    // Lámpara
    sg_apply_pipeline(state.pip_lamp);
    state.bind.views[VIEW_material_diffuse].id = state.material_pared.view_diffuse.id;
    state.bind.views[VIEW_material_specular].id = state.material_pared.view_specular.id;
    state.bind.vertex_buffers[0] = state.buf_pared_z;
    sg_apply_bindings(&state.bind);

    HMM_Mat4 model_lamp = HMM_Translate(lightPos) * HMM_Scale(HMM_V3(0.1f, 0.1f, 0.1f));
    vs_params.model = model_lamp;
    vs_params.mvp = proj * view * model_lamp;
    sg_apply_uniforms(UB_vs_params, SG_RANGE(vs_params));
    sg_draw(0, 12, 1);

    sg_end_pass();
    sg_commit();
}

static void cleanup(void)
{
    sg_destroy_image(state.material_pared.img_diffuse);
    sg_destroy_image(state.material_pared.img_specular);
    sg_destroy_view(state.material_pared.view_diffuse);
    sg_destroy_view(state.material_pared.view_specular);

    sg_destroy_image(state.material_piso.img_diffuse);
    sg_destroy_image(state.material_piso.img_specular);
    sg_destroy_view(state.material_piso.view_diffuse);
    sg_destroy_view(state.material_piso.view_specular);

    sg_destroy_image(state.material_item.img_diffuse);
    sg_destroy_image(state.material_item.img_specular);
    sg_destroy_view(state.material_item.view_diffuse);
    sg_destroy_view(state.material_item.view_specular);

    sg_destroy_buffer(state.buf_piso);
    sg_destroy_buffer(state.buf_pared_z);
    sg_destroy_buffer(state.buf_pared_x);
    sg_destroy_buffer(state.buf_cubo);
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
    desc.window_title = "Roguelike 3D";
    desc.logger.func = slog_func;
    return desc;
}