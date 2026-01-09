#version 150
in vec2 aPosition;
in float aLife;

uniform float uPointSize;

out float vLife;

void main()
{
    vLife = aLife;
    gl_Position = vec4(aPosition, 0.0, 1.0);
    gl_PointSize = uPointSize;
}
