#pragma once
#include "libs/HandmadeMath.h"
#include "sokol/sokol_gfx.h"
#include <string>

struct Vertex {
    HMM_Vec3 Position;
    HMM_Vec3 Normal;
    HMM_Vec2 TexCoords;
};

struct Texture {
    sg_image id; // En vez de GLuint, usamos sg_image
    std::string type; // "texture_diffuse" o "texture_specular"
    std::string path; // Para evitar cargar la misma textura 2 veces
};