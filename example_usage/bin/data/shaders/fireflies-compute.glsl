#version 450

const float PI = 3.1415926535; 

uniform float maxAcceleration = 0.05;
uniform float maxVelocity = 0.35;
uniform float massMultiplier = 1.0;
uniform float velocityDecay = 0.99;
uniform float time = 0.0;

uniform uint momsCount = 9;

struct Firefly {
    float mass;
    float luminance;
    int effect_index;
    int group_index;

    vec4 color;
    
    vec3 velocity;
    float unused;
};

struct Mom {
    float mass;
    vec3 velocity;
    vec3 colA;
    vec3 colB;
    vec3 colC;
    vec3 colD;
};

layout(std140, binding=0) buffer ff_positions_in {
    vec4 in_positions[];
};

layout(std140, binding=1) buffer ff_directions_in {
    vec4 in_directions[];
};

layout(std140, binding=2) buffer ff_positions_out {
    vec4 out_positions[];
};

layout(std140, binding=3) buffer ff_particles_in {
    Firefly in_ff[];
};

layout(std140, binding=4) buffer ff_particles_out {
    Firefly out_ff[];
};

layout(std140, binding=5) buffer mom_positions_in {
    vec4 in_mom_positions[];
};

layout(std140, binding=8) buffer mom_particles_in {
    Mom in_moms[];
};


// cosine based palette, 4 vec3 params
// thanks iq!
vec3 palette( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
}

layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;
void main() {

    Firefly ff_read = in_ff[gl_GlobalInvocationID.x];
    Firefly ff_write = in_ff[gl_GlobalInvocationID.x];
    uint mom_index = gl_GlobalInvocationID.x % momsCount;

    vec3 pos = in_positions[gl_GlobalInvocationID.x].xyz;
    vec3 avoidance_dir = in_directions[gl_GlobalInvocationID.x].xyz;
    Mom mom = in_moms[mom_index];
    vec3 mom_pos = in_mom_positions[mom_index].xyz;
    vec3 to_center = normalize(mom_pos - pos);
    float dist_to_center = distance(mom_pos, pos);

    // The strength of moving towards the center is dependent on how far from center the mom is.
    float center_strength = pow(smoothstep(30.0, 100.0, dist_to_center), 2);

    // Add weighted forces together.
    // vec3 force = avoidance_dir * maxAcceleration;
    vec3 force = normalize(avoidance_dir + to_center * 0.25) * maxAcceleration;

    // Calculate new particle values and write them.
    vec3 acceleration = force / (ff_read.mass * massMultiplier);
    vec3 velocity = max(min(ff_read.velocity * velocityDecay + acceleration, maxVelocity), -maxVelocity);

    /*
    float xMask = dot(velocity, vec3(1.0, 0.0, 0.0));
    float yMask = dot(velocity, vec3(0.0, 1.0, 0.0));
    float zMask = dot(velocity, vec3(0.0, 0.0, 1.0));
    float axisMask = max(abs(xMask), max(abs(yMask), abs(zMask)));

    velocity = vec3(
        velocity.x * float(axisMask == abs(xMask)),
        velocity.y * float(axisMask == abs(yMask)),
        velocity.z * float(axisMask == abs(zMask))
    );
    */

    ff_write.velocity = velocity;
    ff_write.color = vec4(palette(time + float(mom_index) / float(momsCount), mom.colA, mom.colB, mom.colC, mom.colD), 1.0);

    out_positions[gl_GlobalInvocationID.x] = vec4(pos + velocity, 1.0);
    out_ff[gl_GlobalInvocationID.x] = ff_write;
}