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

    @fs fs
        in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

layout(binding = 1) uniform fs_params
{
    vec3 viewPos;
    vec3 lightPos;
    vec3 light_ambient;
    vec3 light_diffuse;
    vec3 light_specular;
    // mat_specular ELIMINADO de aquí (ahora es textura)
    float mat_shininess;
};

// Texturas y Sampler
layout(binding = 0) uniform texture2D material_diffuse;
layout(binding = 1) uniform texture2D material_specular;
layout(binding = 0) uniform sampler smp;

out vec4 FragColor;

void main()
{
    // 1. AMBIENTE
    vec3 texColor = texture(sampler2D(material_diffuse, smp), TexCoords).rgb;
    vec3 ambient = light_ambient * texColor;

    // 2. DIFUSA
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light_diffuse * diff * texColor;

    // 3. ESPECULAR
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), mat_shininess);

    // Leemos el mapa especular (canal rojo 'r')
    vec3 specMap = vec3(texture(sampler2D(material_specular, smp), TexCoords).r);

    vec3 specular = light_specular * spec * specMap;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
@end

    @vs vs_lamp
        in vec3 aPos;
layout(binding = 0) uniform vs_params
{
    mat4 mvp;
    mat4 model;
};
void main() { gl_Position = mvp * vec4(aPos, 1.0); }
@end

    @fs fs_lamp
        out vec4 FragColor;
void main() { FragColor = vec4(1.0); }
@end

    @program lighting vs fs
    @program lamp vs_lamp fs_lamp