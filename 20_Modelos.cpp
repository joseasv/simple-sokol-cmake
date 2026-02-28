// -----------------------------------------------------------------------------
// 20_Modelos.cpp
// -----------------------------------------------------------------------------

// --- 1. LIBRERÍAS DE IMPLEMENTACIÓN (DEFINES PRIMERO) ---
#define SOKOL_IMPL
#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_CPP_MODE
#define HANDMADE_MATH_USE_DEGREES
#define STB_IMAGE_IMPLEMENTATION

// --- 2. HEADERS DE LIBRERÍAS ---
#include <cstdint>
#include <string.h>
#include <string>
#include <vector>

#include "libs/HandmadeMath.h"
#include "libs/stb_image.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

// --- 3. HEADERS GENERADOS Y DE PROYECTO ---
#include "20_Modelos.glsl.h"
#include "Model.h"

// --- ESTRUCTURAS DE UNIFORMS (ALINEADAS CON SHADER) ---
struct vs_params_uniforms {
    HMM_Mat4 mvp;
    HMM_Mat4 model;
};

struct fs_params_uniforms {
    HMM_Vec3 viewPos;
    float _pad0;
    HMM_Vec3 lightPos;
    float _pad1;
    HMM_Vec3 lightColor;
    float _pad2;
};

// --- VARIABLES GLOBALES ---
static struct {
    sg_pipeline pip;
    sg_sampler smp;
    Model* myModel = nullptr;

    // Cámara
    float cam_dist = 5.0f;
    float cam_yaw = -90.0f;
    float cam_pitch = 20.0f;
    bool dragging = false;
    float last_mouse_x, last_mouse_y;
} state;

// --- IMPLEMENTACIÓN DE LA CLASE MODEL ---
Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures, sg_sampler smp)
{
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;
    setupMesh(smp);
}

void Mesh::Draw()
{
    sg_apply_bindings(&bind);
    sg_draw(0, (int)indices.size(), 1);
}

void Mesh::setupMesh(sg_sampler smp)
{
    bind = { 0 };

    sg_buffer_desc vbuf_desc = {};
    vbuf_desc.data = (sg_range) { .ptr = vertices.data(), .size = vertices.size() * sizeof(Vertex) };
    vbuf_desc.label = "mesh-vertices";
    bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

    sg_buffer_desc ibuf_desc = {};
    ibuf_desc.usage.index_buffer = true;
    ibuf_desc.data = (sg_range) { .ptr = indices.data(), .size = indices.size() * sizeof(unsigned int) };
    ibuf_desc.label = "mesh-indices";
    bind.index_buffer = sg_make_buffer(&ibuf_desc);

    for (unsigned int i = 0; i < textures.size(); i++) {
        if (textures[i].type == "texture_diffuse") {
            bind.views[VIEW_texture_diffuse1].id = textures[i].view.id;
        } else if (textures[i].type == "texture_specular") {
            bind.views[VIEW_texture_specular1].id = textures[i].view.id;
        }
    }
    bind.samplers[SMP_smp].id = smp.id;
}

Model::Model(const std::string& path, sg_sampler smp)
{
    this->sampler = smp;
    loadModel(path);
}

void Model::Draw()
{
    for (unsigned int i = 0; i < meshes.size(); i++) {
        meshes[i].Draw();
    }
}

void Model::loadModel(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        printf("ERROR::ASSIMP:: %s\n", importer.GetErrorString());
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
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
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, (aiTextureType)aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        std::vector<Texture> specularMaps = loadMaterialTextures(material, (aiTextureType)aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }

    return Mesh(vertices, indices, textures, this->sampler);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
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

sg_view Model::TextureFromFile(const char* path, const std::string& directory)
{
    std::string filename = directory + '/' + path;
    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 4);

    sg_image_desc img_desc = {};
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    img_desc.label = filename.c_str();
    if (data) {
        img_desc.width = width;
        img_desc.height = height;
        img_desc.data.mip_levels[0] = (sg_range) { .ptr = data, .size = (size_t)(width * height * 4) };
    } else {
        printf("Assimp: Falla al cargar textura en ruta: %s\n", filename.c_str());
        img_desc.width = 1;
        img_desc.height = 1;
        uint32_t p[] = { 0xFF00FFFF };
        img_desc.data.mip_levels[0] = (sg_range) { .ptr = p, .size = 4 };
    }
    sg_image img = sg_make_image(&img_desc);
    stbi_image_free(data);

    sg_view_desc view_desc = {};
    view_desc.texture.image = img;
    return sg_make_view(&view_desc);
}

