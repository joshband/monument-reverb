#version 150
in vec2 vUv;
out vec4 fragColor;

uniform float uTime;
uniform float uLevel;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

void main()
{
    vec2 uv = vUv;
    float t = uTime * 0.2;
    float n = noise(uv * 3.0 + t);
    float waves = sin((uv.x + n) * 10.0 + uTime) * 0.5 + 0.5;
    float intensity = mix(0.2, 1.2, clamp(uLevel, 0.0, 1.0));

    vec3 base = vec3(0.08, 0.32, 0.55);
    vec3 color = base + vec3(0.2, 0.4, 0.6) * (waves + n) * intensity;
    fragColor = vec4(color, 1.0);
}
