#version 150
in float vLife;
out vec4 fragColor;

void main()
{
    float dist = length(gl_PointCoord - vec2(0.5));
    float alpha = smoothstep(0.5, 0.0, dist) * clamp(vLife, 0.0, 1.0);
    fragColor = vec4(1.0, 1.0, 1.0, alpha);
}
