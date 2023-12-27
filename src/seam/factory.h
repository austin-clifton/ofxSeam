#pragma once

#include "nodes/iNode.h"

namespace seam {
	class EventNodeFactory {
	public:
		using CreateFunc = std::function<nodes::INode*()>;

		/// holds metadata about an event node, and a function that can create one
		struct Generator {
			/// the node's hash, calculated from its name
			seam::nodes::NodeId node_id;

			/// the node's reported human readable name
			std::string_view node_name;

			/// list of unique input types
			std::vector<pins::PinType> pin_inputs;

			/// list of unique output types
			std::vector<pins::PinType> pin_outputs;

			/// can create a unique instance of the node described by this struct
			CreateFunc Create;

			bool operator <(const Generator& other) {
				return node_id < other.node_id;
			}

			bool operator <(const seam::nodes::NodeId other) {
				return node_id < other;
			}

			bool operator==(const seam::nodes::NodeId other) {
				return node_id == other;
			}
		};

		template <typename T>
		static EventNodeFactory::CreateFunc MakeCreate() {
			return [] {
				return new T();
			};
		}

		EventNodeFactory();

		~EventNodeFactory();

		nodes::INode* Create(seam::nodes::NodeId node_id);

		/// \return the NodeId of the new node to create, or 0 if no new node was requested
		seam::nodes::NodeId DrawCreatePopup(
			pins::PinType input_type = pins::PinType::TYPE_NONE, 
			pins::PinType output_type = pins::PinType::TYPE_NONE
		);

		/// creates a node and puts its metadata into a generator
		/// \return false if the node id is already registered, otherwise true
		bool Register(CreateFunc&& Create);
	private:

		bool generators_sorted = false;
		std::vector<Generator> generators;
		
		// @brief Generators but sorted by name for GUI usage
		std::vector<Generator*> guiGenerators;
	};
}