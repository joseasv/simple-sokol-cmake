// --- Vertex Shader ---
@vs vs
    in vec3 pos;
in vec2 uv;

out vec2 v_uv;

layout(binding = 0) uniform vs_params
{
    mat4 mvp;
};

void main()
{
    gl_Position = mvp * vec4(pos, 1.0);
    v_uv = uv;
}
@end

    // --- Fragment Shader ---
    @fs fs
        in vec2 v_uv;

// Definimos 2 texturas y 2 samplers
layout(binding = 0) uniform texture2D tex1; // container.jpg
layout(binding = 1) uniform texture2D tex2; // textura.png (la carita)
layout(binding = 0) uniform sampler smp1;

out vec4 frag_color;

void main()
{
    vec4 col1 = texture(sampler2D(tex1, smp1), v_uv);
    vec4 col2 = texture(sampler2D(tex2, smp1), v_uv);

    // Mezclamos: 80% Container, 20% Textura2
    frag_color = mix(col1, col2, 0.2);
}
@end

    @program cubos_multiples vs fs