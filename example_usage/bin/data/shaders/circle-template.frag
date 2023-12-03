#version 410

const float PI = 3.1415926535;

uniform ivec2 resolution = ivec2(2560, 1440);

out vec4 outputColor;

void main() {
    vec4 color = vec4(0.0);
    vec2 aspectRatio = vec2(resolution) / min(resolution.x, resolution.y);
    vec2 fragCoord = vec2( gl_FragCoord.x / resolution.x, gl_FragCoord.y / resolution.y );

    vec2 center = vec2(0.5);
    float distToCenter = distance(fragCoord * aspectRatio, center * aspectRatio);
    color = vec4(vec3(step(distToCenter, 0.4)), 1.0);

    outputColor = color;
}