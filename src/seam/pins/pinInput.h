#pragma once

#include <string_view>
#include <functional> 
#include <assert.h>
#include <cstring>
#include <algorithm>

#include "pinBase.h"
#include "pinTypes.h"
#include "seam/pins/iInPinnable.h"
#include "seam/pins/pinOutput.h"

namespace seam::pins {
    class PinInput final : public Pin, public IInPinnable {
    public:
        PinInput() {
            type = PinType::TYPE_NONE;
            buffer = nullptr;
            totalElements = 0;
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
            buffer = _channelsData;
            totalElements = _channelsSize;
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
            buffer = malloc(sizeInBytes * MAX_EVENTS);
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

        ~PinInput();

        /// <summary>
        /// Get a pointer to the input pin's raw channels data.
        /// </summary>
        /// <param name="size">will be set to the size of the returned array</param>
        /// <returns>An opaque pointer to the array of channels the input Pin owns.</returns>
        void* GetChannels(size_t& size) {
            size = totalElements;
            return buffer;
        }

        void* PinMetadata() {
            return pinMetadata;
        }

        template <typename T>
        inline void PushEvents(T* events, size_t numEvents) {
            assert(isQueue);
            size_t offset = totalElements * sizeof(T);
            memcpy_s(((T*)buffer) + offset, sizeInBytes * MAX_EVENTS - offset, events, numEvents * sizeof(T));
            totalElements = std::min(MAX_EVENTS, totalElements + numEvents);
        }

        inline void* GetEvents(size_t& size) {
            assert(isQueue);
            size = totalElements;
            return buffer;
        }

        inline void ClearEvents(size_t expected) {
            assert(isQueue);
            // This is just a check in case any multithreading funkiness happens;
            // Pins shouldn't be pushed to outside of an update loop, or events WILL be lost.
            assert(expected == totalElements);
            memset(buffer, 0, totalElements * sizeInBytes);
            totalElements = 0;
        }

        /// @brief Bytes needed to contain the input's buffer
        inline size_t BufferSize() {
            return sizeInBytes * totalElements;
        }

        inline void SetBuffer(void* buff, size_t elements) {
            buffer = buff;
            totalElements = elements;
        }

        inline void FlowCallback() {
            assert(type == PinType::FLOW);
            Callback();
        }

        PinInput* PinInputs(size_t &size) override {
            size = childrenSize;
            return childInputs;
        }

        void SetChildren(PinInput* _childInputs, size_t _childrenSize) {
            childInputs = _childInputs;
            childrenSize = _childrenSize;
        }

        /// push pattern id
        PushId push_id;

        // the output pin which connects to this pin
        // can be nullptr
        PinOutput* connection = nullptr;

    private:
        const static size_t MAX_EVENTS;

        bool isQueue;

        // Should be set by pin types which have GUI metadata for mins/maxes etc.
        void* pinMetadata;

        // Size of each element pointed to by void* members.
        size_t sizeInBytes;

        void* buffer;
        size_t totalElements;

        PinInput* childInputs = nullptr;
        size_t childrenSize = 0;

        // Only used by flow pins. Exists outside the union so the copy constructor can still exist.
        std::function<void(void)> Callback;
    };
}