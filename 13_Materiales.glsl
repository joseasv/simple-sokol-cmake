// -----------------------------------------------------------------------------
// 13_Materiales.glsl
// -----------------------------------------------------------------------------

// --- VERTEX SHADER (Objeto) ---
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
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = mvp * vec4(aPos, 1.0);
}
@end

    // --- FRAGMENT SHADER (Objeto con Material + Propiedades Luz) ---
    @fs fs
        in vec3 Normal;
in vec3 FragPos;

layout(binding = 1) uniform fs_params
{
    vec3 viewPos;
    vec3 lightPos;

    // Propiedades de la LUZ
    vec3 light_ambient;
    vec3 light_diffuse;
    vec3 light_specular;

    // Propiedades del MATERIAL
    vec3 mat_ambient;
    vec3 mat_diffuse;
    vec3 mat_specular;
    float mat_shininess;
};

out vec4 FragColor;

void main()
{
    // 1. AMBIENTE: luz.ambient * material.ambient
    vec3 ambient = light_ambient * mat_ambient;

    // 2. DIFUSA: luz.diffuse * (dot * material.diffuse)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light_diffuse * (diff * mat_diffuse);

    // 3. ESPECULAR: luz.specular * (spec * material.specular)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), mat_shininess);
    vec3 specular = light_specular * (spec * mat_specular);

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
@end

    // --- SHADERS LÁMPARA (Cubo de luz) ---
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