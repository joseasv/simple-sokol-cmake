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

    // --- FRAGMENT SHADER (PRINCIPAL) ---
    @fs fs
        in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

// Estructuras internas para cálculo (NO Uniform Blocks)
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// UNIFORM BLOCK APLANADO (Compatible con Sokol/C++)
layout(binding = 1) uniform fs_params
{
    vec3 viewPos;
    float mat_shininess;

    // DirLight
    vec3 dir_direction;
    vec3 dir_ambient;
    vec3 dir_diffuse;
    vec3 dir_specular;

    // PointLights (Arrays de vec4 para alineación)
    vec4 pl_pos_const[4]; // xyz:pos, w:constant
    vec4 pl_att[4]; // x:linear, y:quadratic
    vec4 pl_ambient[4]; // rgb:color
    vec4 pl_diffuse[4];
    vec4 pl_specular[4];

    // SpotLight
    vec3 spot_position;
    vec3 spot_direction;
    vec3 spot_ambient;
    vec3 spot_diffuse;
    vec3 spot_specular;
    vec3 spot_att; // x:const, y:lin, z:quad
    vec2 spot_angles; // x:cut, y:outerCut
};

layout(binding = 0) uniform texture2D material_diffuse;
layout(binding = 1) uniform texture2D material_specular;
layout(binding = 0) uniform sampler smp;

out vec4 FragColor;

// Prototipos
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    // Reconstrucción de structs
    DirLight dirL;
    dirL.direction = dir_direction;
    dirL.ambient = dir_ambient;
    dirL.diffuse = dir_diffuse;
    dirL.specular = dir_specular;

    SpotLight spotL;
    spotL.position = spot_position;
    spotL.direction = spot_direction;
    spotL.ambient = spot_ambient;
    spotL.diffuse = spot_diffuse;
    spotL.specular = spot_specular;
    spotL.constant = spot_att.x;
    spotL.linear = spot_att.y;
    spotL.quadratic = spot_att.z;
    spotL.cutOff = spot_angles.x;
    spotL.outerCutOff = spot_angles.y;

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = vec3(0.0);

    result += CalcDirLight(dirL, norm, viewDir);

    for (int i = 0; i < 4; i++) {
        PointLight pl;
        pl.position = pl_pos_const[i].xyz;
        pl.constant = pl_pos_const[i].w;
        pl.linear = pl_att[i].x;
        pl.quadratic = pl_att[i].y;
        pl.ambient = pl_ambient[i].rgb;
        pl.diffuse = pl_diffuse[i].rgb;
        pl.specular = pl_specular[i].rgb;
        result += CalcPointLight(pl, norm, FragPos, viewDir);
    }

    result += CalcSpotLight(spotL, norm, FragPos, viewDir);
    FragColor = vec4(result, 1.0);
}

// Implementación de funciones de luz
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), mat_shininess);

    vec3 ambient = light.ambient * vec3(texture(sampler2D(material_diffuse, smp), TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(sampler2D(material_diffuse, smp), TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(sampler2D(material_specular, smp), TexCoords).r);
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), mat_shininess);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec3 ambient = light.ambient * vec3(texture(sampler2D(material_diffuse, smp), TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(sampler2D(material_diffuse, smp), TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(sampler2D(material_specular, smp), TexCoords).r);

    return (ambient * attenuation) + (diffuse * attenuation) + (specular * attenuation);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), mat_shininess);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 ambient = light.ambient * vec3(texture(sampler2D(material_diffuse, smp), TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(sampler2D(material_diffuse, smp), TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(sampler2D(material_specular, smp), TexCoords).r);

    return (ambient * attenuation) + (diffuse * attenuation * intensity) + (specular * attenuation * intensity);
}
@end

    // --- FRAGMENT SHADER (LÁMPARA) ---
    // AQUÍ ESTABA EL ERROR: Faltaban las entradas que envía el VS
    @fs fs_lamp
        in vec3 Normal; // <--- Agregado (aunque no se use)
in vec3 FragPos; // <--- Agregado
in vec2 TexCoords; // <--- Agregado

layout(binding = 1) uniform fs_params_lamp
{
    vec3 light_color;
};

out vec4 FragColor;

void main()
{
    FragColor = vec4(light_color, 1.0);
}
@end

    @program lighting vs fs
    @program lamp vs fs_lamp