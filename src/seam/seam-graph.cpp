#include "seam/seam-graph.h"
#include "seam/hash.h"
#include "seam/properties/nodeProperty.h"
#include "seam/pins/pinConnection.h"

using namespace seam;
using namespace seam::nodes;
using namespace seam::pins;

namespace {
	template <typename T>
	void Erase(std::vector<T*>& v, T* item) {
		auto it = std::find(v.begin(), v.end(), item);
		if (it != v.end()) {
			v.erase(it);
		}
	}

    void PrintGraph(const seam::schema::NodeGraph::Reader& reader) {
		printf("seam graph file %s\n", reader.getName().cStr());

		// First list Nodes.
		for (const auto& node : reader.getNodes())
		{
			printf("Node %llu %s [%s]:\n", node.getId(), node.getDisplayName().cStr(), node.getNodeName().cStr());
			printf("\tPosition (%f, %f)\n", node.getPosition().getX(), node.getPosition().getY());
			printf("\tInputs:\n");
			std::stringstream ss;
			for (const auto& pin_in : node.getInputPins()) {
				ss << "\t\t" << pin_in.getId() << "\t" << pin_in.getName().cStr() << " [";
				for (const auto& chan : pin_in.getChannels()) {
					if (chan.isBoolValue()) {
						ss << chan.getBoolValue() << " ";
					} else if (chan.isFloatValue()) {
						ss << chan.getFloatValue() << " ";
					} else if (chan.isIntValue()) {
						ss << chan.getIntValue() << " ";
					} else if (chan.getUintValue()) {
						ss << chan.getUintValue() << " ";
					} else if (chan.isStringValue()) {
						ss << chan.getStringValue().cStr() << " ";
					}
				}
				ss << "]\n";
			}
			printf("%s", ss.str().c_str());

			printf("\tOutputs:\n");
			for (const auto& pin_out : node.getOutputPins()) {
				printf("\t\t%llu\t%s\n", pin_out.getId(), pin_out.getName().cStr());
			}
		}

		// Now list Connections.
		printf("\tConnections (out id, in id):\n");
		for (const auto& conn : reader.getConnections()) {
			printf("\t\t(%llu, %llu)\n", conn.getOutId(), conn.getInId());
		}
	}
    
	using PinInputsBuilder 	= ::capnp::List<::seam::schema::PinIn, ::capnp::Kind::STRUCT>::Builder;
	using PinOutputsBuilder = ::capnp::List<::seam::schema::PinOut, ::capnp::Kind::STRUCT>::Builder;
	using PinInputsReader	= ::capnp::List<::seam::schema::PinIn, ::capnp::Kind::STRUCT>::Reader;
	using PinOutputsReader 	= ::capnp::List<::seam::schema::PinOut, ::capnp::Kind::STRUCT>::Reader;

	void SerializePinInputsList(PinInputsBuilder& inputsBuilder, PinInput* inputs, size_t size) 
	{
		// Serialize input pins
		for (size_t i = 0; i < size; i++) {
			auto& pinIn = inputs[i];
			auto builder = inputsBuilder[i];
			builder.setName(pinIn.name);
			builder.setId(pinIn.id);

			size_t childrenSize;
			PinInput* children = pinIn.PinInputs(childrenSize);

			if (childrenSize > 0) {
				auto serializedChildren = builder.initChildren(childrenSize);
				SerializePinInputsList(serializedChildren, children, childrenSize);
			}

			// Don't serialize channels if this input type doesn't have serializable state
			if (pinIn.type == PinType::FLOW
				|| pinIn.type == PinType::FBO
				|| pinIn.type == PinType::NOTE_EVENT
			) {
				continue;
			}

			size_t channelsSize;
			void* channels = pinIn.GetChannels(channelsSize);
			auto channelsBuilder = builder.initChannels(channelsSize);

			seam::props::Serialize(channelsBuilder, PinTypeToPropType(pinIn.type), channels, channelsSize);
		}
	}

