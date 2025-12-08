// --- Vertex Shader ---
@vs vs
    in vec2 pos;
in vec2 uv;

out vec2 v_uv;

// Bloque de uniforms: La matriz de transformación
layout(binding = 0) uniform vs_params
{
    mat4 transform;
};

void main()
{
    // Multiplicamos Matriz * Vértice
    gl_Position = transform * vec4(pos, 0.0, 1.0);
    v_uv = uv;
}
@end

    // --- Fragment Shader ---
    @fs fs
        in vec2 v_uv;
// Definimos texturas y samplers por separado
layout(binding = 0) uniform texture2D tex;
layout(binding = 1) uniform sampler smp;
out vec4 frag_color;

void main()
{
    frag_color = texture(sampler2D(tex, smp), v_uv);
}
@end

    @program transformacion vs fs