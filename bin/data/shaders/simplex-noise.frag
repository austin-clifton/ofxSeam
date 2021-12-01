#version 410

uniform ivec2 resolution = ivec2(1920, 1080);

// number of noise iterations
uniform int octaves = 8;

// frequency multiplier per octave
uniform float lacunarity = 2.0;

uniform float frequency = 4.0;

// power/strength multiplier per octave
uniform float persistence = 0.5;

uniform float amplitude = 0.7;

out vec4 outputColor;

float rand(vec2 n) { 
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

// GLSL noise
// https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
float noise(vec2 p) {
	vec2 ip = floor(p);
	vec2 u = fract(p);
	u = u*u*(3.0-2.0*u);
	
	float res = mix(
		mix(rand(ip),rand(ip+vec2(1.0,0.0)),u.x),
		mix(rand(ip+vec2(0.0,1.0)),rand(ip+vec2(1.0,1.0)),u.x),u.y);
	return res*res;
}

void main() {
    vec4 color = vec4(0.0);
    vec2 fragCoord = vec2( gl_FragCoord.x / resolution.x, gl_FragCoord.y / resolution.y );
    float curr_lacunarity = frequency;
    float curr_gain = amplitude;
    for ( int i = 0; i < octaves; i++ ) {
        vec2 uv = fragCoord * curr_lacunarity;
        color += vec4(
            noise( uv ),
            noise( vec2(17.3, -8.1) + uv ),
            noise( vec2(-4.7, 23.6) + uv ),
            0.0
        ) * curr_gain;

        curr_gain *= persistence;
        curr_lacunarity *= lacunarity;
    }
    color.a = 1.0;

    outputColor = color;
}