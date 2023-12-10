#version 330

uniform float threshold = 1.0;
uniform ivec2 resolution = ivec2(2560, 1440);
uniform sampler2D hdrBuffer;

out vec4 color;

void main()
{             
    vec2 fragCoord = vec2( gl_FragCoord.x / resolution.x, gl_FragCoord.y / resolution.y );
    vec3 hdrColor = texture(hdrBuffer, fragCoord).rgb;

    // This brightness calculation accounts for how the human eye perceives color; 
    // greens are more heavily weighted than blues.
    // See: https://learnopengl.com/Advanced-Lighting/Bloom
    float brightness = dot(hdrColor, vec3(0.2126, 0.7152, 0.0722));
    color = vec4(step(threshold, brightness) * hdrColor, 1.0);
}