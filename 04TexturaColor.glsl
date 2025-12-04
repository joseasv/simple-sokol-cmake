// 04_Textura.glsl

@vs vs
// Atributos de vértice: posición y coordenadas de textura (UV)
in vec2 pos;
in vec2 uv;

// 'varying' para pasar las UV al fragment shader
out vec2 v_uv;

void main() {
    gl_Position = vec4(pos, 0.5, 1.0);
    v_uv = uv;
}
@end

@fs fs
// Recibimos las coordenadas de textura interpoladas
in vec2 v_uv;

// 'uniforms' para la textura y el sampler por separado
layout(binding=0) uniform texture2D tex;
layout(binding=1) uniform sampler smp;

// Color de salida del fragmento
out vec4 frag_color;

void main() {
    // Combinamos textura y sampler para muestrear el color
    frag_color = texture(sampler2D(tex, smp), v_uv);
}
@end

@program textura vs fs
