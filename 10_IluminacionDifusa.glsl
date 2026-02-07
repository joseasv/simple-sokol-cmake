// --- VERTEX SHADER ---
@vs vs
    in vec3 aPos;
in vec3 aNormal;

out vec3 Normal;
out vec3 FragPos;

layout(binding = 0) uniform vs_params
{
    mat4 mvp; // Proyección * Vista * Modelo
    mat4 model; // Solo Modelo (para la luz)
};

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    // Calculamos la normal corregida (sin traslación)
    Normal = aNormal;
    gl_Position = mvp * vec4(aPos, 1.0);
}
@end

    // --- FRAGMENT SHADER (Ambient + Diffuse) ---
    @fs fs
        in vec3 Normal;
in vec3 FragPos;

layout(binding = 1) uniform fs_params
{
    vec3 objectColor;
    vec3 lightColor;
    vec3 lightPos;
};

out vec4 FragColor;

void main()
{
    // 1. AMBIENT
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // 2. DIFFUSE
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    // Producto punto: Ángulo entre la normal y la luz
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // RESULTADO (Sin Especular)
    vec3 result = (ambient + diffuse) * objectColor;
    FragColor = vec4(result, 1.0);
}
@end

    // --- SHADERS LAMPARA (Igual que antes) ---
    @vs vs_lamp
        in vec3 aPos;
layout(binding = 0) uniform vs_params
{
    mat4 mvp;
    mat4 model;
};
void main()
{
    gl_Position = mvp * vec4(aPos, 1.0);
}
@end

    @fs fs_lamp
        out vec4 FragColor;
void main()
{
    FragColor = vec4(1.0);
}
@end

    @program lighting vs fs
    @program lamp vs_lamp fs_lamp