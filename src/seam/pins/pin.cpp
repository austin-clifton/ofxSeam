#include "pin.h"

namespace seam {

	void Serialize(capnp::List<PinValue, capnp::Kind::STRUCT>::Builder& builder,
		NodePropertyType type, void* srcBuff, size_t srcElementsCount)
	{
		if (srcElementsCount == 0) {
			return;
		}

		switch (type) {
		case NodePropertyType::PROP_BOOL: {
			bool* bool_values = (bool*)srcBuff;
			for (size_t i = 0; i < srcElementsCount; i++) {
				builder[i].setBoolValue(bool_values[i]);
			}
			break;
		}
		case NodePropertyType::PROP_INT: {
			int32_t* int_values = (int32_t*)srcBuff;
			for (size_t i = 0; i < srcElementsCount; i++) {
				builder[i].setIntValue(int_values[i]);
			}
			break;
		}
		case NodePropertyType::PROP_UINT: {
			uint32_t* uint_values = (uint32_t*)srcBuff;
			for (size_t i = 0; i < srcElementsCount; i++) {
				builder[i].setUintValue(uint_values[i]);
			}
			break;
		}
		case NodePropertyType::PROP_FLOAT: {
			float* float_values = (float*)srcBuff;
			for (size_t i = 0; i < srcElementsCount; i++) {
				builder[i].setFloatValue(float_values[i]);
			}
			break;
		}
		case NodePropertyType::PROP_CHAR:
		case NodePropertyType::PROP_STRING:
		default:
			throw std::logic_error("not implemented yet!");
		}
	}

	void Deserialize(const capnp::List<PinValue, capnp::Kind::STRUCT>::Reader& serializedValues, NodePropertyType type, void* dstBuff, size_t dstElementsCount) {
		if (serializedValues.size() == 0) {
			return;
		}

		switch (type) {
		case NodePropertyType::PROP_BOOL: {
			if (serializedValues[0].isBoolValue()) {
				bool* bool_channels = (bool*)dstBuff;
				for (size_t i = 0; i < dstElementsCount && i < serializedValues.size(); i++) {
					bool_channels[i] = serializedValues[i].getBoolValue();
				}
			}
			break;
		}
		case NodePropertyType::PROP_FLOAT: {
			if (serializedValues[0].isFloatValue()) {
				float* float_channels = (float*)dstBuff;
				for (size_t i = 0; i < dstElementsCount && i < serializedValues.size(); i++) {
					float_channels[i] = serializedValues[i].getFloatValue();
				}
			}
			break;
		}
		case NodePropertyType::PROP_INT: {
			if (serializedValues[0].isIntValue()) {
				int* int_channels = (int32_t*)dstBuff;
				for (size_t i = 0; i < dstElementsCount && i < serializedValues.size(); i++) {
					int_channels[i] = serializedValues[i].getIntValue();
				}
			}
			break;
		}
		case NodePropertyType::PROP_UINT: {
			if (serializedValues[0].isUintValue()) {
				uint32_t* uint_channels = (uint32_t*)dstBuff;
				for (size_t i = 0; i < dstElementsCount && i < serializedValues.size(); i++) {
					uint_channels[i] = serializedValues[i].getUintValue();
				}
			}
			break;
		}
		default:
			throw std::logic_error("not implemented!");
		}
	}
}

namespace seam::pins {
	NodePropertyType PinTypeToPropType(PinType pinType) {
		switch (pinType) {
		case PinType::BOOL:
			return NodePropertyType::PROP_BOOL;
		case PinType::CHAR:
			return NodePropertyType::PROP_CHAR;
		case PinType::FLOAT:
			return NodePropertyType::PROP_FLOAT;
		case PinType::INT:
			return NodePropertyType::PROP_INT;
		case PinType::UINT:
			return NodePropertyType::PROP_UINT;
		case PinType::STRING:
			return NodePropertyType::PROP_STRING;
		default:
			throw std::logic_error("Only some pin types can be converted to property types");
		}
	}

	NodePropertyType SerializedPinTypeToPropType(PinValue::Which pinType) {
		switch (pinType) {
		case PinValue::Which::BOOL_VALUE:
			return NodePropertyType::PROP_BOOL;
		case PinValue::Which::FLOAT_VALUE:
			return NodePropertyType::PROP_FLOAT;
		case PinValue::Which::INT_VALUE:
			return NodePropertyType::PROP_INT;
		case PinValue::Which::UINT_VALUE:
			return NodePropertyType::PROP_UINT;
		case PinValue::Which::STRING_VALUE:
			return NodePropertyType::PROP_STRING;
		default:
			throw std::logic_error("Unimplemented?");
		}
	}

