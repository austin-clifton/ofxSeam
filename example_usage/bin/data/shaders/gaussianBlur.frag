#version 330

uniform bool horizontal = false;
uniform float weights[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
uniform ivec2 resolution = ivec2(2560, 1440);
uniform sampler2D tex0;

out vec4 color;

void main()
{
    vec2 texOffset = 1.0 / textureSize(tex0, 0);            
    vec2 fragCoord = vec2( gl_FragCoord.x / resolution.x, gl_FragCoord.y / resolution.y );
    vec3 texColor = texture(tex0, fragCoord).rgb;

    vec3 blended = texture(tex0, fragCoord).rgb * weights[0];

    for (int i = 1; i < 5; i++) {
        vec2 offset = vec2(texOffset.x * float(horizontal), texOffset.y * (1.0 - float(horizontal))) * i;
        blended += texture(tex0, fragCoord + offset).rgb * weights[i];
        blended += texture(tex0, fragCoord - offset).rgb * weights[i];
    }

    color = vec4(blended, 1.0);
}  