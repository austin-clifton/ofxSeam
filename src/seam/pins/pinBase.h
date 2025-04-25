#pragma once

#include <string>
#include <functional>

#include "seam/pins/pinTypes.h"
#include "seam/flagsHelper.h"
#include "seam/idsDistributor.h"
#include "seam/properties/nodePropertyType.h"

namespace seam::nodes {
    class INode;
}

namespace seam::pins {
    class PinInput;
    class PinOutput;
    class PushPatterns;

	struct PinConnectedArgs {
		PinInput* pinIn;
		PinOutput* pinOut;
		PushPatterns* pushPatterns;
	};

    using ConnectedCallback = std::function<void(PinConnectedArgs)>;
    using DisconnectedCallback = std::function<void(PinConnectedArgs)>;

    using PinId = uint64_t;
    using PushId = uint32_t; 

    // a bitmask enum for marking boolean properties of a Pin
    enum class PinFlags : uint16_t {
        None = 0,
        /// @brief Input pins must have this flag raised;
        /// incompatible with PinFlags::Output
        Input = 1 << 0,

        /// @brief Output pins must have this flag raised;
        /// incompatible with PinFlags::Input
        Output = 1 << 1,

        /// @brief Pins which receive feedback textures (to be used by the next update frame) must raise this flag.
        /// only INPUT pins can be marked as feedback
        Feedback = 1 << 2,

        /// @brief Only variable-sized INPUT Pins can raise this flag.
        /// Causes the node to receive a stream of pushed events to be drained during update,
        /// rather than treating the Pin's underlying vector as value channels.
        /// Is useful for receiving note events or any kind of event stream.
        EventQueue = 1 << 3,

        /// @brief Valid for Input pins only; means Pin channels are resizable,
        /// and the void* backing the Pin points to a vector<T>
        Vector = 1 << 4,
    };

    DeclareFlagOperators(PinFlags, uint16_t);

    // TODO move me
    enum class RangeType : uint8_t {
        Linear,
        // logarithmic
        Log,
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