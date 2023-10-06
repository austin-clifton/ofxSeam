#pragma once

#include <string_view>
#include <functional> 
#include <assert.h>
#include <cstring>
#include <algorithm>

#include "pinBase.h"
#include "pinTypes.h"

namespace seam::pins {

    class PinOutput;

    class PinInput : public Pin {
    public:
        PinInput() {
            type = PinType::TYPE_NONE;
            channel.data = nullptr;
            channel.size = 0;
        }

        PinInput(PinType _type, 
            const std::string_view _name, 
            const std::string_view _description, 
            nodes::INode* _node,
            void* _channelsData,
            size_t _channelsSize,
            size_t _elementSizeInBytes,
            void* _pinMetadata
            )
        {
            type = _type;
            name = _name;
            description = _description;
            node = _node;
            isQueue = false;
            flags = (PinFlags)(flags | PinFlags::INPUT);
            channel.data = _channelsData;
            channel.size = _channelsSize;
            sizeInBytes = _elementSizeInBytes;
            pinMetadata = _pinMetadata;
        }

        PinInput(PinType _type,
            const std::string_view _name,
            const std::string_view _description,
            nodes::INode* _node,
            size_t _elementSizeInBytes,
            void* _pinMetadata
        ) {
            type = _type;
            name = _name;
            description = _description;
            node = _node;
            isQueue = true;
            flags = (PinFlags)(flags | PinFlags::INPUT | PinFlags::EVENT_QUEUE);
            pinMetadata = _pinMetadata;
            sizeInBytes = _elementSizeInBytes;
            eventQueue.data = malloc(sizeInBytes * MAX_EVENTS);
        }

        PinInput(const std::string_view _name,
            const std::string_view _description,
            nodes::INode* _node,
            std::function<void(void)>&& callback,
            void* _pinMetadata
        )
        {
            type = PinType::FLOW;
            name = _name;
            description = _description;
            node = _node;
            isQueue = false;
            flags = (PinFlags)(flags | PinFlags::INPUT);
            pinMetadata = _pinMetadata;
            Callback = std::move(callback);
            sizeInBytes = 0;
        }

        ~PinInput() {
            // Nothing to do...?
        }

        /// <summary>
        /// Get a pointer to the input pin's raw channels data.
        /// </summary>
        /// <param name="size">will be set to the size of the returned array</param>
        /// <returns>An opaque pointer to the array of channels the input Pin owns.</returns>
        void* GetChannels(size_t& size) {
            size = channel.size;
            return channel.data;
        }

        void* PinMetadata() {
            return pinMetadata;
        }

        template <typename T>
        inline void PushEvents(T* events, size_t numEvents) {
            assert(isQueue);
            size_t offset = eventQueue.size * sizeof(T);
            memcpy_s(((T*)eventQueue.data) + offset, sizeInBytes * MAX_EVENTS - offset, events, numEvents * sizeof(T));
            eventQueue.size = std::min(MAX_EVENTS, eventQueue.size + numEvents);
        }

        inline void* GetEvents(size_t& size) {
            assert(isQueue);
            size = eventQueue.size;
            return eventQueue.data;
        }

        inline void ClearEvents(size_t expected) {
            assert(isQueue);
            // This is just a check in case any multithreading funkiness happens;
            // Pins shouldn't be pushed to outside of an update loop, or events WILL be lost.
            assert(expected == eventQueue.size);
            memset(eventQueue.data, 0, eventQueue.size * sizeInBytes);
            eventQueue.size = 0;
        }

        inline void FlowCallback() {
            assert(type == PinType::FLOW);
            Callback();
        }

        /// push pattern id
        PushId push_id;

        // the output pin which connects to this pin
        // can be nullptr
        PinOutput* connection = nullptr;

    private:
        const size_t MAX_EVENTS = 16;

        struct Channel {
            void* data;
            size_t size;
        };

        struct EventQueue {
            void* data;
            size_t size;
        };

        bool isQueue;

        // Should be set by pin types which have GUI metadata for mins/maxes etc.
        void* pinMetadata;

        // Size of each element pointed to by void* members.
        size_t sizeInBytes;

        union {
            // Only used by non-event non-flow pins.
            Channel channel;

            // Only used by event queue inputs.
            EventQueue eventQueue;
        };

        // Only used by flow pins. Exists outside the union so the copy constructor can still exist.
        std::function<void(void)> Callback;
    };
}