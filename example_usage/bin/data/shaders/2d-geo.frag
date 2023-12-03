#version 410

const float PI = 3.1415926535;

uniform uvec2 resolution = uvec2(2560, 1440);

uniform float divisions = 1.0;
uniform float warpX = 2.0;
uniform float warpY = 2.0;

uniform int warpIterations = 4;

out vec4 outputColor;

void main() {
    vec4 color = vec4(0.0);
    vec2 aspectRatio = vec2(resolution) / max(resolution.x, resolution.y);
    vec2 fragCoord = vec2( gl_FragCoord.x / resolution.x, gl_FragCoord.y / resolution.y );
    // fragCoord = fragCoord * aspectRatio;

    fragCoord = mod(fragCoord * divisions, 1.0);

    /* squiggly
    for (int i = 0; i < warpIterations; i++) {
        fragCoord.x = fragCoord.x + sin(fragCoord.y * warpX * PI);
        fragCoord.y = fragCoord.y + cos(fragCoord.x * warpY * PI);
    }
    */

    float r = cos(fragCoord.x * 4 * PI) / 2.0 + 0.5;
    float g = sin(PI/2 - fragCoord.y * 2 * PI) / 2.0 + 0.5;

    // color.r = ;
    // color.g = ;
    // color.b = 1.0;
    color.rgb = vec3(fragCoord.x, fragCoord.y, (fragCoord.x + fragCoord.y) / 2);
    color.a = 1.0;

    outputColor = color;
}