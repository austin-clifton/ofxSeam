#include "pin.h"

const size_t seam::pins::PinInput::MAX_EVENTS = 16;

namespace {
	bool StrCmpLower(std::string_view s1, std::string_view s2) {
		if (s1.length() != s2.length()) {
			return false;
		}

		bool same = true;
		for (size_t i = 0; same && i < s1.length(); i++) {
			same = std::tolower(s1[i]) == std::tolower(s2[i]);
		}
		return same;
	}
}

namespace seam::pins {
	props::NodePropertyType PinTypeToPropType(PinType pinType) {
		using namespace seam::props;
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

	PinType SerializedPinTypeToPinType(seam::schema::PinValue::Which serializedType) {
		switch (serializedType) {
			case seam::schema::PinValue::Which::BOOL_VALUE:
				return PinType::BOOL;
			case seam::schema::PinValue::Which::FLOAT_VALUE:
				return PinType::FLOAT;
			case seam::schema::PinValue::Which::INT_VALUE:
				return PinType::INT;
			case seam::schema::PinValue::Which::UINT_VALUE:
				return PinType::UINT;
			case seam::schema::PinValue::Which::STRING_VALUE:
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
		pin_out.node = node;
		pin_out.type = type;
		pin_out.name = name;
		pin_out.flags = (PinFlags)(flags | PinFlags::OUTPUT);
		pin_out.userp = userp;
		return pin_out;
	}

	std::vector<PinInput> UniformsToPinInputs(
		ofShader& shader, 
		nodes::INode* node, 
		std::vector<char>& pinBuffer
	) {
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

		// Clear out any old uniform buffer data, and keep track
		// of how much space is needed for the new uniform data.
		pinBuffer.clear();
		size_t totalBytesNeeded = 0;

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
			PinType pinType = PinType::TYPE_NONE;
			size_t channelsSize = 0;

			// Now make a PinInput for the uniform.
			// First need to figure out what kind of Pin is needed.
			// TODO many more possible types here, including textures.
			switch (uniform_type) {
			case GL_FLOAT:
				pinType = PinType::FLOAT;
				channelsSize = 1;
				break;
			case GL_INT:
				pinType = PinType::INT;
				channelsSize = 1;
				break;
			case GL_UNSIGNED_INT:
				pinType = PinType::UINT;
				channelsSize = 1;
				break;
			case GL_FLOAT_VEC2:
				pinType = PinType::FLOAT;
				channelsSize = 2;
				break;
			case GL_FLOAT_VEC3:
				pinType = PinType::FLOAT;
				channelsSize = 3;
				break;
			case GL_INT_VEC2:
				pinType = PinType::INT;
				channelsSize = 2;
				break;
			case GL_UNSIGNED_INT_VEC2:
				pinType = PinType::UINT;
				channelsSize = 2;
				break;
			default:
				printf("uniform to PinInput for GL with enum type %d not implemented yet\n", uniform_type);
				failed = true;
				break;
			}

			// skip over non-supported Pin types
			if (failed) {
				continue;
			}

			const size_t bytesPerChannel = PinTypeToElementSize(pinType);
			totalBytesNeeded += bytesPerChannel * channelsSize;

			pin_inputs.push_back(SetupInputPin(pinType, node, nullptr, channelsSize, snippedName));

		}

		// Resize the pin buffer now and point each Pin's channel buffer at it.
		pinBuffer.resize(totalBytesNeeded, 0);
		size_t currentByte = 0;

		for (auto& pinIn : pin_inputs) {
			pinIn.node = node;

			// Now that memory has been alloc'd for the uniform channels buffer,
			// set each Pin's channels buffer to point to the right place.
			size_t channelsSize;
			void* channels = pinIn.GetChannels(channelsSize);
			channels = &pinBuffer[currentByte];
			pinIn.SetBuffer(channels, channelsSize);
			currentByte += PinTypeToElementSize(pinIn.type) * channelsSize;

			// grab the location of the uniform so we can set initial values for the Pin
			GLint uniform_location = glGetUniformLocation(program, pinIn.name.c_str());
			assert(glGetError() == GL_NO_ERROR);

			// fill the Pin's default values
			switch (pinIn.type) {
			case PinType::FLOAT:
				glGetnUniformfv(program, uniform_location, 
					channelsSize * sizeof(float), (float*)channels);
				break;
			case PinType::INT:
				glGetnUniformiv(program, uniform_location, 
					channelsSize * sizeof(int32_t), (int32_t*)channels);
				break;
			case PinType::UINT:
				glGetnUniformuiv(program, uniform_location, 
					channelsSize * sizeof(uint32_t), (uint32_t*)channels);
				break;
			}

			assert(glGetError() == GL_NO_ERROR);
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
        std::function<void(void)>&& callback,
		const std::string_view description
	) {
		// TODO: Validate inputs in debug mode...?

		if (elementSizeInBytes == 0) {
			elementSizeInBytes = PinTypeToElementSize(pinType);
		}

		return PinInput(
			pinType, 
			name, 
			description, 
			node, 
			channels, 
			numChannels, 
			elementSizeInBytes, 
			std::move(callback),
			pinMetadata);
	}

	PinInput SetupInputPin(
		PinType pinType,
		nodes::INode* node,
		void* channels,
		const size_t numChannels,
		const std::string_view name,
		PinInOptions&& options
	) {
		size_t elementSize = options.elementSize > 0 
			? options.elementSize : PinTypeToElementSize(pinType);

		return PinInput(
			pinType,
			name,
			options.description,
			node,
			channels, 
			numChannels,
			elementSize,
			std::move(options.callback),
			options.pinMetadata
		);
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
		case PinType::ANY:
		case PinType::FLOW:
		case PinType::STRUCT:
			return 0;
		case PinType::INT:
			return sizeof(int32_t);
		case PinType::UINT:
			return sizeof(uint32_t);
		case PinType::NOTE_EVENT:
			return sizeof(notes::NoteEvent*);
		case PinType::FBO:
			return sizeof(ofFbo*);
		default:
			throw std::runtime_error("Unknown pin type! You need to provide the element size in bytes yourself.");
		}
	}

	PinInput* FindPinInByName(PinInput* pins, size_t pinsSize, std::string_view name) {
		auto it = std::find_if(pins, pins + pinsSize, [name](const PinInput& pinIn) {
			return StrCmpLower(name, pinIn.name);
		});
		return it != pins + pinsSize ? it : nullptr;
	}

	PinInput* FindPinInByName(IInPinnable* pinnable, std::string_view name) {
		size_t size;
		PinInput* pins = pinnable->PinInputs(size);
		return FindPinInByName(pins, size, name);
	}

	PinOutput* FindPinOutByName(PinOutput* pins, size_t pinsSize, std::string_view name) {
		auto it = std::find_if(pins, pins + pinsSize, [name](const PinOutput& pin_out) { 
			return StrCmpLower(name, pin_out.name); 
		});
		return it != pins + pinsSize ? it : nullptr;
	}
	
	PinOutput* FindPinOutByName(IOutPinnable* pinnable, std::string_view name) {
		size_t size;
		PinOutput* pins = pinnable->PinOutputs(size);
		return FindPinOutByName(pins, size, name);
	}
}