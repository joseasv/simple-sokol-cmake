#pragma once

#include <string>
#include <vector>

// Forward-declare Sokol and HandmadeMath types to avoid including full headers.
// The .cpp file is responsible for including the actual headers.
struct sg_sampler;
struct sg_view;
struct sg_bindings;
union HMM_Vec2;
union HMM_Vec3;

// Forward-declare Assimp types in the global namespace
struct aiNode;
struct aiMesh;
struct aiScene;
struct aiMaterial;
enum aiTextureType;

// --- ESTRUCTURAS BÁSICAS ---
struct Vertex {
    HMM_Vec3 Position;
    HMM_Vec3 Normal;
    HMM_Vec2 TexCoords;
};

struct Texture {
    sg_view view;
    std::string type;
    std::string path;
};

// --- CLASE MESH ---
class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    sg_bindings bind;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures, sg_sampler smp);
    void Draw();

private:
    void setupMesh(sg_sampler smp);
};

// --- CLASE MODEL ---
class Model {
public:
    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> textures_loaded;
    sg_sampler sampler;

    Model(const std::string& path, sg_sampler smp);
    void Draw();

private:
    sg_view TextureFromFile(const char* path, const std::string& directory);
    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};
