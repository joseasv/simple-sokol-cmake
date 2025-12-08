// --- Vertex Shader ---
@vs vs
    in vec3 pos; // Ahora es vec3
in vec2 uv;

out vec2 v_uv;

layout(binding = 0) uniform vs_params
{
    mat4 mvp;
};

void main()
{
    // Ya no añadimos 0.0 a pos, porque pos ya trae Z (es vec3)
    gl_Position = mvp * vec4(pos, 1.0);
    v_uv = uv;
}
@end

    // --- Fragment Shader ---
    @fs fs
        in vec2 v_uv;
layout(binding = 0) uniform texture2D tex;
layout(binding = 0) uniform sampler smp;
out vec4 frag_color;

void main()
{
    frag_color = texture(sampler2D(tex, smp), v_uv);
}
@end

    @program transformacion vs fs