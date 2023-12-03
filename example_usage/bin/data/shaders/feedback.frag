#version 450

uniform ivec2 resolution = ivec2(1920, 1080);

uniform sampler2D tex0;
uniform sampler2D imgTex;

uniform float decay = 0.05;
uniform vec4 filterColor = vec4(1.0);
uniform vec2 feedbackOffset = vec2(0.0);

out vec4 fragColor;

void main (void)
{
    vec2 aspectRatio = vec2(resolution) / max(resolution.x, resolution.y);
    vec2 fragCoord = vec2(
        (gl_FragCoord.x + feedbackOffset.x) / resolution.x, 
        (gl_FragCoord.y + feedbackOffset.y) / resolution.y
    );

    vec4 feedbackColor = texture(tex0, fragCoord) * filterColor;
    vec4 imgColor = texture(imgTex, fragCoord);

    fragColor = imgColor + feedbackColor * (1.0 - decay);
}