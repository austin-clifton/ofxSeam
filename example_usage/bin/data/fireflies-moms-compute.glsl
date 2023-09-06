#version 450

const float PI = 3.1415926535; 

uniform float maxAcceleration = 0.05;
uniform float maxVelocity = 0.02;
uniform float massMultiplier = 1.0;
uniform float velocityDecay = 0.99;
uniform float time = 0.0;

struct Mom {
    float mass;
    vec3 velocity;
    vec3 colA;
    vec3 colB;
    vec3 colC;
    vec3 colD;
};

layout(std140, binding=5) buffer mom_positions_in {
    vec4 in_positions[];
};

layout(std140, binding=6) buffer mom_directions_in {
    vec4 in_directions[];
};

layout(std140, binding=7) buffer mom_positions_out {
    vec4 out_positions[];
};

layout(std140, binding=8) buffer mom_particles_in {
    Mom in_moms[];
};

layout(std140, binding=9) buffer mom_particles_out {
    Mom out_moms[];
};

vec2 hash( vec2 p ) // replace this by something better
{
	p = vec2( dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)) );
	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float noise( in vec2 p )
{
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;

	vec2  i = floor( p + (p.x+p.y)*K1 );
    vec2  a = p - i + (i.x+i.y)*K2;
    float m = step(a.y,a.x); 
    vec2  o = vec2(m,1.0-m);
    vec2  b = a - o + K2;
	vec2  c = a - 1.0 + 2.0*K2;
    vec3  h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
	vec3  n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));
    return dot( n, vec3(70.0) );
}

// https://iquilezles.org/articles/fbm/
float fbm( in vec2 x, in float H, in int octaves )
{    
    float G = exp2(-H);
    float f = 1.0;
    float a = 1.0;
    float t = 0.0;
    for( int i=0; i<octaves; i++ )
    {
        t += a*noise(f*x);
        f *= 2.0;
        a *= G;
    }
    return t;
}

layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;
void main() {
    Mom momRead = in_moms[gl_GlobalInvocationID.x];
    Mom momWrite = in_moms[gl_GlobalInvocationID.x];

    vec3 pos = in_positions[gl_GlobalInvocationID.x].xyz;
    vec3 avoidance_dir = in_directions[gl_GlobalInvocationID.x].xyz;
    vec3 to_center = normalize(vec3(0.0) - pos);
    float dist_to_center = distance(vec3(0.0), pos);

    // The strength of moving towards the center is dependent on how far from center the mom is.
    float center_strength = pow(smoothstep(50.0, 100.0, dist_to_center), 2);

    // Moms also move around on their own using FBM.
    float fbmY = gl_GlobalInvocationID.x * 1.84;
    vec3 fbmForce = vec3(0.0);
    fbmForce.x = fbm(pos.xz + time + fbmY, 0.5, 4);
    fbmForce.y = fbm(pos.xz + time + fbmY + 197.872, 0.5, 4);
    fbmForce.z = fbm(pos.xz + time + fbmY - 71.234, 0.5, 4);

    // Add weighted forces together.
    vec3 force = normalize(avoidance_dir 
        + fbmForce * 0.5 
        + to_center * center_strength) 
        * maxAcceleration;

    // vec3 force = normalize(fbmForce) * maxAcceleration;

    // Calculate new particle values and write them.
    vec3 acceleration = force / (momRead.mass * massMultiplier);
    vec3 velocity = max(min(momRead.velocity * velocityDecay + acceleration, maxVelocity), -maxVelocity);

    momWrite.velocity = velocity;

    out_positions[gl_GlobalInvocationID.x] = vec4(pos + velocity, 1.0);
    out_moms[gl_GlobalInvocationID.x] = momWrite;
}