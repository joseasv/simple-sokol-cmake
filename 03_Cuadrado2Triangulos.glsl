// Bloque del Vertex Shader
@vs vs
    in vec4 position;
in vec4 color0;
out vec4 color;

void main()
{
    gl_Position = position;
    color = color0;
}
@end

    // Bloque del Fragment Shader
    @fs fs
        in vec4 color;
out vec4 frag_color;

void main()
{
    frag_color = color;
}
@end

    // Definición del Programa (une vs y fs)
    @program cuadrado vs fs