	void SerializePinOutputsList(PinOutputsBuilder& outputsBuilder, PinOutput* outputs, size_t size,
		std::vector<std::pair<PinId, PinId>>& connections) 
	{
		for (size_t i = 0; i < size; i++) {
			auto pinOut = outputs[i];
			auto builder = outputsBuilder[i];

			builder.setId(pinOut.id);
			builder.setName(pinOut.name);

			// Remember what connections this output has so we can serialize all the connections after this loop.
			for (size_t conn_index = 0; conn_index < pinOut.connections.size(); conn_index++) {
				auto& conn = pinOut.connections[conn_index];
				connections.push_back(std::make_pair(pinOut.id, conn.input->id));
			}
						
			size_t childrenSize;
			PinOutput* children = pinOut.PinOutputs(childrenSize);
			if (childrenSize > 0) {
				auto serializedChildren = builder.initChildren(childrenSize);
				SerializePinOutputsList(serializedChildren, children, childrenSize, connections);
			}
		}
	}

	void DeserializePinInput(const seam::schema::PinIn::Reader& serializedPin, PinInput* pinIn) {
		pinIn->id = serializedPin.getId();
		const auto serializedChannels = serializedPin.getChannels();

		if (serializedChannels.size()) {
			size_t channels_size;
			void* channels = pinIn->GetChannels(channels_size);

			seam::props::Deserialize(serializedChannels, PinTypeToPropType(pinIn->type), channels, channels_size);
		}

		size_t childrenSize;
		PinInput* children = pinIn->PinInputs(childrenSize);

		for (const auto& child : serializedPin.getChildren()) {
			PinInput* match = FindPinInByName(children, childrenSize, child.getName().cStr());
			if (match != nullptr) {
				DeserializePinInput(child, match);
			} else {
				printf("DeserializePinInput(): couldn't find matching pin for %s\n", child.getName().cStr());
			}
		}
	}

	PinId UpdatePinMap(PinInput* pinIn, std::map<PinId, Pin*>& pinMap) {
		pinMap.emplace(pinIn->id, pinIn);
		size_t maxId = pinIn->id;

		size_t childrenSize;
		PinInput* children = pinIn->PinInputs(childrenSize);
		for (size_t i = 0; i < childrenSize; i++) {
			maxId = std::max(UpdatePinMap(&children[i], pinMap), maxId);
		}

		return maxId;
	}

	PinId UpdatePinMap(PinOutput* pinOut, std::map<PinId, Pin*>& pinMap) {
		pinMap.emplace(pinOut->id, pinOut);
		size_t maxId = pinOut->id;

		size_t childrenSize;
		PinOutput* children = pinOut->PinOutputs(childrenSize);
		for (size_t i = 0; i < childrenSize; i++) {
			maxId = std::max(UpdatePinMap(&children[i], pinMap), maxId);
		}

		return maxId;
	}

	void DeserializePinOutput(const seam::schema::PinOut::Reader& serializedPin, PinOutput* pinOut) {
		pinOut->id = serializedPin.getId();
		
		size_t childrenSize;
		PinOutput* children = pinOut->PinOutputs(childrenSize);

		for (const auto& child : serializedPin.getChildren()) {
			PinOutput* match = FindPinOutByName(children, childrenSize, child.getName().cStr());
			if (match != children + childrenSize) {
				DeserializePinOutput(child, match);
			}
		}
	}
}

SeamGraph::SeamGraph(const ofSoundStreamSettings& soundSettings) {
    factory = new EventNodeFactory(soundSettings);
}

SeamGraph::~SeamGraph() {
    NewGraph();

    if (factory != nullptr) {
        delete factory;
        factory = nullptr;
    }
}

void SeamGraph::Draw() {
    DrawParams params;
    params.time = ofGetElapsedTimef();
    params.delta_time = ofGetLastFrameTime();

    for (auto n : nodesToDraw) {
        n->Draw(&params);
    }
}

void SeamGraph::UpdateVisibleNodeGraph(INode* n, UpdateParams* params) {
    // Traverse parents and update them before traversing this node.
    // Parents are sorted by update order, so that any "shared parents" are updated first
    int16_t last_update_order = -1;
    static std::vector<INode*> in_use_parents;
    n->InUseParents(in_use_parents);

    for (auto p : in_use_parents) {
        // assert(last_update_order <= p->update_order);
        last_update_order = p->update_order;
        UpdateVisibleNodeGraph(p, params);
    }

    // now, this node can update, if it's dirty
    if (n->dirty) {
        n->Update(params);
        n->dirty = false;

        // if this is a visual node, it will need to be re-drawn now
        if (n->IsVisual()) {
            nodesToDraw.push_back(n);
        }
    }
}

