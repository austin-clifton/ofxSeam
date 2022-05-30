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

		// we use vec4 here for position and velocity instead of vec3 because it aligns well with std140:
		// https://encreative.blogspot.com/2019/06/opengl-buffers-in-openframeworks.html
		struct Particle {
			glm::vec4 pos;
			glm::vec4 vel;
			ofFloatColor color;
			float theta;
			float mass;
			float size;	// for std140 alignment?
			float unused2;
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


		PinFloat<1> pin_fbm_offset = PinFloat<1>("fbm_offset");
		PinFloat<1> pin_time = PinFloat<1>("time");
		PinFloat<1> pin_camera_speed = PinFloat<1>("camera speed", "", { -0.1f });
		PinFloat<1> pin_camera_theta_tolerance = PinFloat<1>("camera theta tolerance", "", { PI/2 });
		std::array<IPinInput*, 4> pin_inputs = {
			&pin_time,
			&pin_camera_speed,
			&pin_fbm_offset,
			&pin_camera_theta_tolerance
		};

		PinOutput pin_out_material = pins::SetupOutputPin(this, pins::PinType::MATERIAL, "material");
	};
}