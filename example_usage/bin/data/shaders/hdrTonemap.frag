#version 330

uniform float gamma = 2.2;
uniform ivec2 resolution = ivec2(2560, 1440);

uniform sampler2D hdrBuffer;

uniform sampler2D blurTex0;
uniform sampler2D blurTex1;
uniform sampler2D blurTex2;
uniform sampler2D blurTex3;
uniform sampler2D blurTex4;

out vec4 color;

// ACES Filmic tonemap approximation
// See: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESApprox(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), vec3(0), vec3(1));
}

void main()
{             
    vec2 fragCoord = vec2( gl_FragCoord.x / resolution.x, gl_FragCoord.y / resolution.y );
    vec3 hdrColor = texture(hdrBuffer, fragCoord).rgb;

    vec3 bloom = texture(blurTex0, fragCoord).rgb;
    bloom += texture(blurTex1, fragCoord).rgb;
    bloom += texture(blurTex2, fragCoord).rgb;
    bloom += texture(blurTex3, fragCoord).rgb;
    bloom += texture(blurTex4, fragCoord).rgb;
    bloom = bloom / 5.0;

    hdrColor += bloom;

    vec3 tonemapped = ACESApprox(hdrColor);
    // gamma correction 
    tonemapped = pow(tonemapped, vec3(1.0 / gamma));
  
    color = vec4(tonemapped, 1.0);
}  