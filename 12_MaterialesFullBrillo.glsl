// --- VERTEX SHADER ---
@vs vs
    in vec3 aPos;
in vec3 aNormal;

out vec3 Normal;
out vec3 FragPos;

layout(binding = 0) uniform vs_params
{
    mat4 mvp;
    mat4 model;
};

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    // Inversa de la transpuesta para las normales
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = mvp * vec4(aPos, 1.0);
}
@end

    // --- FRAGMENT SHADER ---
    @fs fs
        in vec3 Normal;
in vec3 FragPos;

layout(binding = 1) uniform fs_params
{
    // Escena
    vec3 lightPos;
    vec3 viewPos;
    vec3 lightColor;

    // Material (Struct simulado)
    vec3 mat_ambient;
    vec3 mat_diffuse;
    vec3 mat_specular;
    float mat_shininess;
};

out vec4 FragColor;

void main()
{
    // 1. AMBIENTE
    vec3 ambient = lightColor * mat_ambient;

    // 2. DIFUSA
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = lightColor * (diff * mat_diffuse);

    // 3. ESPECULAR
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), mat_shininess);
    vec3 specular = lightColor * (spec * mat_specular);

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
@end

    @vs vs_lamp
        in vec3 aPos;
layout(binding = 0) uniform vs_params
{
    mat4 mvp;
    mat4 model; // No se usa aquí, pero mantenemos el bloque compatible
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
    FragColor = vec4(1.0); // Blanco puro
}
@end

    @program lighting vs fs
    @program lamp vs_lamp fs_lamp // (Usa el shader de lámpara simple anterior o un básico)