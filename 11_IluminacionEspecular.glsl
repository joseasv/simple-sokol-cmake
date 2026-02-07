// --- VERTEX SHADER ---
@vs vs
    in vec3 aPos;
in vec3 aNormal;

out vec3 Normal;
out vec3 FragPos;

// Uniforms del Vertex Shader
layout(binding = 0) uniform vs_params
{
    mat4 mvp; // Proyección * Vista * Modelo (Optimizado en CPU)
    mat4 model; // Matriz de Modelo pura (Para cálculos de luz)
};

void main()
{
    // 1. Calculamos la posición del fragmento en el "Mundo"
    FragPos = vec3(model * vec4(aPos, 1.0));

    // 2. Calculamos la Normal corregida.
    // Usamos la inversa de la transpuesta para manejar escalas no uniformes correctamente.
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // 3. Posición final en pantalla (Clip Space)
    gl_Position = mvp * vec4(aPos, 1.0);
}
@end

    // --- FRAGMENT SHADER (Modelo Phong) ---
    @fs fs
        in vec3 Normal;
in vec3 FragPos;

// Uniforms del Fragment Shader
layout(binding = 1) uniform fs_params
{
    vec3 objectColor;
    vec3 lightColor;
    vec3 lightPos;
    vec3 viewPos;
};

out vec4 FragColor;

void main()
{
    // A. AMBIENT (Luz base)
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // B. DIFFUSE (Luz direccional)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    // Producto punto: si es 1.0 miran igual, si es 0.0 son perpendiculares
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // C. SPECULAR (Brillo/Reflejo)
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);

    // reflect espera: reflect(incidente, normal). La luz incidente es -lightDir
    vec3 reflectDir = reflect(-lightDir, norm);

    // 32 es el Shininess (Brillo). Más alto = punto más concentrado.
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 256);
    vec3 specular = specularStrength * spec * lightColor;

    // RESULTADO FINAL
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
@end

    // --- SHADERS PARA LA LÁMPARA (Solo color blanco) ---
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

    // --- PROGRAMAS ---
    @program lighting vs fs
    @program lamp vs_lamp fs_lamp