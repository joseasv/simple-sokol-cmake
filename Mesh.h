#include <vector>
#include "Commons.h" // Donde definiste Vertex y Texture

class Mesh {
public:
    // Datos de la malla
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;

    // EL REEMPLAZO DEL VAO: Bindings de Sokol
    sg_bindings bind;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures) {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        setupMesh();
    }

    // Renderizamos pasando el pipeline (que contiene el shader)
    void Draw(sg_pipeline pip) {
        // 1. Aplicar Pipeline (Shader + Estados)
        // Nota: Generalmente el pipeline se aplica ANTES de iterar los meshes en el Model,
        // pero si cada mesh tuviera materiales muy distintos, podría ir aquí.
        // Asumiremos que el shader ya está activo o se pasa por fuera.

        // 2. Aplicar Bindings (Vértices + Índices + Texturas)
        sg_apply_bindings(&bind);

        // 3. Dibujar
        sg_draw(0, (int)indices.size(), 1);
    }

private:
    void setupMesh() {
        // A. Inicializar el struct de bindings a cero
        bind = {0};

        // B. Crear Vertex Buffer
        sg_buffer_desc vbuf_desc = {0};
        vbuf_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
        vbuf_desc.data.ptr = vertices.data();
        vbuf_desc.data.size = vertices.size() * sizeof(Vertex);
        bind.vertex_buffers[0] = sg_make_buffer(&vbuf_desc);

        // C. Crear Index Buffer
        sg_buffer_desc ibuf_desc = {0};
        ibuf_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
        ibuf_desc.data.ptr = indices.data();
        ibuf_desc.data.size = indices.size() * sizeof(unsigned int);
        bind.index_buffer = sg_make_buffer(&ibuf_desc);

        // D. Asignar Texturas a los slots del shader
        // Asumimos convención: Slot 0 = Diffuse, Slot 1 = Specular
        int diffuseNr = 1;
        int specularNr = 1;

        for(unsigned int i = 0; i < textures.size(); i++) {
            std::string name = textures[i].type;
            if(name == "texture_diffuse") {
                // Asignamos al slot 0 (fs_images[0])
                bind.fs_images[0] = textures[i].id;
            } else if(name == "texture_specular") {
                // Asignamos al slot 1 (fs_images[1])
                bind.fs_images[1] = textures[i].id;
            }
        }

        // OJO: Sokol requiere que si el shader espera una imagen en el slot 1,
        // y la malla NO tiene textura especular, asignes una textura "dummy" (blanca o negra).
        // De lo contrario, Sokol puede validar error o crashear.
    }
};s