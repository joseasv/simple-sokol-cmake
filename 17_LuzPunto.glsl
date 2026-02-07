@header const float PI = 3.14159265359;

// --- VERTEX SHADER (Compartido) ---
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

    // --- FRAGMENT SHADER (Iluminación) ---
    @fs fs
        in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

layout(binding = 1) uniform fs_params
{
    vec3 viewPos;
    float light_constant;
    vec3 lightPos;
    float light_linear;
    vec3 light_ambient;
    float light_quadratic;
    vec3 light_diffuse;
    float mat_shininess;
    vec3 light_specular;
    float _pad;
};

layout(binding = 0) uniform texture2D material_diffuse;
layout(binding = 1) uniform texture2D material_specular;
layout(binding = 0) uniform sampler smp;

out vec4 FragColor;

void main()
{
    vec3 texColor = texture(sampler2D(material_diffuse, smp), TexCoords).rgb;
    vec3 specMap = vec3(texture(sampler2D(material_specular, smp), TexCoords).r);
    vec3 norm = normalize(Normal);

    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 ambient = light_ambient * texColor;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light_diffuse * diff * texColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), mat_shininess);
    vec3 specular = light_specular * spec * specMap;

    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (light_constant + light_linear * distance + light_quadratic * (distance * distance));

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
@end

    // --- FRAGMENT SHADER (Lámpara) ---
    @fs fs_lamp
        // CORRECCIÓN: Agregar estas entradas para coincidir con 'vs'
        in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0);
}
@end

    @program lighting vs fs
    @program lamp vs fs_lamp