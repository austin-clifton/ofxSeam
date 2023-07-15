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

		// This struct should keep 16-byte alignment for the sake of std-140:
		// https://encreative.blogspot.com/2019/06/opengl-buffers-in-openframeworks.html
		struct ComputeFirefly {
			float mass;
			float luminance;
			int effect_index;
			int group_index;

			glm::vec4 color;
			
			glm::vec3 velocity;
			glm::vec3 acceleration;
			float unused1;
			float unused2;
		};

		Octree<glm::vec3, 8> octree;
		
		std::thread avoidance_thread;
		bool stop = false;
		bool invalidate_posmap = false;
		bool invalidate_dirmap = false;

		uint32_t fireflies_count = 1000;

		ofShader ff_compute_shader;
		ofShader ff_geo_shader;

		ofShader test_shader;

		ofEasyCam camera;

		std::vector<glm::vec3> ff_positions;
		
		// Firefly positions are written to this buffer for use by the billboard geometry shader.
		ofBufferObject ff_vertex_buffer;

		// Front and back buffers for compute-internal firefly 
		ofBufferObject ff_compute_ssbo;
		ofBufferObject ff_compute_ssbo2;
		
		GLuint ff_positions_ssbo;
		GLuint ff_directions_ssbo;
		void* ff_posmap = nullptr;
		void* ff_dirmap = nullptr;

		ofVbo ff_vbo;

		// Final image is written here
		ofFbo fbo;

		ofBoxPrimitive box;

		PinFloat<1> pin_avoidance_radius = PinFloat<1>("avoidance radius", "", { 1.0f }, 0.001f);
		PinFloat<1> pin_avoidance_force = PinFloat<1>("avoidance force");

		std::array<IPinInput*, 2> pin_inputs = {
			&pin_avoidance_radius,
			&pin_avoidance_force,
		};
		
		PinOutput pin_out_texture = pins::SetupOutputPin(this, pins::PinType::TEXTURE, "texture");
	};
}