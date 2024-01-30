#pragma once

#include <string>

#include "seam/pins/pinTypes.h"
#include "seam/idsDistributor.h"
#include "seam/properties/nodePropertyType.h"

namespace seam::nodes {
    class INode;
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

        /// @brief Valid for Input pins only; means Pin channels are resizable,
        /// and the void* backing the Pin points to a vector<T>
        VECTOR = 1 << 4,
    };

    // TODO move me
    enum RangeType : uint8_t {
        LINEAR,
        // logarithmic
        LOG,
    };

    // base struct for pin types, holds metadata about the pin
    struct Pin {
        PinId id;

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
 
        /// @brief Don't touch! For seam internal usage.
        void* seamp = nullptr;

        /// @brief Unused pointer for user data, if any is needed.
        void* userp = nullptr;

        virtual ~Pin() { }

        Pin() {
            id = IdsDistributor::GetInstance().NextPinId();
        }
    };

    props::NodePropertyType PinTypeToPropType(PinType pinType);
    size_t PinTypeToElementSize(PinType type);
}