// --- VERTEX SHADER ---
@vs vs_common
    in vec4 position;

// Binding 0 para VS
layout(binding = 0) uniform vs_params
{
    mat4 mvp;
};

void main()
{
    gl_Position = mvp * position;
}
@end

    // --- FRAGMENT SHADER 1 (Objeto) ---
    @fs fs_lighting

    // Binding 1 para FS (¡Para que no choque con el 0 del VS!)
    layout(binding = 1) uniform fs_params
{
    vec3 objectColor;
    vec3 lightColor;
};
out vec4 frag_color;

void main()
{
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    vec3 result = ambient * objectColor;
    frag_color = vec4(result, 1.0);
}
@end

    // --- FRAGMENT SHADER 2 (Lámpara) ---
    @fs fs_lamp
        out vec4 frag_color;

void main()
{
    frag_color = vec4(1.0, 1.0, 1.0, 1.0);
}
@end

    // --- PROGRAMAS ---
    @program lighting vs_common fs_lighting
    @program lamp vs_common fs_lamp