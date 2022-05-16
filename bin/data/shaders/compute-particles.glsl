#version 440

const float PI = 3.1415926535;

struct Particle{
	vec4 pos;
	vec4 vel;
	vec4 color;
	float theta;
	float mass;
	float unused1;
	float unused2;
};

layout(std140, binding=0) buffer particle{
    Particle p[];
};

layout(std140, binding=1) buffer particleBack{
    Particle p2[];
};

uniform float timeLastFrame = 0.f;
uniform float elapsedTime = 0.f;
uniform vec3 attractor1 = vec3(800.f, 0.f, 5000.f);
uniform vec3 attractor2 = vec3(400.f, 10.f, 5000.f);
uniform vec3 attractor3 = vec3(1200.f, 0.f, 5000.f);
uniform float attraction = 0.18f;
uniform float cohesion = 0.05f;
uniform float repulsion = 0.7f;
uniform float max_speed = 2500.0f;
uniform float attr1_force = 800.f;
uniform float attr2_force = 800.f;
uniform float attr3_force = 1200.f;

uniform float torus_radius = 500.0f;
uniform float torus_thickness = 50.0f;

// TODO: this should map to an array of note forces
uniform float angular_velocity = 1.0f;
uniform float torus_gravity = 1.0f;

// https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}

float noise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}

// https://iquilezles.org/articles/fbm/
float fbm( in vec3 x, in float H, in int octaves )
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
void main(){
	// grab the read and write particles
	Particle pr = p2[gl_GlobalInvocationID.x];
	Particle pw = p2[gl_GlobalInvocationID.x];

	const float mass_inverse = 1 / pr.mass;

	const vec3 torus_center = vec3(0);
	const vec3 up = vec3(0,1,0);
	const float max_vel = 4.f * mass_inverse;

	// calculate the forward and right vectors given the current theta
	const vec3 base_pos_in_torus = torus_center + torus_radius * vec3(cos(pr.theta), 0, sin(pr.theta));
	const vec3 to_center = normalize(torus_center - base_pos_in_torus);
	const vec3 forward = cross(to_center, up);
	const vec3 right = cross(forward, up);

	vec3 force = vec3(0.0);

	const int layers = 8;

	const float fbm_offset = (gl_GlobalInvocationID.x % layers) * 113.84f
		+ (gl_GlobalInvocationID.x % layers) / layers * 2.f;

	vec3 force_fbm;
	vec3 normalized_pos = normalize(pr.pos.xyz);
	float nt = elapsedTime * 0.1 + fbm_offset;
	force_fbm.x = fbm(normalized_pos + nt, 0.5, 4);
	force_fbm.y = fbm(force_fbm.x + normalized_pos + nt, 0.5, 4);
	force_fbm.z = fbm(force_fbm.y + normalized_pos + nt, 0.5, 4);
	force_fbm = ( force_fbm - 1.1 ) * 2;
	force_fbm *= 3.f;

	// add forward force = mass (size) * angular acceleration
	force += forward * mass_inverse * angular_velocity;

	// add torus gravitational pull towards center
	force += to_center * mass_inverse * torus_gravity;

	// add offset forces with fbm for current position
	force += right * force_fbm.y * mass_inverse;
	force += up * force_fbm.z * mass_inverse;

	// apply some force back towards the particle's center position within the torus,
	// dependent on how far away from center the particle is
	float b2c_factor = distance(pr.pos.xyz, base_pos_in_torus) / torus_thickness;
	// calculate the direction back towards the torus's center
	vec3 back_to_center = normalize(base_pos_in_torus - pw.pos.xyz);
	force += back_to_center * mass_inverse * b2c_factor;


	/*
	// push the particle back into the torus if it leaves its radius within the torus
	if (distance(pr.pos.xyz, base_pos_in_torus) > torus_thickness) {
		
		force += back_to_center * mass_inverse * 2.f;



		// move to the opposite edge of the torus at theta
		// this looks kinda... bad
		// vec3 opposite_edge_dir = normalize(base_pos_in_torus - pw.pos.xyz);
		// vec3 opposite_edge = base_pos_in_torus + opposite_edge_dir * torus_thickness;
		// pw.pos.xyz = opposite_edge;
	}
	*/

	// calculate new velocity
	pw.vel.xyz += force;
	// incorporate some drag so things don't speed up forever
	pw.vel.xyz *= 0.98;
	if (length(pw.vel.xyz) > max_vel) {
		pw.vel.xyz = normalize(pw.vel.xyz) * max_vel;
	}

	// calculate new position
	pw.pos.xyz += pr.vel.xyz;

	// calculate and store new theta
	vec3 normalized_dir = normalize(pw.pos.xyz - torus_center);
	// pw.theta = pw.theta + 0.0001 > 1.0 ? 0.0 : pw.theta + 0.0001;
	pw.theta = atan(normalized_dir.z, normalized_dir.x);

	// pw.pos = pr.pos + 0.0001f;

	p[gl_GlobalInvocationID.x] = pw;

	return;

	/*
	vec3 acc = vec3(0.0,0.0,0.0);
	uint m = uint(1024.0*8.0*elapsedTime);
	uint start = m%(1024*8-512);
	uint end = start + 512;
	float minDist;
	uint first = 1;
	for(uint i=start;i<end;i++){
		if(i!=gl_GlobalInvocationID.x){
			acc += rule1(particle,p2[i].pos.xyz) * repulsion;
			acc += rule2(particle,p2[i].pos.xyz, p2[gl_GlobalInvocationID.x].vel.xyz, p2[i].vel.xyz) * cohesion;
			acc += rule3(particle,p2[i].pos.xyz) * attraction;
		}
	}
	
	p[gl_GlobalInvocationID.x].pos.xyz += p[gl_GlobalInvocationID.x].vel.xyz*timeLastFrame;
	
	if(gl_GlobalInvocationID.x%2==1){
		vec3 dir = attractor1 - particle;
		acc += normalize(dir)*attr1_force;
	}
	
	
	if(gl_GlobalInvocationID.x%2==0){
		vec3 dir = attractor2 - particle;
		acc += normalize(dir)*attr2_force;
	}
	
	vec3 dir = attractor3 - particle;
	acc += normalize(dir)*attr3_force;
	
	p[gl_GlobalInvocationID.x].vel.xyz += acc*timeLastFrame;
	p[gl_GlobalInvocationID.x].vel.xyz *= 0.99;
	
	dir = normalize(p[gl_GlobalInvocationID.x].vel.xyz);
	
	if(length(p[gl_GlobalInvocationID.x].vel.xyz)>max_speed){
		p[gl_GlobalInvocationID.x].vel.xyz = dir * max_speed;
	}
	
	float max_component = max(max(dir.x,dir.y),dir.z);
	p[gl_GlobalInvocationID.x].color.rgb = dir/max_component;
	p[gl_GlobalInvocationID.x].color.a = 0.4;
	*/
}