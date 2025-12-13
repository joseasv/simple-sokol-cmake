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

    @fs fs
        in vec2 v_uv;
layout(binding = 0) uniform texture2D tex1;
layout(binding = 1) uniform texture2D tex2;
layout(binding = 0) uniform sampler smp1;
layout(binding = 1) uniform sampler smp2;

out vec4 frag_color;

void main()
{
    vec4 col1 = texture(sampler2D(tex1, smp1), v_uv);
    vec4 col2 = texture(sampler2D(tex2, smp2), v_uv);
    frag_color = mix(col1, col2, 0.2); // Mezcla 80/20
}
@end

    @program camara_orbital vs fs