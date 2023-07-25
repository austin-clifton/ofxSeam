#pragma once

#include "i-node.h"

#include "../pins/pin.h"
#include "../containers/octree.h"

using namespace seam::pins;

namespace seam::nodes {
	/// <summary>
	/// Fireflies scene originally made for Lazy Bear 2023
	/// </summary>
	class Fireflies : public INode {
	public:
		Fireflies();
		~Fireflies();

		void Update(UpdateParams* params) override;

		void Draw(DrawParams* params) override;

		IPinInput** PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		bool GuiDrawPropertiesList() override;

	private:
		bool LoadShaders();
		void AvoidanceLoop(); 
		void UpdateDirections(
			Octree<glm::vec4, 8>& octree,
			const std::vector<glm::vec4>& positions, 
			glm::vec4* dirmap, 
			size_t count, 
			std::vector<glm::vec4*>& found, 
			float avoidance_radius
		);

		// This struct should keep 16-byte alignment for the sake of std-140:
		// https://encreative.blogspot.com/2019/06/opengl-buffers-in-openframeworks.html
		struct ComputeFirefly {
			float mass;
			float luminance;
			int effect_index;
			int group_index;

			glm::vec4 color;
			
			glm::vec3 velocity;
			float unused;
		};

		struct ComputeMomma {
			float mass;
			glm::vec3 velocity;
			
			// float col_time_offset = 0.f;
			// float col_time_mul = 1.0f;


			glm::vec3 col_a;
			glm::vec3 col_b;
			glm::vec3 col_c;
			glm::vec3 col_d;
		};

		Octree<glm::vec4, 8> ff_octree;
		Octree<glm::vec4, 8> mom_octree;
		
		std::thread avoidance_thread;
		bool stop = false;
		bool invalidate_posmap = false;
		bool invalidate_dirmap = false;

		size_t fireflies_count = 3000;
		size_t moms_count = 9;

		ofShader ff_compute_shader;
		ofShader ff_geo_shader;

		ofShader mom_compute_shader;

		ofShader test_shader;

		ofEasyCam camera;

		std::vector<glm::vec4> ff_positions;
		std::vector<glm::vec4> mom_positions;
		
		// Firefly positions are written to this buffer for use by the billboard geometry shader.
		ofBufferObject ff_vertex_buffer;
		ofBufferObject mom_vertex_buffer;

		// Front and back buffers for compute-internal firefly calculations.
		ofBufferObject ff_compute_ssbo_read;
		ofBufferObject ff_compute_ssbo_write;

		// Front and back buffers for compute-internal momma calculations.
		ofBufferObject mom_compute_ssbo_read;
		ofBufferObject mom_compute_ssbo_write;
		
		// For both fireflies and their mommas, directions are dictated by positions which are updated in an octree.
		// Directions are persistently mapped for writing, and positions are persistently mapped for reading.
		GLuint ff_positions_ssbo;
		GLuint ff_directions_ssbo;
		glm::vec4* ff_posmap = nullptr;
		glm::vec4* ff_dirmap = nullptr;

		GLuint mom_positions_ssbo;
		GLuint mom_directions_ssbo;
		glm::vec4* mom_posmap = nullptr;
		glm::vec4* mom_dirmap = nullptr;

		ofVbo ff_vbo;
		ofVbo mom_vbo;

		// Final image is written here
		ofFbo fbo;

		ofBoxPrimitive box;

		PinFloat<1> pin_avoidance_radius = PinFloat<1>("avoidance radius", "", { 4.0f }, 0.001f);
		PinFloat<1> pin_mom_avoidance_radius = PinFloat<1>("mom avoidance radius", "", { 16.0f }, 0.001f);
		PinFloat<1> pin_avoidance_force = PinFloat<1>("avoidance force");
		PinFloat<1> pin_max_velocity = PinFloat<1>("max velocity", "", { 0.2f });
		PinFloat<1> pin_pulse_camera_ndistance = PinFloat<1>(
			"pulse distance", 
			"distance (relative to camera near and far plane depth) that fireflies should pulse at", 
			{ -10.f }
		);
		PinFloat<1> pin_pulse_range = PinFloat<1>("pulse range", "", { 10.f }, 0.f);

		std::array<IPinInput*, 6> pin_inputs = {
			&pin_avoidance_radius,
			&pin_mom_avoidance_radius,
			&pin_avoidance_force,
			&pin_max_velocity,
			&pin_pulse_camera_ndistance,
			&pin_pulse_range,
		};
		
		PinOutput pin_out_texture = pins::SetupOutputPin(this, pins::PinType::FBO, "texture");
	};
}