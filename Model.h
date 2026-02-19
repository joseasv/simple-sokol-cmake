#pragma once
#include "libs/HandmadeMath.h"
#include "libs/stb_image.h" // Asegúrate de que la ruta sea correcta
#include "sokol/sokol_gfx.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <string>
#include <vector>

// --- ESTRUCTURAS BÁSICAS ---
struct Vertex {
    HMM_Vec3 Position;
    HMM_Vec3 Normal;
    HMM_Vec2 TexCoords;
};

struct Texture {
    sg_view view; // CORRECCIÓN: Usamos sg_view en lugar de sg_image
    std::string type; // "texture_diffuse" o "texture_specular"
    std::string path;
};

// --- CLASE MESH ---
class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    sg_bindings bind;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        setupMesh();
    }

    void Draw(sg_pipeline pip)
    {
        sg_apply_bindings(&bind);
        sg_draw(0, (int)indices.size(), 1);
    }

private:
    void setupMesh()
    {
        bind = { 0 };

        // Vertex Buffer
        sg_buffer_desc vbuf_desc = { 0 };
        vbuf_desc.data.ptr = vertices.data();
        vbuf_desc.data.size = vertices.size() * sizeof(Vertex);
        bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

        // Index Buffer
        sg_buffer_desc ibuf_desc = { 0 };
        ibuf_desc.data.ptr = indices.data();
        ibuf_desc.data.size = indices.size() * sizeof(unsigned int);
        bind.index_buffer = sg_make_buffer(&ibuf_desc);

        // Asignar Texturas a los slots (Views)
        // Según el shader 20_Modelos.glsl: Slot 0 = Diffuse, Slot 1 = Specular
        for (unsigned int i = 0; i < textures.size(); i++) {
            if (textures[i].type == "texture_diffuse") {
                bind.views[0].id = textures[i].view.id;
            } else if (textures[i].type == "texture_specular") {
                bind.views[1].id = textures[i].view.id;
            }
        }
    }
};

// --- CLASE MODEL ---
class Model {
public:
    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> textures_loaded;

    Model(std::string const& path)
    {
        loadModel(path);
    }

    void Draw(sg_pipeline pip)
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(pip);
    }

private:
    void loadModel(std::string const& path)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            printf("ERROR::ASSIMP:: %s\n", importer.GetErrorString());
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode* node, const aiScene* scene)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            vertex.Position.X = mesh->mVertices[i].x;
            vertex.Position.Y = mesh->mVertices[i].y;
            vertex.Position.Z = mesh->mVertices[i].z;

            if (mesh->HasNormals()) {
                vertex.Normal.X = mesh->mNormals[i].x;
                vertex.Normal.Y = mesh->mNormals[i].y;
                vertex.Normal.Z = mesh->mNormals[i].z;
            }

            if (mesh->mTextureCoords[0]) {
                vertex.TexCoords.X = mesh->mTextureCoords[0][i].x;
                vertex.TexCoords.Y = mesh->mTextureCoords[0][i].y;
            } else {
                vertex.TexCoords = HMM_V2(0.0f, 0.0f);
            }
            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        }

        return Mesh(vertices, indices, textures);
    }

    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
    {
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++) {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            if (!skip) {
                Texture texture;
                texture.view = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }

    // CORRECCIÓN: Cargador de texturas idéntico a tus ejemplos anteriores
    sg_view TextureFromFile(const char* path, const std::string& directory)
    {
        std::string filename = std::string(path);
        filename = directory + '/' + filename;

        int width, height, nrComponents;
        // Obligar a voltear la textura
        stbi_set_flip_vertically_on_load(true);
        // Forzar 4 canales (RGBA) siempre, salva muchos dolores de cabeza
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 4);

        sg_image_desc img_desc = { 0 };
        img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;

        if (data) {
            img_desc.width = width;
            img_desc.height = height;
            img_desc.data.mip_levels[0] = (sg_range) { .ptr = data, .size = (size_t)(width * height * 4) };
        } else {
            printf("Assimp: Falla al cargar textura en ruta: %s\n", filename.c_str());
            // Textura por defecto magenta en caso de error
            img_desc.width = 1;
            img_desc.height = 1;
            uint32_t p = 0xFF00FFFF;
            img_desc.data.mip_levels[0] = SG_RANGE(p);
        }

        sg_image img = sg_make_image(&img_desc);
        if (data)
            stbi_image_free(data);

        // Generar la vista
        sg_view_desc view_desc = { 0 };
        view_desc.texture.image = img;
        return sg_make_view(&view_desc);
    }
};