#include "pinInput.h"

namespace seam::pins {
    class VectorPinInputBase {
    public:
        virtual ~VectorPinInputBase() { }
        virtual void UpdateSize(PinInput* pinIn, size_t newSize) = 0;
    };

    template <typename T>
    class VectorPinInput : public VectorPinInputBase {
    public:
        struct Options {
            std::function<void(VectorPinInput*)> onSizeChanged;
            std::function<void(VectorPinInput*, size_t)> onPinAdded;
            std::function<void(VectorPinInput*, size_t)> onPinRemoved;
            std::function<void(VectorPinInput*, size_t)> onPinChanged;
        };

        PinInput SetupVectorPin(
            nodes::INode* node, 
            PinType pinType, 
            const std::string_view name,
            size_t initialSize = 0
        ) {
            assert(sizeof(T) == PinTypeToElementSize(pinType));
            PinInput pinIn = SetupInputPin(pinType, node, v.data(), v.size(), name);
            pinIn.flags = (PinFlags)(pinIn.flags | PinFlags::VECTOR);
            pinIn.seamp = this;

            UpdateSize(&pinIn, initialSize);
            return pinIn;
        }

        void SetCallbackOptions(Options&& _options) {
            options = std::move(_options);

            if (options.onPinChanged) {
                // Make sure all existing pins' callbacks are updated.
                for (size_t i = 0; i < childPins.size(); i++) {
                    childPins[i].SetCallback([this, i]() {
                        options.onPinChanged(this, i);
                    });
                }
            } else {
                // Clear existing callbacks
                for (size_t i = 0; i < childPins.size(); i++) {
                    childPins[i].SetCallback(std::function<void(void)>());
                }
            }
        }

        const std::vector<T>& Get() {
            return v;
        }

        void UpdateSize(PinInput* pinIn, size_t newSize) override {
            assert((pinIn->flags & PinFlags::VECTOR) > 0);

            v.resize(newSize);
            pinIn->SetBuffer(v.data(), v.size());

            // Set up any new additional child pins.
            while (childPins.size() < newSize) {
                std::function<void(void)> changedCb;
                if (options.onPinChanged) {
                    size_t pinIndex = childPins.size();
                    changedCb = [this, pinIndex]() {
                        options.onPinChanged(this, pinIndex);
                    };
                } 

                childPins.push_back(SetupInputPin(pinIn->type, pinIn->node,
                    &v[childPins.size()], 1, std::to_string(childPins.size()), 
                    PinInOptions(std::move(changedCb)))
                );

                if (options.onPinAdded) {
                    options.onPinAdded(this, childPins.size() - 1);
                }
            }

            // Cut off any pins that don't need to exist anymore.
            if (childPins.size() > newSize) {
                if (options.onPinRemoved) {
                    while (childPins.size() > newSize) {
                        options.onPinRemoved(this, childPins.size() - 1);
                        childPins.pop_back();
                    }
                } else {
                    childPins.resize(newSize);
                }
            }

            assert(v.size() == childPins.size());

            pinIn->SetChildren(childPins.data(), childPins.size());

            // Assume the pointer to v moved, and re-set each child pin's buffer.
            for (size_t i = 0; i < v.size(); i++) {
                childPins[i].SetBuffer(&v[i], 1);

                size_t size;
                void* buff = childPins[i].GetChannels(size);

                printf("%llu v = %p, buff = %p\n", i, (void*)&v[i], buff);
            }

            pinIn->RecacheInputConnections();

            if (options.onSizeChanged) {
                options.onSizeChanged(this);
            }
        }

    private:
        std::vector<pins::PinInput> childPins;
        std::vector<T> v;
        Options options;
    };
}