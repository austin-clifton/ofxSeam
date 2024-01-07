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
#include "seam/pins/uniformsPin.h"
#include "seam/pins/vectorPin.h"

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
			PinUintMeta(uint32_t _min = 0, uint32_t _max = UINT_MAX) {
				min = _min;
				max = _max;
			}

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
			void* buffer,
			const size_t totalElements,
			const std::string_view name,
			PinInOptions&& options = PinInOptions()
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
			uint16_t numCoords = 1,
			PinFlags flags = PinFlags::FLAGS_NONE,
			void* userp = nullptr
		);
		
		PinInput* FindPinInByName(PinInput* pins, size_t pinsSize, std::string_view name);

		PinInput* FindPinInByName(IInPinnable* pinnable, std::string_view name);

		PinOutput* FindPinOutByName(PinOutput* pins, size_t pinsSize, std::string_view name);
		
		PinOutput* FindPinOutByName(IOutPinnable* pinnable, std::string_view name);

		PinInput SetupVec2InputPin(
			nodes::INode* node,
			glm::vec2& v,
			std::string_view name
		);

		/// Queries a linked shader program's active uniforms and creates a PinInput list from them.
		/// \param shader The linked shader program to query the uniforms of.
		/// \param node The node the shader's Pins will be added to. 
		/// \param pinBuffer Is used to allocate a buffer for pin values. 
		///	This needs the same life cycle scope as the returned list!
		/// \return The list of PinInputs which maps to the uniforms in the shader.
		/// All IPinInput structs are heap-allocated, and must be freed when no longer in use.
		std::vector<PinInput> UniformsToPinInputs(
			ofShader& shader, 
			nodes::INode* node, 
			std::vector<char>& pinBuffer
		);
		
 		PinType SerializedPinTypeToPinType(seam::schema::PinValue::Which pinType);
	}
};

#endif