void SeamGraph::Update() {
    // Traverse the parent tree of each visible visual node and determine what needs to update
    nodesToDraw.clear();

    // Before traversing the graphs of visible nodes, dirty nodes which update every frame
    for (auto n : nodesUpdateOverTime) {
        // Set the dirty flag directly so children aren't affected;
        // just because the node updates over time doesn't mean it will change every frame,
        // for instance in the case of a timer which fires every XX seconds
        n->dirty = true;
    }

    // Clear the per-frame allocation pool
    alloc_pool.Clear();

    // Assemble update parameters
    UpdateParams params;
    params.push_patterns = &push_patterns;
    params.alloc_pool = &alloc_pool;
    params.time = ofGetElapsedTimef();
    params.delta_time = ofGetLastFrameTime();

    // Traverse Nodes which must be updated every frame;
    // these are usually nodes which handle some kind of external input and/or can be dirtied by other threads
    for (auto n : nodesUpdateEveryFrame) {
        // Assume the node will dirty itself if it needs to Update()
        if (n->dirty) {
            n->Update(&params);
        }
    }

    // Traverse the visible node graph and update nodes that need to be updated
    for (auto n : visibleNodes) {
        UpdateVisibleNodeGraph(n, &params);
    }
}

void SeamGraph::ProcessAudio(ofSoundBuffer& buffer) {
    for (auto n : audioNodes) {
        n->ProcessAudio(buffer);
    }
}

void SeamGraph::NewGraph() {
    for (size_t i = 0; i < nodes.size(); i++) {
        delete nodes[i];
    }

    nodes.clear();
    nodesToDraw.clear();
    visibleNodes.clear();
    nodesUpdateOverTime.clear();
    nodesUpdateEveryFrame.clear();
    audioNodes.clear();

    IdsDistributor::GetInstance().ResetIds();
}

INode* SeamGraph::CreateAndAdd(seam::nodes::NodeId node_id) {
	INode* node = factory->Create(node_id);
	if (node != nullptr) {
		nodes.push_back(node);

		if (node->IsVisual()) {
			// probably temporary: add to the list of visible nodes up front
			auto it = std::upper_bound(visibleNodes.begin(), visibleNodes.end(), node, &INode::CompareUpdateOrder);
			visibleNodes.insert(it, node);
		}

		if (node->UpdatesEveryFrame()) {
			nodesUpdateEveryFrame.push_back(node);
		} 
		if (node->UpdatesOverTime()) {
			nodesUpdateOverTime.push_back(node);
		}

		// Does this Node process audio?
		IAudioNode* audioNode = dynamic_cast<IAudioNode*>(node);
		if (audioNode != nullptr) {
			audioNodes.push_back(audioNode);
		}
	}

	return node;
}

INode* SeamGraph::CreateAndAdd(const std::string_view node_name) {
	return CreateAndAdd(SCHash(node_name.data(), node_name.length()));
}

void SeamGraph::DeleteNode(INode* node) {
    size_t size;

    // Undo Input connections.
    PinInput* pinInputs = node->PinInputs(size);
    for (size_t i = 0; i < size; i++) {
        if (pinInputs[i].connection != nullptr) {
            Disconnect(&pinInputs[i], pinInputs[i].connection);
        }
    }

    // Undo Output connections.
    PinOutput* pinOutputs = node->PinOutputs(size);
    for (size_t i = 0; i < size; i++) {
        // Loop in reverse since connections will be removed as we go.
        for (size_t j = pinOutputs[i].connections.size(); j > 0; j--) {
            Disconnect(pinOutputs[i].connections[j - 1].input, &pinOutputs[i]);
        }
    }

    // Remove the Node from whatever lists it's in...
    Erase(nodes, node);
    Erase(nodesToDraw, node);
    Erase(visibleNodes, node);
    Erase(nodesUpdateEveryFrame, node);
    Erase(nodesUpdateOverTime, node);
    
    IAudioNode* audioNode = dynamic_cast<IAudioNode*>(node);
    if (audioNode != nullptr) {
        Erase(audioNodes, audioNode);
    }

    // Finally, delete the Node.
    delete node;
}

