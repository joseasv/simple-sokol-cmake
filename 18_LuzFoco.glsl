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

    // --- FRAGMENT SHADER (Spotlight) ---
    @fs fs
        in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

// Uniform Block alineado (96 bytes)
layout(binding = 1) uniform fs_params
{
    vec3 light_position;
    float light_cutOff; // 16 bytes
    vec3 light_direction;
    float light_outerCutOff; // 16 bytes
    vec3 light_ambient;
    float light_constant; // 16 bytes
    vec3 light_diffuse;
    float light_linear; // 16 bytes
    vec3 light_specular;
    float light_quadratic; // 16 bytes
    float mat_shininess; // 4 bytes
};

layout(binding = 0) uniform texture2D material_diffuse;
layout(binding = 1) uniform texture2D material_specular;
layout(binding = 0) uniform sampler smp;

out vec4 FragColor;

void main()
{
    // 0. Datos
    vec3 texColor = texture(sampler2D(material_diffuse, smp), TexCoords).rgb;
    vec3 specMap = vec3(texture(sampler2D(material_specular, smp), TexCoords).r);
    vec3 norm = normalize(Normal);

    // Vectores básicos
    vec3 lightDir = normalize(light_position - FragPos);

    // Cálculo del Theta (ángulo entre dirección de la luz y el vector al fragmento)
    float theta = dot(lightDir, normalize(-light_direction));

    // Componente Ambiental (siempre existe, pero la atenuamos un poco)
    vec3 ambient = light_ambient * texColor;
    // ambient *= attenuation;

    // -------------------------------------------------------------------------
    // OPCIÓN A: BORDE DURO (Hard Edge) - ACTIVADO POR DEFECTO
    // -------------------------------------------------------------------------

    if (theta > light_cutOff) // Recuerda: > porque son cosenos (cerca de 1.0 es centro)
    {
        // Difusa
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = light_diffuse * diff * texColor;

        // Especular
        vec3 viewDir = normalize(light_position - FragPos); // ViewPos es la misma LightPos en linterna
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), mat_shininess);
        vec3 specular = light_specular * spec * specMap;

        FragColor = vec4(ambient + diffuse + specular, 1.0);
    } else {
        // Si está fuera del cono, solo luz ambiental
        FragColor = vec4(ambient, 1.0);
    }

    // -------------------------------------------------------------------------
    // OPCIÓN B: BORDE SUAVE (Soft Edge) - DESCOMENTAR PARA USAR
    // -------------------------------------------------------------------------
    /*
    // 1. Calcular intensidad basada en el borde exterior e interior
    float epsilon   = light_cutOff - light_outerCutOff;
    float intensity = clamp((theta - light_outerCutOff) / epsilon, 0.0, 1.0);

    // 2. Calcular componentes estándar
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light_diffuse * diff * texColor;

    vec3 viewDir = normalize(light_position - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), mat_shininess);
    vec3 specular = light_specular * spec * specMap;

    // 3. Aplicar todo (Atenuación * Intensidad del Spot)
    diffuse  *= intensity;
    specular *= intensity;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
    */
}
@end

    @program lighting vs fs