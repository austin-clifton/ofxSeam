#pragma once

#include "i-node.h"

using namespace seam::pins;

namespace seam::nodes {
	class ComputeParticles : public INode {
	public:
		ComputeParticles();

		~ComputeParticles();

		void Update(UpdateParams* params) override;

		void Draw(DrawParams* params) override;

		bool GuiDrawPropertiesList() override;

		IPinInput** PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

	private:
		// we use vec4 here for position and velocity instead of vec3 because it aligns well with std140:
		// https://encreative.blogspot.com/2019/06/opengl-buffers-in-openframeworks.html
		struct Particle {
			glm::vec4 pos;
			glm::vec4 vel;
			ofFloatColor color;
			float theta;
			float mass;
			float unused1;	// for std140 alignment?
			float unused2;
		};

		// TODO: ideally this would be instantiation-time editable
		// (not a pin input, but maybe a constructor arg somehow...?)
		const int total_particles = 1024 * 32;

		// GPU-side particles front buffer and back buffer (one is written to while the other is read from)
		ofBufferObject particles_buffer, particles_buffer2;

		// initial seed params for particle positions in a torus
		// these should probably be editable
		const glm::vec3 torus_center = glm::vec3(0);
		const float torus_radius = 500.0f;
		const float torus_thickness = 50.0f;

		// TODO use a normal camera!
		ofEasyCam camera;
		// camera's torus angular offset
		float camera_theta = 0.f;

		ofVbo vbo;

		// final image is written here
		ofFbo fbo;

		// maximum theta delta a particle can have versus the camera before the particle's theta should wrap around
		float max_theta_allowance = 2 * PI;

		const std::string compute_shader_name = "compute-particles.glsl";
		ofShader compute_shader;

		PinFloat<1> pin_time = PinFloat<1>("time");
		PinFloat<1> pin_camera_speed = PinFloat<1>("camera speed", "", { 0.01f });
		std::array<IPinInput*, 2> pin_inputs = {
			&pin_time,
			&pin_camera_speed
		};

		PinOutput pin_out_material = pins::SetupOutputPin(this, pins::PinType::MATERIAL, "material");
	};
}