bool SeamGraph::Connect(PinInput* pinIn, PinOutput* pinOut) {
	assert((pinIn->flags & PinFlags::INPUT) == PinFlags::INPUT);
	assert((pinOut->flags & PinFlags::OUTPUT) == PinFlags::OUTPUT);

	INode* parent = pinOut->node;
	INode* child = pinIn->node;

	// Validations: make sure pin_out is actually an output pin of node_out, and same for the input
	assert(child->FindPinInput(pinIn->id) != nullptr);
	assert(parent->FindPinOutput(pinOut->id) != nullptr);

	// create the connection
	pinOut->connections.push_back(PinConnection(pinIn, pinOut->type));
	pinIn->connection = pinOut;

	PinConnectedArgs connectedArgs;
	connectedArgs.pinIn = pinIn;
	connectedArgs.pinOut = pinOut;
	connectedArgs.pushPatterns = &push_patterns;

	parent->OnPinConnected(connectedArgs);
	child->OnPinConnected(connectedArgs);

	// give the input pin the default push pattern
	pinIn->push_id = push_patterns.Default().id;

	// add to each node's parents and children list
	// if this connection rearranged the node graph, 
	// its traversal order will need to be recalculated
	const bool is_new_child = parent->AddChild(child);
	const bool is_new_parent = child->AddParent(parent);
	const bool rearranged = is_new_parent || is_new_child;

	if (rearranged) {
		RecalculateTraversalOrder(child);
	}

	return true;
}

bool SeamGraph::Disconnect(PinInput* pinIn, PinOutput* pinOut) {
    assert((pinIn->flags & PinFlags::INPUT) == PinFlags::INPUT);
	assert((pinOut->flags & PinFlags::OUTPUT) == PinFlags::OUTPUT);

	INode* parent = pinOut->node;
	INode* child = pinIn->node;

	assert(child->FindPinInput(pinIn->id) != nullptr);
	assert(parent->FindPinOutput(pinOut->id) != nullptr);

	pinIn->connection = nullptr;

	// remove from pinOut's connections list
	size_t i = 0;
	for (; i < pinOut->connections.size(); i++) {
		if (pinOut->connections[i].input == pinIn) {
			break;
		}
	}
	assert(i < pinOut->connections.size());
	pinOut->connections.erase(pinOut->connections.begin() + i);

	parent->OnPinDisconnected(pinIn, pinOut);
	child->OnPinDisconnected(pinIn, pinOut);

	// track if we need to update traversal order
	bool rearranged = false;

	// remove or decrement from receivers / transmitters lists
	{
		auto it = std::find(parent->children.begin(), parent->children.end(), child);
		assert(it != parent->children.end());
		if (it->conn_count == 1) {
			parent->children.erase(it);
			rearranged = true;
		} else {
			it->conn_count -= 1;
		}
	}

	{
		auto it = std::find(child->parents.begin(), child->parents.end(), parent);
		assert(it != child->parents.end());
		if (it->conn_count == 1) {
			child->parents.erase(it);
			rearranged = true;
		} else {
			it->conn_count -= 1;
		}
	}

	if (rearranged) {
		// the child node and its children need to recalculate draw and update order now
		RecalculateTraversalOrder(child);
	}

	return true;
}

int16_t SeamGraph::RecalculateUpdateOrder(INode* node) {
	// update order is always max of parents' update order + 1
	if (node->update_order != -1) {
		return node->update_order;
	}

	// TODO! deal with feedback pins

	int16_t max_parents_update_order = -1;
	for (auto parent : node->parents) {
		max_parents_update_order = std::max(
			max_parents_update_order,
			// TODO checking the parent update order may not be necessary,
			// if the children list is sorted for its recursive traversal
			RecalculateUpdateOrder(parent.node)
		);
	}
	node->update_order = max_parents_update_order + 1;

	// recursively update children
	for (auto child : node->children) {
		RecalculateUpdateOrder(child.node);
	}

	return node->update_order;
}

void SeamGraph::InvalidateChildren(INode* node) {
    // if this node has already been invalidated, don't go over it again
    if (node->update_order == -1) {
        return;
    }

	node->update_order = -1;
	for (auto child : node->children) {
		InvalidateChildren(child.node);
	}
}

void SeamGraph::RecalculateTraversalOrder(INode* node) {
	// invalidate this node and its children
	InvalidateChildren(node);
	RecalculateUpdateOrder(node);
}

