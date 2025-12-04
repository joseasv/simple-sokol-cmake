// 04_2Texturas.glsl

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

// 'uniforms' para dos texturas y dos samplers
layout(binding=0) uniform texture2D tex1;
layout(binding=1) uniform sampler smp1;
layout(binding=2) uniform texture2D tex2;
layout(binding=3) uniform sampler smp2;

// Color de salida del fragmento
out vec4 frag_color;

void main() {
    // Muestreamos ambas texturas
    vec4 color1 = texture(sampler2D(tex1, smp1), v_uv);
    vec4 color2 = texture(sampler2D(tex2, smp2), v_uv);
    // Mezclamos los colores (50% de cada una)
    frag_color = mix(color1, color2, 0.5);
}
@end

@program dos_texturas vs fs
