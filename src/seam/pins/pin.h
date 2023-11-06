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

		struct PinInOptions {
			PinInOptions() { }

			PinInOptions(std::function<void(void)>&& _callback) {
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
			std::function<void(void)> callback;
		};

		PinInput SetupInputPin(
			PinType pinType,
			nodes::INode* node,
			void* channels,
			const size_t numChannels,
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

		/// @brief Use this function to set up an Input pin which is backed by a vector,
		/// and the size of the vector should be user configurable using the Seam GUI.
		/// @tparam T Should match the PinType argument
		/// @param pinType 
		/// @param node 
		/// @param vector 
		/// @param name 
		/// @return 
		template <typename T>
		PinInput SetupVectorPin(
			PinType pinType,
			nodes::INode* node,
			std::vector<T>* vector,
			const std::string_view name
		) {
			assert(sizeof(T) == PinTypeToElementSize(pinType));
			PinInput pinIn = SetupInputPin(pinType, node, vector->data(), vector->size(), name);
			pinIn.flags = (PinFlags)(pinIn.flags | PinFlags::VECTOR);
			pinIn.seamp = vector;
			return pinIn;
		}

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