bool SeamGraph::SaveGraph(const std::string_view filename, const std::vector<INode*>& nodesToSave) {
    // Build maps of IDs to nodes, input pins, and output pins.
    // ID assignment will happen now for anything that isn't already assigned.
    std::map<seam::nodes::NodeId, INode*> nodeMap;
    std::map<seam::pins::PinId, Pin*> inputPinMap;
    std::map<seam::pins::PinId, Pin*> outputPinMap;
    seam::nodes::NodeId maxNodeId = 1;

    // First pass finds IDs which are already taken and finds maximum assigned IDs.
    for (size_t i = 0; i < nodesToSave.size(); i++) {
        // If the node isn't assigned an ID yet, none of its pins will be assigned either.
        auto node_id = nodesToSave[i]->id;
        if (node_id != 0) {
            if (nodeMap.find(node_id) != nodeMap.end()) {
                printf("[Serialization]: Node has ID matching another Node, clearing this Node's ID. This shouldn't happen!\n");
                nodesToSave[i]->id = 0;
            } else {
                nodeMap.emplace(std::make_pair(node_id, nodesToSave[i]));
            }

            maxNodeId = std::max(node_id, maxNodeId);

            size_t outputs_size, inputs_size;
            auto output_pins = nodesToSave[i]->PinOutputs(outputs_size);
            auto input_pins = nodesToSave[i]->PinInputs(inputs_size);

            // Find assigned input pin IDs.
            for (size_t i = 0; i < inputs_size; i++) {
                UpdatePinMap(&input_pins[i], inputPinMap);
            }

            // Find assigned output pin IDs.
            for (size_t i = 0; i < outputs_size; i++) {
                UpdatePinMap(&output_pins[i], outputPinMap);
            }
        }
    }

    // Second pass assigns IDs to anything without IDs.
    for (size_t i = 0; i < nodesToSave.size(); i++) {
        auto node_id = nodesToSave[i]->id;
        // Find the next available ID if unassigned.
        assert(node_id != 0);
        if (node_id == 0) {
            while (nodeMap.find(maxNodeId) != nodeMap.end()) {
                maxNodeId++;
            }
            nodesToSave[i]->id = node_id = maxNodeId;
            maxNodeId++;
            nodeMap.emplace(std::make_pair(node_id, nodesToSave[i]));
        }

        size_t outputs_size, inputs_size;
        auto output_pins = nodesToSave[i]->PinOutputs(outputs_size);
        auto input_pins = nodesToSave[i]->PinInputs(inputs_size);
    }

    // Third pass: all nodes and pins should be assigned unique IDs now, and we can finally build the serialized graph.
    capnp::MallocMessageBuilder message;
    seam::schema::NodeGraph::Builder serialized_graph = message.initRoot<seam::schema::NodeGraph>();
    capnp::List<seam::schema::Node>::Builder serialized_nodes = serialized_graph.initNodes(nodesToSave.size());

    // Pair is output pin, input pin
    std::vector<std::pair<PinId, PinId>> connections;

    for (size_t i = 0; i < nodesToSave.size(); i++) {
        auto node = nodesToSave[i];
        auto node_builder = serialized_nodes[i];

        // Serialize node fields.
        auto nodePosition = ed::GetNodePosition((ed::NodeId)node);
        node_builder.getPosition().setX(nodePosition.x);
        node_builder.getPosition().setY(nodePosition.y);

        node_builder.setDisplayName(node->InstanceName());
        node_builder.setNodeName(node->NodeName().data());
        node_builder.setId(node->Id());

        size_t outputs_size, inputs_size;
        auto output_pins = nodesToSave[i]->PinOutputs(outputs_size);
        auto input_pins = nodesToSave[i]->PinInputs(inputs_size);
        const auto properties = node->GetProperties();

        auto inputs_builder = node_builder.initInputPins(inputs_size);
        auto outputs_builder = node_builder.initOutputPins(outputs_size);
        auto propertiesBuilder = node_builder.initProperties(properties.size());

        SerializePinInputsList(inputs_builder, input_pins, inputs_size);
        SerializePinOutputsList(outputs_builder, output_pins, outputs_size, connections);

        // Serialize properties
        for (size_t i = 0; i < properties.size(); i++) {
            const auto prop = properties[i];
            auto propBuilder = propertiesBuilder[i];

            size_t valuesSize;
            void* propValues = prop.getValues(valuesSize);

            propBuilder.setName(prop.name);
            auto valuesBuilder = propBuilder.initValues(valuesSize);

            Serialize(valuesBuilder, prop.type, propValues, valuesSize);
        }
    }

    // FINALLY, serialize pin connections.
    auto connections_builder = serialized_graph.initConnections(connections.size());
    for (size_t i = 0; i < connections.size(); i++) {
        connections_builder[i].setOutId(connections[i].first);
        connections_builder[i].setInId(connections[i].second);
    }

    // TODO get a name from elsewhere
    serialized_graph.setName(filename.data());

    PrintGraph(serialized_graph.asReader());

    FILE* file = fopen(filename.data(), "wb");
    if (file == NULL) {
        printf("error while opening %s: %d", filename.data(), errno);
    } else {
        capnp::writePackedMessageToFd(fileno(file), message);
        fclose(file);
        return true;
    }
    return false;
}

