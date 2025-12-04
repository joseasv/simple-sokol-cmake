// 04TexturaColor.glsl

@vs vs
// Atributos de vértice: posición, coordenadas de textura (UV) y color
in vec2 pos;
in vec2 uv;
in vec3 color; // Nuevo atributo de color

// 'varyings' para pasar datos al fragment shader
out vec2 v_uv;
out vec3 v_color; // Nuevo varying para el color

void main() {
    gl_Position = vec4(pos, 0.5, 1.0);
    v_uv = uv;
    v_color = color; // Pasamos el color al fragment shader
}
@end

@fs fs
// Recibimos los datos interpolados
in vec2 v_uv;
in vec3 v_color; // Recibimos el color interpolado

// 'uniforms' para la textura y el sampler
layout(binding=0) uniform texture2D tex;
layout(binding=1) uniform sampler smp;

// Color de salida del fragmento
out vec4 frag_color;

void main() {
    // Combinamos textura y sampler, y multiplicamos por el color del vértice
    frag_color = texture(sampler2D(tex, smp), v_uv) * vec4(v_color, 1.0);
}
@end

@program textura_color vs fs
