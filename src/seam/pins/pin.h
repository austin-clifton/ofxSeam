#ifndef PIN_H
#define PIN_H

#include <vector>

#include "ofMain.h"

#include "capnp/message.h"
#include "capnp/serialize-packed.h"
#include "seam/schema/codegen/node-graph.capnp.h"

#include "seam/pins/pinBase.h"
#include "seam/pins/pinTypes.h"
#include "seam/pins/pinInput.h"
#include "seam/pins/pinOutput.h"
#include "seam/pins/iInPinnable.h"
#include "seam/pins/iOutPinnable.h"
#include "seam/pins/pinConnection.h"
#include "seam/pins/push.h"

#include "../notes.h"

namespace seam {

	namespace nodes {
		class INode;
	}

	namespace pins {
		struct PinFloatMeta {
			PinFloatMeta(float _min = -FLT_MAX, float _max = FLT_MAX, RangeType _range = RangeType::LINEAR) {
				min = _min;
				max = _max;
				range_type = _range;
			}

			float min;
			float max;
			RangeType range_type;
		};

		struct PinIntMeta {
			PinIntMeta(int _min = INT_MIN, int _max = INT_MAX, RangeType _range = RangeType::LINEAR) {
				min = _min;
				max = _max;
				range_type = _range;
			}

			int min;
			int max;
			RangeType range_type;
		};

		struct PinUintMeta {
			uint32_t min = 0;
			uint32_t max = UINT_MAX;
			RangeType range_type = RangeType::LINEAR;
		};

		struct PinNoteEventMeta {
			/// specifies what note event types this input pin accepts
			notes::EventTypes note_types;
		};

		PinInput SetupInputPin(
			PinType pinType,
			nodes::INode* node,
			void* channels,
			const size_t numChannels,
			const std::string_view name = "Input",
			size_t elementSizeInBytes = 0,
			void* pinMetadata = nullptr,
			const std::string_view description = ""
		);

		PinInput SetupInputQueuePin(
			PinType pinType,
			nodes::INode* node,
			const std::string_view name = "Input Queue",
			size_t elementSizeInBytes = 0,
			void* pinMetadata = nullptr,
			const std::string_view description = ""
		);

		PinInput SetupInputFlowPin(
			nodes::INode* node,
			std::function<void(void)>&& callback,
			const std::string_view name = "Flow",
			void* pinMetadata = nullptr,
			const std::string_view description = ""
		);

		PinOutput SetupOutputPin(
			nodes::INode* node, 
			PinType type, 
			std::string_view name, 
			PinFlags flags = PinFlags::FLAGS_NONE,
			void* userp = nullptr
		);
		
		PinInput* FindPinInByName(PinInput* pins, size_t pinsSize, std::string_view name);

		PinInput* FindPinInByName(IInPinnable* pinnable, std::string_view name);

		PinOutput* FindPinOutByName(PinOutput* pins, size_t pinsSize, std::string_view name);
		
		PinOutput* FindPinOutByName(IOutPinnable* pinnable, std::string_view name);

		class Vec2PinInput {
        public:
            PinInput SetupVec2Pin(nodes::INode* node, glm::vec2& v, std::string_view name) {
                childPins = {
                    pins::SetupInputPin(PinType::FLOAT, node, &v.x, 1, "X"),
                    pins::SetupInputPin(PinType::FLOAT, node, &v.y, 1, "Y"),
                };

                PinInput pinIn = pins::SetupInputPin(PinType::FLOAT, node, &v, 2, name);
                pinIn.SetChildren(childPins.data(), childPins.size());
                return pinIn;
            }

        private:
            std::array<pins::PinInput, 2> childPins;
        };

		/// Queries a linked shader program's active uniforms and creates a PinInput list from them.
		/// \param shader The linked shader program to query the uniforms of.
		/// \param node The node the shader's Pins will be added to. 
		/// \return The list of PinInputs which maps to the uniforms in the shader.
		/// All IPinInput structs are heap-allocated, and must be freed when no longer in use.
		std::vector<PinInput> UniformsToPinInputs(ofShader& shader, nodes::INode* node);
 		PinType SerializedPinTypeToPinType(seam::schema::PinValue::Which pinType);
	}
};

#endif