#pragma once

#include "seam/pins/pin.h"
#include "seam/pins/pinConnection.h"
#include "seam/hash.h"
#include "seam/flags-helper.h"

namespace seam::nodes {
	class INode;
}

namespace seam::pins {
	using PushFunc = std::function<void(
		char* src, 
		size_t src_size_bytes, 
		char* dst, 
		size_t dst_size_bytes,
		size_t element_size
	)>;

	/// Describes a pattern or strategy for pushing values from an output pin to an input pin.
	/// Helps you propagate changes to input pins, 
	/// but you don't have to use them if you have another way to push changes.
	struct Pusher {
		Pusher(std::string_view _name, PushFunc _func)
			: name(_name), func(_func) 
		{
			id = SCHash(name.data(), name.length());
		}

		std::string_view name;
		PushFunc func;
		/// is hashed from the name
		PushId id;

		bool operator<(const Pusher& other) {
			return id < other.id;
		}

		bool operator<(const PushId other_id) {
			return id < other_id;
		}

		bool operator==(const Pusher& other) {
			return id == other.id;
		}
	};

	class PushPatterns {
	public:
		PushPatterns();

		/// Register a Pusher. If another Pusher with the same name already exists, it will be overridden.
		/// \return true if another Pusher was overridden
		bool Register(Pusher&& pusher);
		
		/// @brief Push data from output pins to their attached input pins. 
		/// Use this Push() function for event queue pins.
		/// @param pinOut The output pin to push data from.
		/// @param data Pointer to the first element to be pushed.
		/// @param numElements The number of elements pointed to by the data pointer.
		template <typename T>
		void Push(PinOutput& pinOut, T* data, size_t numElements) {
			const bool isEventQueuePin = flags::AreRaised(pinOut.flags, pins::PinFlags::EVENT_QUEUE);
			// Event queue pins don't use push patterns, they just push to the input pins' vectors
			if (isEventQueuePin) {
				for (auto& conn : pinOut.connections) {
					// Dirty the input node.
					conn.pinIn->node->SetDirty();
					// Event queue input pins should always be event queue pins themselves
					assert((conn.pinIn->flags & pins::PinFlags::EVENT_QUEUE) == pins::PinFlags::EVENT_QUEUE);
					// Push the output pin's data to the back of the input pin's vector
					conn.pinIn->PushEvents(data, numElements);
				}
			} else {
				for (auto& conn : pinOut.connections) {
					// Dirty the input node
					conn.pinIn->node->SetDirty();
					ConvertMultiArgs args(data, pinOut.numCoords, numElements, conn.pinIn);
					conn.convertMulti(args);
				
					// Fire whatever user callback was provided, if any.
					conn.pinIn->Callback();
				}
			}
		}

		void PushFlow(const PinOutput& pinOut) {
			assert(pinOut.type == PinType::FLOW);
			for (auto& conn : pinOut.connections) {
				conn.pinIn->Callback();
			}
		}

		Pusher& Get(PushId push_id);
		Pusher& Get(std::string_view name);

		Pusher& Default();
		void SetDefault(PushId push_id);
		void SetDefault(std::string_view name);
		
	private:
		// push patterns are sorted by pusher id
		std::vector<Pusher> push_patterns;

		Pusher* default_pattern = nullptr;
	};
}
