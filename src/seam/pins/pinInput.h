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
    class VectorPinInput;

    struct PinInOptions {
        PinInOptions() { }

        PinInOptions(std::function<void(void)>&& _callback) {
            callback = std::move(_callback);
        }

        PinInOptions(size_t _stride, 
            size_t _offset = 0,
            std::function<void(void)>&& _callback = std::function<void(void)>()
        ) {
            stride = _stride;
            offset = _offset;
            callback = std::move(_callback);
        }

        PinInOptions(
            std::string_view _description,
            void* _pinMetadata = nullptr,
            std::function<void(void)>&& _callback = std::function<void(void)>(),
            size_t _elementSize = 0
        ) {
            pinMetadata = _pinMetadata;
            description = _description;
            elementSize = _elementSize;
            callback = std::move(_callback);
        }

        void* pinMetadata = nullptr;
        std::string_view description;
        size_t elementSize = 0;
        size_t stride = 0;
        size_t offset = 0;
        std::function<void(void)> callback;
    };

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
            size_t _stride,
            size_t _offset,
            std::function<void(void)>&& _callback,
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
            stride = _stride;
            offset = _offset;
            callback = std::move(_callback);
            pinMetadata = _pinMetadata;

            assert(stride >= sizeInBytes);
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
            std::function<void(void)>&& _callback,
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
            callback = std::move(_callback);
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

        inline void Callback() {
            if (callback) {
                callback();
            }
        }

        inline void SetCallback(std::function<void(void)>&& cb) {
            callback = cb;
        }

        PinInput* PinInputs(size_t &size) override {
            size = childPins.size();
            return childPins.data();
        }

        void SetChildren(std::vector<PinInput>&& children) {
            childPins = children;
        }

        inline size_t Stride() {
            return stride;
        }

        inline size_t Offset() {
            return offset;
        }

        /// push pattern id
        PushId push_id;

        // the output pin which connects to this pin
        // can be nullptr
        PinOutput* connection = nullptr;

        friend class pins::VectorPinInput;

    private:
        const static size_t MAX_EVENTS;

        bool isQueue = false;

        // Size of each element pointed to by void* members.
        size_t sizeInBytes = 0;

        size_t stride = 0;
        size_t offset = 0;

        void* buffer = nullptr;
        size_t totalElements = 0;

        std::vector<PinInput> childPins;

        std::function<void(void)> callback;

        // Should be set by pin types which have GUI metadata for mins/maxes etc.
        void* pinMetadata;
    };
}