	PinType SerializedPinTypeToPinType(PinValue::Which serializedType) {
		switch (serializedType) {
			case PinValue::Which::BOOL_VALUE:
				return PinType::BOOL;
			case PinValue::Which::FLOAT_VALUE:
				return PinType::FLOAT;
			case PinValue::Which::INT_VALUE:
				return PinType::INT;
			case PinValue::Which::UINT_VALUE:
				return PinType::UINT;
			case PinValue::Which::STRING_VALUE:
				return PinType::STRING;
			default:
				throw std::logic_error("Unimplemented?");
		}
	}

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

	/// <summary>
	/// 
	/// </summary>
	/// <param name="shader"></param>
	/// <param name="node"></param>
	/// <returns>A vector containing all the found uniforms that map to pins.
	/// Each pin has memory allocated for its channels data.
	/// TODO: handle deallocation of allocated memory with a helper function.
	/// </returns>
	std::vector<PinInput> UniformsToPinInputs(ofShader& shader, nodes::INode* node) {
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

		std::vector<PinInput> pin_inputs;
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
			std::string_view snippedName(name.c_str());

			// TODO filter out uniforms prefixed with "gl_"?

			bool failed = false;

			// Now make a PinInput for the uniform.
			// First need to figure out what kind of Pin is needed.
			// TODO many more possible types here, including textures.
			switch (uniform_type) {
			case GL_FLOAT:
				pin_inputs.push_back(SetupInputPin(PinType::FLOAT, node, new float, 1, snippedName));
				break;
			case GL_INT:
				pin_inputs.push_back(SetupInputPin(PinType::INT, node, new int32_t, 1, snippedName));
				break;
			case GL_UNSIGNED_INT:
				pin_inputs.push_back(SetupInputPin(PinType::UINT, node, new uint32_t, 1, snippedName));
				break;
			case GL_FLOAT_VEC2:
				pin_inputs.push_back(SetupInputPin(PinType::FLOAT, node, new glm::vec2(), 2, snippedName));
				break;
			case GL_INT_VEC2:
				pin_inputs.push_back(SetupInputPin(PinType::INT, node, new glm::ivec2(), 2, snippedName));
				break;
			case GL_UNSIGNED_INT_VEC2:
				pin_inputs.push_back(SetupInputPin(PinType::INT, node, new glm::u32vec2(), 2, snippedName));
			default:
				printf("uniform to PinInput for GL with enum type %d not implemented yet\n", uniform_type);
				failed = true;
				break;
			}

			// skip over non-supported Pin types
			if (failed) {
				continue;
			}

			PinInput& pin = pin_inputs[pin_inputs.size() - 1];

			// grab the location of the uniform so we can set initial values for the Pin
			GLint uniform_location = glGetUniformLocation(program, snippedName.data());
			assert(glGetError() == GL_NO_ERROR);

			// fill the Pin's default values
			size_t size;
			void* channels = pin.GetChannels(size);
			switch (pin.type) {
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
			pin.node = node;
			pin_inputs.push_back(pin);
		}

		return pin_inputs;
	}

	PinInput SetupInputPin(
		PinType pinType,
		nodes::INode* node,
		void* channels,
		const size_t numChannels,
		const std::string_view name,
		size_t elementSizeInBytes,
		void* pinMetadata,
		const std::string_view description
	) {
		// TODO: Validate inputs in debug mode...?

		if (elementSizeInBytes == 0) {
			elementSizeInBytes = PinTypeToElementSize(pinType);
		}


		return PinInput(pinType, name, description, node, channels, numChannels, elementSizeInBytes, pinMetadata);
	}

	PinInput SetupInputQueuePin(
		PinType pinType,
		nodes::INode* node,
		const std::string_view name,
		size_t elementSizeInBytes,
		void* pinMetadata,
		const std::string_view description
	) {
		// TODO: Validate inputs in debug mode...?

		if (elementSizeInBytes == 0) {
			elementSizeInBytes = PinTypeToElementSize(pinType);
		}

		return PinInput(pinType, name, description, node, elementSizeInBytes, pinMetadata);
	}

	PinInput SetupInputFlowPin(
		nodes::INode* node,
		std::function<void(void)>&& callback,
		const std::string_view name,
		void* pinMetadata,
		const std::string_view description
	) {
		return PinInput(name, description, node, std::move(callback), pinMetadata);
	}

	size_t PinTypeToElementSize(PinType type) {
		switch (type) {
		case PinType::BOOL:
			return sizeof(bool);
		case PinType::CHAR:
			return sizeof(char);
		case PinType::FLOAT:
			return sizeof(float);
		case PinType::FLOW:
			return 0;
		case PinType::INT:
			return sizeof(int32_t);
		case PinType::UINT:
			return sizeof(uint32_t);
		case PinType::NOTE_EVENT:
			return sizeof(notes::NoteEvent*);
		default:
			throw std::runtime_error("Unknown pin type! You need to provide the element size in bytes yourself.");
		}
	}
}