bool SeamGraph::LoadGraph(const std::string_view filename, std::vector<SeamGraph::Link>& links) {
    FILE* file = fopen(filename.data(), "rb");
	if (file == NULL) {
		printf("Error opening file for read back: %d\n", errno);
		return false;
	}
	auto message = capnp::PackedFdMessageReader(fileno(file));
	auto node_graph = message.getRoot<seam::schema::NodeGraph>();

	PrintGraph(node_graph);
	NewGraph();

	// Keep a pin id to pin map so pins can be looked up for connecting shortly.
	std::map<PinId, Pin*> input_pin_map;
	std::map<PinId, Pin*> output_pin_map;

	// Remember max pin and node IDs.
	PinId maxPinId = 0;
	NodeId maxNodeId = 0;

	// First deserialize nodes and pins
	for (const auto& serialized_node : node_graph.getNodes()) {
		auto node = CreateAndAdd(serialized_node.getNodeName().cStr());
		node->id = serialized_node.getId();
		maxNodeId = std::max(node->id, maxNodeId);
		node->instance_name = serialized_node.getDisplayName();

		auto position = ImVec2(serialized_node.getPosition().getX(), serialized_node.getPosition().getY());
		ed::SetNodePosition((ed::NodeId)node, position);

		size_t inputs_size, outputs_size;
		auto input_pins = node->PinInputs(inputs_size);
		auto output_pins = node->PinOutputs(outputs_size);

		auto dynamicPinsNode = dynamic_cast<IDynamicPinsNode*>(node);

		// Deserialize properties before inputs and outputs, since setting properties might create some pins.
		for (const auto& serializedProperty : serialized_node.getProperties()) {
			const auto propValues = serializedProperty.getValues();

			if (!propValues.size()) {
				continue;
			}

			props::NodePropertyType propertyType = props::SerializedPinTypeToPropType(propValues[0].which());

			// First try to match the property against a known property in the Node's properties list.
			// Ask for the properties list again after deserializing each property in case the list changed.
			auto properties = node->GetProperties();
			for (auto& prop : properties) {
				if (prop.name == serializedProperty.getName().cStr()) {
					if (prop.type != propertyType) {
						printf("Serialized Node property %s doesn't have same type as the Node\n", prop.name.c_str());
					}

					switch (prop.type) {
						case props::NodePropertyType::PROP_INT: {
							std::vector<int32_t> values = props::Deserialize<int32_t>(propValues, prop.type);
							prop.setValues(&values[0], values.size());
							break;
						}
						case props::NodePropertyType::PROP_UINT: {
							std::vector<uint32_t> values = props::Deserialize<uint32_t>(propValues, prop.type);
							prop.setValues(&values[0], values.size());
							break;
						}
						case props::NodePropertyType::PROP_FLOAT: {
							std::vector<float> values = props::Deserialize<float>(propValues, prop.type);
							prop.setValues(&values[0], values.size());
							break;
						}
						case props::NodePropertyType::PROP_STRING: {
							std::vector<std::string> values = props::Deserialize<std::string>(propValues, prop.type);
							prop.setValues(&values[0], values.size());
							break;
						}
						case props::NodePropertyType::PROP_BOOL: {
							// Can't use std::vector<bool> since it packs data tighter than is desirable here.
							bool* buff = new bool[propValues.size()];
							props::Deserialize(propValues, prop.type, buff, propValues.size());
							prop.setValues(buff, propValues.size());
							delete[] buff;
							break;
						}
						
						default: {
							throw std::logic_error("Node Property type is missing deserialization logic");
						}
					}
				
					continue;
				}
			}

			// POSSIBLE IMPROVEMENT: is there a use case for PropertyBag here?
		}

		// Deserialize inputs
		uint32_t inputIndex = 0;
		for (const auto& serialized_pin_in : serialized_node.getInputPins()) {
			std::string serialized_pin_name = serialized_pin_in.getName().cStr();
			std::transform(serialized_pin_name.begin(), serialized_pin_name.end(), serialized_pin_name.begin(), 
				[](unsigned char c) {return std::tolower(c); });

			// Try to find an input pin on the node with a matching name.
			auto matchPos = FindPinInByName(input_pins, inputs_size, serialized_pin_name);

			const auto serialized_channels = serialized_pin_in.getChannels();

			if (matchPos != (input_pins + inputs_size)) {
				PinInput* match = matchPos;
				// If this is a vector pin, first set its size.
				if ((match->flags & PinFlags::VECTOR) == PinFlags::VECTOR) {
					VectorPinInputBase* vectorPin = (VectorPinInputBase*)match->seamp;
					vectorPin->UpdateSize(match, serialized_pin_in.getChildren().size());
				}
				
				DeserializePinInput(serialized_pin_in, match);
				maxPinId = std::max(maxPinId, UpdatePinMap(match, input_pin_map));

			} else if (dynamicPinsNode != nullptr) {
				PinType pinType = (PinType)serialized_pin_in.getType();
				IDynamicPinsNode::PinInArgs pinArgs(pinType, serialized_pin_name, 
					serialized_channels.size(), inputIndex);

				PinInput* added = dynamicPinsNode->AddPinIn(pinArgs);

				if (added == nullptr) {
					printf("Dynamic Pins Node %s refused to add a pin named %s\n",
						node->NodeName().data(), serialized_pin_name.c_str());
				} else {
					maxPinId = std::max(maxPinId, UpdatePinMap(added, input_pin_map));

					// Refresh the input pins list!
					input_pins = node->PinInputs(inputs_size);
				}

			} else {
				printf("Could not match serialized input pin with name %s on node %s\n", 
					serialized_pin_name.c_str(), serialized_node.getDisplayName().cStr());
			}

			inputIndex += 1;
		}

		// Deserialize outputs
		for (const auto& serialized_pin_out : serialized_node.getOutputPins()) {
			std::string_view pin_name = serialized_pin_out.getName().cStr();

			// Try to find an input pin on the node with a matching name.
			auto match = std::find_if(output_pins, output_pins + outputs_size, [pin_name](const PinOutput& pin_out) 
				{ return pin_name == pin_out.name; }
			);

			if (match != (output_pins + outputs_size)) {
				DeserializePinOutput(serialized_pin_out, match);
				maxPinId = std::max(maxPinId, UpdatePinMap(match, output_pin_map));

			} else if (dynamicPinsNode != nullptr) {
				assert(false);//shit shit dooo me ples
			} else {
				printf("Could not match serialized output pin with name %s on node %s\n",
					pin_name.data(), serialized_node.getDisplayName().cStr());
			}
		}
	}

	// Now add any valid connections.
	links.clear();
	for (const auto& conn : node_graph.getConnections()) {
		size_t outId = conn.getOutId();
		size_t inId = conn.getInId();
		auto output_pin = output_pin_map.find(outId);
		auto input_pin = input_pin_map.find(inId);
		if (input_pin != input_pin_map.end() && output_pin != output_pin_map.end()) {
			if (input_pin->second->type != output_pin->second->type) {
				continue;
			}
			
			if (Connect((PinInput*)input_pin->second, (PinOutput*)output_pin->second)) {
				links.push_back(Link(output_pin->second->node, outId, input_pin->second->node, inId));
			}
		}
	}

	ed::NavigateToContent();

	// Make sure the IdsDistributor knows where to start assigning IDs from.
	IdsDistributor::GetInstance().ResetIds();
	IdsDistributor::GetInstance().SetNextNodeId(maxNodeId + 1);
	IdsDistributor::GetInstance().SetNextPinId(maxPinId + 1);

    return true;
}