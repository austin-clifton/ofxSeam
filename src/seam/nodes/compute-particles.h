#pragma once

#include "iNode.h"

using namespace seam::pins;

namespace seam::nodes {
	class ComputeParticles : public INode {
	public:
		ComputeParticles();

		~ComputeParticles();

		void Update(UpdateParams* params) override;

		void Draw(DrawParams* params) override;

		bool GuiDrawPropertiesList(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

	private:
		bool ReloadGeometryShader();

		// Use vec4 here for position and velocity instead of vec3 because it aligns well with std140:
		// https://encreative.blogspot.com/2019/06/opengl-buffers-in-openframeworks.html
		struct Particle {
			glm::vec4 pos;
			glm::vec4 vel;
			ofFloatColor color;
			float theta;
			float mass;
			float size;	
			float unused2; // for std140 alignment?
		};

		// TODO: ideally this would be instantiation-time editable
		// (not a pin input, but maybe a constructor arg somehow...?)
		const int total_particles = 1024 * 1024;

		// GPU-side particles front buffer and back buffer (one is written to while the other is read from)
		ofBufferObject particles_buffer, particles_buffer2;

		// initial seed params for particle positions in a torus
		// these should probably be editable
		const glm::vec3 torus_center = glm::vec3(0);
		const float torus_radius = 500.0f;
		const float torus_thickness = 50.0f;

		// TODO use a normal camera!
		ofEasyCam camera;
		glm::vec3 camera_forward;

		// camera's torus angular offset
		float camera_theta = 0.f;

		ofVbo vbo;

		// final image is written here
		ofFbo fbo;

		// maximum theta delta a particle can have versus the camera before the particle's theta should wrap around
		float max_theta_allowance = 2 * PI;

		const std::string compute_shader_name = "compute-particles.glsl";
		ofShader compute_shader;

		ofShader billboard_shader;

		ofShader test_shader;

		bool free_camera = true;

		float fbm_offset = 0.f;
		float fbm_strength = 3.0f;
		float time = 0.f;
		float camera_speed = 0.1f;
		float camera_theta_tolerance = 1.0f;
		float max_particle_velocity = 4.0f;
		float particle_size_modifier = 1.0f;
		float particle_wave_distance = 0.f;

		std::array<PinInput, 8> pin_inputs = {
			pins::SetupInputPin(PinType::FLOAT, this, &fbm_offset, 1, "FBM Offset"),
			pins::SetupInputPin(PinType::FLOAT, this, &fbm_strength, 1, "FBM Strength"),
			pins::SetupInputPin(PinType::FLOAT, this, &time, 1, "Time"),
			pins::SetupInputPin(PinType::FLOAT, this, &camera_speed, 1, "Camera Speed"),
			pins::SetupInputPin(PinType::FLOAT, this, &camera_theta_tolerance, 1, "Camera Theta Tolerance"),
			pins::SetupInputPin(PinType::FLOAT, this, &max_particle_velocity, 1, "Max Particle Velocity"),
			pins::SetupInputPin(PinType::FLOAT, this, &particle_size_modifier, 1, "Particle Size Modifier"),
			pins::SetupInputPin(PinType::FLOAT, this, &particle_wave_distance, 1, "Particle Wave Distance"),
		};

		PinOutput pin_out_material = pins::SetupOutputPin(this, pins::PinType::FBO_RGBA, "FBO");
	};
}