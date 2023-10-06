#pragma once

#include "pinTypes.h"
#include <string>

namespace seam::nodes {
    class INode;
}

// TODO MOVE ME
namespace seam {
    enum NodePropertyType : uint8_t {
		PROP_BOOL,
		PROP_CHAR,
		PROP_FLOAT,
		PROP_INT,
		PROP_UINT,
		PROP_STRING
	};

	struct NodeProperty {
		NodeProperty(const std::string& _name, NodePropertyType _type, void* _data, size_t _count) {
			name = _name;
			type = _type;
			data = _data;
			count = _count;
		}

		std::string name;
		NodePropertyType type;
		void* data;
		size_t count;
	};
}

namespace seam::pins {
    using PinId = uint64_t;
    using PushId = uint32_t; 

    // a bitmask enum for marking boolean properties of a Pin
    enum PinFlags : uint16_t {
        FLAGS_NONE = 0,
        ///  input pins must have this flag raised;
        /// incompatible with PinFlags::OUTPUT
        INPUT = 1 << 0,

        /// output pins must have this flag raised;
        /// incompatible with PinFlags::INPUT
        OUTPUT = 1 << 1,

        /// Pins which receive feedback textures (to be used by the next update frame) must raise this flag.
        /// only INPUT pins can be marked as feedback
        FEEDBACK = 1 << 2,

        /// Only variable-sized INPUT Pins can raise this flag.
        /// Causes the node to receive a stream of pushed events to be drained during update,
        /// rather than treating the Pin's underlying vector as value channels.
        /// Is useful for receiving note events or any kind of event stream.
        EVENT_QUEUE = 1 << 3,
    };


    // TODO move me
    enum RangeType : uint8_t {
        LINEAR,
        // logarithmic
        LOG,
    };

    // base struct for pin types, holds metadata about the pin
    struct Pin {
        PinId id = 0;

        PinType type;

        // TODO make name and description string-view-ifiable again somehow

        // human-readable Pin name for display purposes
        std::string name;
        // human-readable Pin description for display purposes
        std::string description;

        // Pins keep track of the node they are associated with;
        // is expected to have a valid pointer value
        nodes::INode* node = nullptr;

        PinFlags flags = (PinFlags)0;

        virtual ~Pin() { }
    };

    NodePropertyType PinTypeToPropType(PinType pinType);
    size_t PinTypeToElementSize(PinType type);
}