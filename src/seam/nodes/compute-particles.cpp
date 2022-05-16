#include "compute-particles.h"
#include "../imgui-utils/properties.h"

using namespace seam::nodes;
using namespace seam::pins;
using namespace seam::notes;

namespace {
	// calculate local position within a torus
	glm::vec3 CalculateTorusPosition(glm::vec3 torus_center, float torus_radius, float torus_thickness, float theta, glm::vec2 offset) {
		theta = theta * 2 * PI;
		const glm::vec3 base_pos_in_torus = torus_center + torus_radius * glm::vec3(cos(theta), 0.f, sin(theta));

		// figure out what the forward vector is so we can find the right vector for the offset
		const glm::vec3 up = glm::vec3(0, 1, 0);
		const glm::vec3 to_center = glm::normalize(torus_center - base_pos_in_torus);
		const glm::vec3 forward = glm::cross(to_center, up);
		const glm::vec3 right = glm::cross(forward, up);

		// now apply the offset within the torus 
		return base_pos_in_torus
			+ (offset.x * torus_thickness * right)
			+ (offset.y * torus_thickness * up);
	}

	bool LoadComputeShader(const std::string shader_name, ofShader& compute_shader) {
		const std::filesystem::path shader_path = std::filesystem::current_path() / "data" / "shaders" / shader_name;
		return compute_shader.setupShaderFromFile(GL_COMPUTE_SHADER, shader_path) && compute_shader.linkProgram();
	}
}

ComputeParticles::ComputeParticles() : INode("Compute Particles") {
	flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL);

	// TEMP!!!
	flags = (NodeFlags)(flags | NodeFlags::UPDATES_OVER_TIME);

	gui_display_fbo = &fbo;

	if (!LoadComputeShader(compute_shader_name, compute_shader)) {
		printf("failed to load particles shader!\n");
	}

	// TEMP from example
	camera.setFarClip(ofGetWidth() * 10.f);

	// set up initial "seed" particle buffer
	std::vector<Particle> particles;
	particles.resize(total_particles);

	for (auto& p : particles) {
		float theta = ofRandom(camera_theta, camera_theta + max_theta_allowance);
		glm::vec2 offset = glm::vec2(ofRandomf(), ofRandomf());
		glm::vec3 ppos = CalculateTorusPosition(torus_center, torus_radius, torus_thickness, theta, offset);
		p.pos = glm::vec4(ppos.x, ppos.y, ppos.z, 1);
		p.vel = glm::vec4(0);
		p.theta = theta;
		p.mass = ofRandom(0.1, 5.0);
	}

	// allocate and seed initialize buffer objects
	particles_buffer.allocate(particles, GL_DYNAMIC_DRAW);
	particles_buffer2.allocate(particles, GL_DYNAMIC_DRAW);

	// tell the VBO where to source its data from
	vbo.setVertexBuffer(particles_buffer, 4, sizeof(Particle));
	vbo.setColorBuffer(particles_buffer, sizeof(Particle), sizeof(glm::vec4) * 2);
	vbo.enableColors();

	// TODO size
	fbo.allocate(1920, 1080);

	// TODO not sure this belongs here vs Draw()
	ofBackground(0);
	ofEnableBlendMode(OF_BLENDMODE_ADD);

	glm::vec3 camera_pos = CalculateTorusPosition(torus_center, torus_radius, torus_thickness, camera_theta, glm::vec2(0));
	camera.setPosition(camera_pos);

	particles_buffer.bindBase(GL_SHADER_STORAGE_BUFFER, 0);
	particles_buffer2.bindBase(GL_SHADER_STORAGE_BUFFER, 1);

}

ComputeParticles::~ComputeParticles() {

}

void ComputeParticles::Update(UpdateParams* params) {
	compute_shader.begin();

	compute_shader.setUniform1f("timeLastFrame", ofGetLastFrameTime());
	compute_shader.setUniform1f("elapsedTime", ofGetElapsedTimef());

	// since each work group has a local_size of 1024 (this is defined in the shader)
	// we only have to issue 1 / 1024 workgroups to cover the full workload.
	// note how we add 1024 and subtract one, this is a fast way to do the equivalent
	// of std::ceil() in the float domain, i.e. to round up, so that we're also issueing
	// a work group should the total size of particles be < 1024
	compute_shader.dispatchCompute((total_particles + 1024 - 1) / 1024, 1, 1);

	compute_shader.end();

	particles_buffer.copyTo(particles_buffer2);
}

void ComputeParticles::Draw(DrawParams* params) {
	fbo.begin();
	fbo.clearColorBuffer(0);

	// update camera's position and rotation
	// TODO up vector should use the torus's transform (you need to make a torus transform...)
	camera_theta += params->delta_time * pin_camera_speed[0];
	glm::vec3 camera_pos = CalculateTorusPosition(torus_center, torus_radius, torus_thickness, camera_theta, glm::vec2(0));
	camera.setPosition(camera_pos);

	glm::vec3 forward = glm::cross(glm::vec3(0, 1, 0), glm::normalize(torus_center - camera_pos));
	// I can't figure out how else to do this in OF; I want to set the camera's forward vector to the forward I just calculated...
	// use lookat instead I guess
	camera.lookAt(camera_pos + forward);

	// this is "wrong", but looks cool regardless
	// camera.setOrientation(glm::quat(forward));

	ofEnableBlendMode(OF_BLENDMODE_ADD);
	camera.begin();

	ofSetColor(255, 70);
	glPointSize(2);
	vbo.draw(GL_POINTS, 0, total_particles);
	ofSetColor(255);
	glPointSize(1);
	vbo.draw(GL_POINTS, 0, total_particles);

	camera.end();

	camera.lookAt(camera_pos - forward);

	camera.begin();

	ofSetColor(255, 70);
	glPointSize(2);
	vbo.draw(GL_POINTS, 0, total_particles);
	ofSetColor(255);
	glPointSize(1);
	vbo.draw(GL_POINTS, 0, total_particles);

	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	ofSetColor(255);

	camera.end();

	fbo.end();
}

bool ComputeParticles::GuiDrawPropertiesList() {
	if (ImGui::Button("refresh")) {
		if (!LoadComputeShader(compute_shader_name, compute_shader)) {
			printf("compute shader failed to reload!\n");
			return false;
		}
		return true;
	}
	return false;
}

IPinInput** ComputeParticles::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

PinOutput* ComputeParticles::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_material;
}