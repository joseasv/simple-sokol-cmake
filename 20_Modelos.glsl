@header const float PI = 3.14159265359;

// --- VERTEX SHADER ---
@vs vs
    in vec3 aPos;
in vec3 aNormal;
in vec2 aTexCoords;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

layout(binding = 0) uniform vs_params
{
    mat4 mvp;
    mat4 model;
};

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    gl_Position = mvp * vec4(aPos, 1.0);
}
@end

    // --- FRAGMENT SHADER ---
    @fs fs
        in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

layout(binding = 1) uniform fs_params
{
    vec3 viewPos;
    vec3 lightPos;
    vec3 lightColor;
};

// Nombres coinciden con lo que asignamos en la clase Mesh
// Slot 0 = Diffuse, Slot 1 = Specular
layout(binding = 0) uniform texture2D texture_diffuse1;
layout(binding = 1) uniform texture2D texture_specular1;
layout(binding = 0) uniform sampler smp;

out vec4 FragColor;

void main()
{
    // 1. Obtener colores de las texturas
    vec3 colorDiff = texture(sampler2D(texture_diffuse1, smp), TexCoords).rgb;

    // CORRECCIÓN AQUÍ: Envolver la lectura del canal .r en un vec3()
    vec3 colorSpec = vec3(texture(sampler2D(texture_specular1, smp), TexCoords).r);

    // 2. Iluminación Básica (Blinn-Phong)
    vec3 ambient = 0.1 * colorDiff;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * colorDiff;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = vec3(0.5) * spec * colorSpec;

    vec3 result = (ambient + diffuse + specular) * lightColor;
    FragColor = vec4(result, 1.0);
}
@end

    @program lighting vs fs