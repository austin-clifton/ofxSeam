#version 440

const float PI = 3.1415926535;

struct Particle{
	vec4 pos;
	vec4 vel;
	vec4 color;
	float theta;
	float mass;
	float size;
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

uniform float torus_radius = 500.0f;
uniform float torus_thickness = 50.0f;
uniform vec3 torus_center = vec3(0);

// TODO: this should map to an array of note forces
uniform float angular_velocity = 1.0f;
uniform float torus_gravity = 1.0f;

uniform float global_fbm_offset = 0.0f;

uniform float camera_theta = 0;
uniform float camera_theta_tolerance = PI/4;

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


vec3 CalculateBasePosInTorus(float theta) {
	return torus_center + torus_radius * vec3(cos(theta), 0, sin(theta));
}

// smallest distance between two angles on range [-PI, PI]
// https://stackoverflow.com/a/7869457
float AngleBetween(float a, float b) {
	float d = a - b;
	return d + ( (d > PI) ? -2 * PI
		: (d < -PI) ? 2 * PI
		: 0 );
}

layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;
void main(){
	// grab the read and write particles
	Particle pr = p2[gl_GlobalInvocationID.x];
	Particle pw = p2[gl_GlobalInvocationID.x];

	const float mass_inverse = 1 / pr.mass;

	const vec3 up = vec3(0,1,0);
	const float max_vel = 16.f * mass_inverse;

	float theta = pr.theta;
	// our new position is the current position + last calculated velocity
	vec3 pos = pr.pos.xyz + pr.vel.xyz;
	vec3 force = vec3(0);

	vec3 camera_pos = CalculateBasePosInTorus(camera_theta);
	vec3 camera_dir = normalize(camera_pos - torus_center); 
	vec3 camera_forward = cross(camera_dir, -up);

	{
		// respect camera theta; if the particle is too far away radially from the camera,
		// warp it to the opposite end of the torus
		float theta_diff = AngleBetween(camera_theta, theta);

		if (theta_diff > camera_theta_tolerance && dot(camera_forward, pos - camera_pos) > 0) {
			// calculate the particle's current offset from center before overwriting its position
			vec3 current_offset = pos - CalculateBasePosInTorus(theta);
			theta = camera_theta + camera_theta_tolerance;
			pos = CalculateBasePosInTorus(theta) + current_offset;
		}
	}

	// calculate the forward and right vectors given the current theta
	const vec3 base_pos_in_torus = CalculateBasePosInTorus(theta);
	const vec3 to_center = normalize(torus_center - base_pos_in_torus);
	const vec3 forward = cross(to_center, up);
	const vec3 right = cross(forward, up);

	const int layers = 64;

	const float fbm_offset = (gl_GlobalInvocationID.x % layers) * 113.84f
		+ (gl_GlobalInvocationID.x % layers) / layers * 2.f
		+ global_fbm_offset;

	vec3 force_fbm;
	vec3 normalized_pos = normalize(pos);
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

	// add back to center factor:
	// apply some force back towards the particle's center position within the torus,
	// dependent on how far away from center the particle is
	float b2c_factor = distance(pos, base_pos_in_torus) / torus_thickness;
	// calculate the direction back towards the torus's center
	vec3 back_to_center = normalize(base_pos_in_torus - pw.pos.xyz);
	force += back_to_center * mass_inverse * b2c_factor;

	// calculate new velocity
	pw.vel.xyz += force;
	// incorporate some drag so things don't speed up forever
	pw.vel.xyz *= 0.98;
	if (length(pw.vel.xyz) > max_vel) {
		pw.vel.xyz = normalize(pw.vel.xyz) * max_vel;
	}

	pw.pos.xyz = pos;

	// calculate and store new theta
	vec3 normalized_dir = normalize(pos - torus_center);
	pw.theta = atan(normalized_dir.z, normalized_dir.x);

	p[gl_GlobalInvocationID.x] = pw;
}