// --- FUNCIÓN DE INICIALIZACIÓN ---
void init(void)
{
    sg_desc desc = {};
    desc.environment = sglue_environment();
    desc.logger.func = slog_func;
    desc.buffer_pool_size = 2048;
    desc.image_pool_size = 256;
    sg_setup(&desc);

    sg_sampler_desc smp_desc = {};
    smp_desc.min_filter = SG_FILTER_LINEAR;
    smp_desc.mag_filter = SG_FILTER_LINEAR;
    smp_desc.wrap_u = SG_WRAP_REPEAT;
    smp_desc.wrap_v = SG_WRAP_REPEAT;
    smp_desc.label = "model-sampler";
    state.smp = sg_make_sampler(&smp_desc);

    sg_pipeline_desc pip_desc = {};
    pip_desc.shader = sg_make_shader(lighting_shader_desc(sg_query_backend()));
    pip_desc.label = "model-pipeline";
    pip_desc.layout.attrs[ATTR_lighting_aPos].format = SG_VERTEXFORMAT_FLOAT3;
    pip_desc.layout.attrs[ATTR_lighting_aNormal].format = SG_VERTEXFORMAT_FLOAT3;
    pip_desc.layout.attrs[ATTR_lighting_aTexCoords].format = SG_VERTEXFORMAT_FLOAT2;
    pip_desc.index_type = SG_INDEXTYPE_UINT32;
    pip_desc.depth.write_enabled = true;
    pip_desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;
    // pip_desc.cull_mode = SG_CULLMODE_BACK;
    state.pip = sg_make_pipeline(&pip_desc);

    state.myModel = new Model("objects/backpack/backpack.obj", state.smp);
    //state.myModel = new Model("objects/Link(Adult)/Link Adult.obj", state.smp);
    //state.myModel = new Model("objects/Metal Gear REX/StgMetalgearRex.dae", state.smp);
}

// --- INPUT ---
void input(const sapp_event* ev)
{
    if (ev->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
        state.cam_dist -= ev->scroll_y * 0.5f;
        if (state.cam_dist < 1.0f)
            state.cam_dist = 1.0f;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_DOWN && ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        state.dragging = true;
        state.last_mouse_x = ev->mouse_x;
        state.last_mouse_y = ev->mouse_y;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_UP) {
        state.dragging = false;
    } else if (ev->type == SAPP_EVENTTYPE_MOUSE_MOVE && state.dragging) {
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

// --- FRAME ---
void frame(void)
{
    float r_yaw = HMM_ToRad(state.cam_yaw);
    float r_pitch = HMM_ToRad(state.cam_pitch);
    HMM_Vec3 camPos = {
        state.cam_dist * cosf(r_pitch) * sinf(r_yaw),
        state.cam_dist * sinf(r_pitch),
        state.cam_dist * cosf(r_pitch) * cosf(r_yaw)
    };
    HMM_Mat4 view = HMM_LookAt_RH(camPos, HMM_V3(0.0f, 1.0f, 0.0f), HMM_V3(0.0f, 1.0f, 0.0f));
    HMM_Mat4 proj = HMM_Perspective_RH_NO(60.0f, sapp_widthf() / sapp_heightf(), 0.1f, 100.0f);
    HMM_Mat4 model = HMM_Rotate_RH(HMM_ToRad(15.0f * (float)sapp_frame_count() * 0.01f), HMM_V3(0.0f, 1.0f, 0.0f));

    sg_pass_action pass_action = {};
    pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    pass_action.colors[0].clear_value = { 0.1f, 0.1f, 0.1f, 1.0f };

    sg_pass pass = {};
    pass.action = pass_action;
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);

    sg_apply_pipeline(state.pip);

    vs_params_uniforms vs_u;
    vs_u.model = model;
    vs_u.mvp = proj * view * model;
    sg_range vs_range = SG_RANGE(vs_u);
    sg_apply_uniforms(UB_vs_params, &vs_range);

    fs_params_uniforms fs_u;
    fs_u.viewPos = camPos;
    fs_u.lightPos = HMM_V3(2.0f, 4.0f, 2.0f);
    fs_u.lightColor = HMM_V3(1.0f, 1.0f, 1.0f);
    sg_range fs_range = SG_RANGE(fs_u);
    sg_apply_uniforms(UB_fs_params, &fs_range);

    if (state.myModel) {
        state.myModel->Draw();
    }

    sg_end_pass();
    sg_commit();
}

// --- CLEANUP ---
void cleanup(void)
{
    if (state.myModel) {
        for (auto& mesh : state.myModel->meshes) {
            for (auto& tex : mesh.textures) {
                sg_destroy_view(tex.view);
            }
        }
        delete state.myModel;
    }
    sg_destroy_sampler(state.smp);
    sg_destroy_pipeline(state.pip);
    sg_shutdown();
}

// --- SOKOL APP ENTRY POINT ---
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
    desc.window_title = "20 - Carga de Modelos (Assimp)";
    desc.logger.func = slog_func;
    return desc;
}
