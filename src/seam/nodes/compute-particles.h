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

		PinFloat<1> pin_fbm_offset = PinFloat<1>("fbm_offset");
		PinFloat<1> pin_fbm_strength = PinFloat<1>("fbm strength", "", { 3.0f });
		PinFloat<1> pin_time = PinFloat<1>("time");
		PinFloat<1> pin_camera_speed = PinFloat<1>("camera speed", "", { 0.1f });
		PinFloat<1> pin_camera_theta_tolerance = PinFloat<1>("camera theta tolerance", "", { 1.8f });
		PinFloat<1> pin_max_particle_velocity = PinFloat<1>("max particle velocity", "", { 4.0f });
		PinFloat<1> pin_particle_size_modifier = PinFloat<1>("particle size modifier", "", { 1.0f });
		PinFloat<1> pin_particle_wave_distance = PinFloat<1>(
			"particle wave distance",
			"[0..PI) particles near the wave will have increased size; values past PI turn the wave off",
			{ PI },
			{ 0.f }
		);
		std::array<IPinInput*, 8> pin_inputs = {
			&pin_time,
			&pin_camera_speed,
			&pin_fbm_offset,
			&pin_fbm_strength,
			&pin_camera_theta_tolerance,
			&pin_max_particle_velocity,
			&pin_particle_size_modifier,
			&pin_particle_wave_distance
		};

		PinOutput pin_out_material = pins::SetupOutputPin(this, pins::PinType::MATERIAL, "material");
	};
}