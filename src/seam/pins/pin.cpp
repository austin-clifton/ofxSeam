#include "seam/pins/pin.h"

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
		uint16_t numCoords,
		PinFlags flags,
		void* userp
	) {
#if !defined(RUN_DOCTEST)
		assert(node != nullptr);
#endif
		PinOutput pinOut;
		pinOut.node = node;
		pinOut.type = type;
		pinOut.name = name;
		pinOut.SetNumCoords(numCoords);
		pinOut.flags = (PinFlags)(flags | PinFlags::OUTPUT);
		pinOut.userp = userp;
		return pinOut;
	}

	PinOutput SetupOutputStaticFboPin(
		nodes::INode* node,
		ofFbo* fbo,
		PinType fboType,
		const std::string_view name,
		PinFlags flags,
		const std::string_view description,
		void* userp
	) {
#if !defined(RUN_DOCTEST)
		assert(node != nullptr);
#endif
		assert(IsFboPin(fboType));

		PinOutput pinOut;
		pinOut.node = node;
		pinOut.type = fboType;
		pinOut.name = name;
		pinOut.flags = (PinFlags)(flags | PinFlags::OUTPUT);
		pinOut.userp = userp;

		pinOut.SetOnConnected([fbo](PinConnectedArgs args) {
			if (fbo->isAllocated()) {
				ofFbo* fbos = { fbo };
				args.pushPatterns->Push<ofFbo*>(*args.pinOut, &fbos, 1);
			}
		});

		pinOut.SetOnDisconnected([](PinConnectedArgs args) {
			ofFbo* fbos = { nullptr };
			args.pushPatterns->Push<ofFbo*>(*args.pinOut, &fbos, 1);
		});

		return pinOut;
	}

	std::vector<PinInput> UniformsToPinInputs(
		ofShader& shader, 
		nodes::INode* node, 
		std::vector<char>& pinBuffer,
		const std::unordered_set<const char*>& blacklist
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

			// init string with c_str() so it finds the nul char for us
			std::string snippedName(name.c_str());

			if (blacklist.find(snippedName.c_str()) != blacklist.end() 
				|| snippedName.substr(0, 3) == "gl_") 
			{
				continue;
			}

			bool failed = false;
			PinType pinType = PinType::TYPE_NONE;
			uint16_t numCoords = 0;

			// Now make a PinInput for the uniform.
			// First need to figure out what kind of Pin is needed.
			// TODO many more possible types here, including textures.
			switch (uniform_type) {
			case GL_FLOAT:
				pinType = PinType::FLOAT;
				numCoords = 1;
				break;
			case GL_INT:
				pinType = PinType::INT;
				numCoords = 1;
				break;
			case GL_UNSIGNED_INT:
				pinType = PinType::UINT;
				numCoords = 1;
				break;
			case GL_FLOAT_VEC2:
				pinType = PinType::FLOAT;
				numCoords = 2;
				break;
			case GL_FLOAT_VEC3:
				pinType = PinType::FLOAT;
				numCoords = 3;
				break;
			case GL_INT_VEC2:
				pinType = PinType::INT;
				numCoords = 2;
				break;
			case GL_UNSIGNED_INT_VEC2:
				pinType = PinType::UINT;
				numCoords = 2;
				break;
			case GL_SAMPLER_2D_ARB:
				pinType = PinType::FBO_RGBA;
				numCoords = 1;
				break;
			default:
				printf("uniform to PinInput for %s with enum type %d not implemented yet\n", snippedName.data(), uniform_type);
				failed = true;
				break;
			}

			// skip over non-supported Pin types
			if (failed) {
				continue;
			}

			const size_t bytesPerChannel = PinTypeToElementSize(pinType);
			totalBytesNeeded += bytesPerChannel * numCoords;

			if (IsFboPin(pinType)) {
				pin_inputs.push_back(
					SetupInputFboPin(
						pinType,
						node,
						&shader,
						nullptr,
						snippedName,
						snippedName,
						PinInOptions::WithCoords(numCoords)
					)
				);
			} else {
				pin_inputs.push_back(
					SetupInputPin(pinType, node, nullptr, 1, snippedName, PinInOptions::WithCoords(numCoords))
				);
			}
		}

		// Resize the pin buffer now and point each Pin's channel buffer at it.
		pinBuffer.resize(totalBytesNeeded, 0);
		size_t currentByte = 0;

		for (auto& pinIn : pin_inputs) {
			pinIn.node = node;

			// Now that memory has been alloc'd for the uniforms buffer,
			// set each Pin's buffer to point to the right place.
			void* buffer = &pinBuffer[currentByte];
			pinIn.SetBuffer(buffer, 1);
			currentByte += PinTypeToElementSize(pinIn.type) * pinIn.NumCoords();

			// grab the location of the uniform so we can set initial values for the Pin
			GLint uniform_location = glGetUniformLocation(program, pinIn.name.c_str());
			assert(glGetError() == GL_NO_ERROR);

			// fill the Pin's default values
			switch (pinIn.type) {
			case PinType::FLOAT:
				glGetnUniformfv(program, uniform_location, 
					pinIn.NumCoords() * sizeof(float), (float*)buffer);
				break;
			case PinType::INT:
				glGetnUniformiv(program, uniform_location, 
					pinIn.NumCoords() * sizeof(int32_t), (int32_t*)buffer);
				break;
			case PinType::UINT:
				glGetnUniformuiv(program, uniform_location, 
					pinIn.NumCoords() * sizeof(uint32_t), (uint32_t*)buffer);
				break;
			case PinType::FBO_RGBA:
			case PinType::FBO_RGBA16F:
			case PinType::FBO_RED:
				memset(buffer, 0, sizeof(ofFbo**));
				break;
			}

			assert(glGetError() == GL_NO_ERROR);
		}

		return pin_inputs;
	}

	PinInput SetupInputPin(
		PinType pinType,
		nodes::INode* node,
		void* buffer,
		const size_t totalElements,
		const std::string_view name,
		PinInOptions&& options
	) {
#if DEBUG
		// FBO pin types should use SetupInputFboPin() instead in most cases,
		// unless the shader re-bind logic is more complex than
		// simply setting the the texture uniform of the shader.
		if (IsFboPin(pinType)) {
			printf("WARNING: Node %s uses SetupInputPin() for FBO pin %s, "
				"in most cases you should use SetupInputFboPin() instead.\n",
				node->NodeName().data(), name.data());
		}
#endif

		size_t elementSize = options.elementSize > 0 
			? options.elementSize : PinTypeToElementSize(pinType);

		return PinInput(
			pinType,
			name,
			options.description,
			node,
			buffer, 
			totalElements,
			elementSize,
			options.stride > 0 ? options.stride : elementSize * options.numCoords,
			options.offset,
			options.numCoords,
			std::move(options.onValueChanged),
			std::move(options.onValueChanging),
			options.pinMetadata
		);
	}

	PinInput SetupInputFboPin(
		PinType pinType,
		nodes::INode* node,
		ofShader* shader,
		ofFbo** fbo,
		const std::string_view uniformName,
		const std::string_view name,
		PinInOptions&& options
	) {
		assert(IsFboPin(pinType));

		ValueChangedCallback onValueChanged = std::move(options.onValueChanged);
		ValueChangingCallback onValueChanging = std::move(options.onValueChanging);

		return PinInput(
			pinType,
			name,
			options.description,
			node,
			fbo,	// We need the address of the fbo pointer, not the fbo pointer itself
			1,
			sizeof(ofFbo*),
			options.stride > 0 ? options.stride : sizeof(ofFbo*),
			options.offset,
			1,
			[shader, fbo, node, onValueChanged, uniformName]() {
				if (*fbo == nullptr) {
					return;
				}

				shader->begin();

				uint32_t textureLoc = node->Seam().texLocResolver->Bind(&(*fbo)->getTexture());
				shader->setUniformTexture(uniformName.data(), (*fbo)->getTexture(), textureLoc);

				shader->end();

				if (onValueChanged) {
					onValueChanged();
				}
			},
			[fbo, node, onValueChanging]() {
				if (*fbo != nullptr) {
					node->Seam().texLocResolver->Release(&(*fbo)->getTexture());
				}

				if (onValueChanging) {
					onValueChanging();
				}
			},
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

	PinInput SetupVec2InputPin(
		nodes::INode* node,
		glm::vec2& v,
		std::string_view name
	) {
		std::vector<PinInput> childPins = {
			pins::SetupInputPin(PinType::FLOAT, node, &v.x, 1, "X"),
			pins::SetupInputPin(PinType::FLOAT, node, &v.y, 1, "Y"),
		};

		PinInput pinIn = pins::SetupInputPin(PinType::FLOAT, node, &v, 2, name);
		pinIn.SetChildren(std::move(childPins));
		return pinIn;
	}

	PinInput SetupInputFlowPin(
		nodes::INode* node,
		ValueChangedCallback&& callback,
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
		case PinType::FBO_RGBA:
		case PinType::FBO_RGBA16F:
		case PinType::FBO_RED:
			return sizeof(ofFbo*);
		default:
			throw std::runtime_error("Unknown pin type! You need to provide the element size in bytes yourself.");
		}
	}

	PinInput* FindPinInByName(PinInput* pins, size_t pinsSize, std::string_view name) {
		auto it = std::find_if(pins, pins + pinsSize, [name](const PinInput& pinIn) {
			return StrCmpLower(name, pinIn.name);
		});

		if (it != pins + pinsSize) {
			// Found, return it
			return it;
		} else {
			// If not found, try children.
			PinInput* found = nullptr;
			for (size_t i = 0; i < pinsSize && found == nullptr; i++) {
				size_t childrenSize;
				PinInput* children = pins[i].PinInputs(childrenSize);
				found = FindPinInByName(children, childrenSize, name);
			}

			return found;
		}
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

		if (it != pins + pinsSize) {
			// Found, return it
			return it;
		} else {
			// If not found, try children.
			PinOutput* found = nullptr;
			for (size_t i = 0; i < pinsSize && found == nullptr; i++) {
				size_t childrenSize;
				PinOutput* children = pins[i].PinOutputs(childrenSize);
				found = FindPinOutByName(children, childrenSize, name);
			}

			return found;
		}
	}
	
	PinOutput* FindPinOutByName(IOutPinnable* pinnable, std::string_view name) {
		size_t size;
		PinOutput* pins = pinnable->PinOutputs(size);
		return FindPinOutByName(pins, size, name);
	}
}