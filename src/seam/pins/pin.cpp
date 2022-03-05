#include "pin.h"
#include "pin-int.h"
#include "pin-float.h"
#include "pin-uint.h"

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
			GLenum err = glGetError();
			assert(err == GL_NO_ERROR);

			// use string_view with c_str() so it finds the nul char for us
			std::string_view snipped_name(name.c_str());

			// TODO filter out uniforms prefixed with "gl_"?

			// now make a PinInput for the uniform
			// first need to figure out what kind of Pin is needed
			IPinInput* pin = nullptr;
			// TODO many more possible types here, including textures,
			// and many which are "multichannel", and require a rework of Pins so that they're multichannel capable
			switch (uniform_type) {
			case GL_FLOAT:
				pin = new PinFloat<1>(snipped_name);
				break;
			case GL_INT:
				pin = new PinInt<1>(snipped_name);
				break;
			case GL_UNSIGNED_INT:
				pin = new PinUint<1>(snipped_name);
				break;
			case GL_FLOAT_VEC2:
				pin = new PinFloat<2>(snipped_name);
				break;
			case GL_INT_VEC2:
				pin = new PinInt<2>(snipped_name);
				break;
			case GL_UNSIGNED_INT_VEC2:
				pin = new PinUint<2>(snipped_name);
			default:
				printf("uniform to PinInput for GL with enum type %d not implemented yet\n", uniform_type);
				break;
			}

			// skip over non-supported Pin types
			if (pin == nullptr) {
				continue;
			}

			// grab the location of the uniform so we can set initial values for the Pin
			GLint uniform_location = glGetUniformLocation(program, snipped_name.data());
			assert(glGetError() == GL_NO_ERROR);

			// fill the Pin's default values
			size_t size;
			void* channels = pin->GetChannels(size);
			switch (pin->type) {
			case PinType::FLOAT:
				glGetnUniformfv(program, uniform_location, size * sizeof(float), (float*)channels);
				break;
			case PinType::INT:
				glGetnUniformiv(program, uniform_location, size * sizeof(int), (int*)channels);
				break;
			case PinType::UINT:
				glGetnUniformuiv(program, uniform_location, size * sizeof(uint32_t), (uint32_t*)channels);
				break;
			}

			assert(glGetError() == GL_NO_ERROR);

			// add it to the list
			pin->node = node;
			pin_inputs.push_back(pin);
			
		}

		return pin_inputs;
	}
}