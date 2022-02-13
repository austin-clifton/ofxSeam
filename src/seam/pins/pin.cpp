#include "pin.h"
#include "pin-int.h"
#include "pin-float.h"

namespace seam::pins {
	PinOutput SetupOutputPin(
		nodes::INode* node, 
		PinType type, 
		std::string_view name, 
		PinFlags flags,
		void* userp
	) {
		assert(node != nullptr);
		PinOutput pin_out;
		pin_out.pin.node = node;
		pin_out.pin.type = type;
		pin_out.pin.name = name;
		pin_out.pin.flags = (PinFlags)(flags | PinFlags::OUTPUT);
		pin_out.userp = userp;
		return pin_out;
	}

	std::vector<IPinInput*> UniformsToPinInputs(ofShader& shader, nodes::INode* node) {
		// sanity check there were no errors before now
		assert(glGetError() == GL_NO_ERROR);

		// first ask opengl how many active uniforms there are
		const GLint program = shader.getProgram();
		GLint uniforms_count;
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniforms_count);
		assert(glGetError() == GL_NO_ERROR);

		// allocate a string big enough to store the uniforms' longest name
		GLint max_name_length;
		glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_length);
		assert(glGetError() == GL_NO_ERROR);
		std::string name;
		// the length includes the null terminating char
		name.resize(max_name_length);

		std::vector<IPinInput*> pin_inputs;
		pin_inputs.reserve(uniforms_count);
		// query each uniform variable and make a PinInput for it
		for (GLint i = 0; i < uniforms_count; i++) {
			// glGetActiveUniform() writes uniform metadata to each of these
			GLint name_length;
			GLint uniform_size;
			GLenum uniform_type;
			// query a uniform's data
			glGetActiveUniform(
				program, 
				i, 
				max_name_length, 
				&name_length, 
				&uniform_size, 
				&uniform_type, 
				&name[0]
			);
			assert(glGetError() == GL_NO_ERROR);

			// TODO filter out uniforms prefixed with "gl_"?

			// now make a PinInput for the uniform
			// first need to figure out what kind of Pin is needed
			IPinInput* pin = nullptr;
			// TODO many more possible types here, including textures,
			// and many which are "multichannel", and require a rework of Pins so that they're multichannel capable
			switch (uniform_type) {
			case GL_FLOAT:
				pin = new PinFloat<1>(name.c_str());
				break;
			case GL_INT:
				pin = new PinInt<1>(name.c_str());
				break;
			default:
				printf("uniform to PinInput for GL with enum type %d not implemented yet\n", uniform_type);
				break;
			}
			
			// add it to the list, if it's an implemented and supported pin type
			if (pin != nullptr) {
				pin->node = node;
				pin_inputs.push_back(pin);
			}
		}

		return pin_inputs;
	}
}