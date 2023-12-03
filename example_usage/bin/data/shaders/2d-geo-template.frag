#version 410

const float PI = 3.1415926535;

uniform uvec2 resolution = uvec2(2560, 1440);

out vec4 outputColor;

void main() {
    vec4 color = vec4(0.0);
    vec2 aspectRatio = vec2(resolution) / max(resolution.x, resolution.y);
    vec2 fragCoord = vec2( gl_FragCoord.x / resolution.x, gl_FragCoord.y / resolution.y );
    fragCoord = fragCoord * aspectRatio;

    color.r = fragCoord.x;
    color.g = fragCoord.y;
    color.a = 1.0;

    outputColor = color;
}