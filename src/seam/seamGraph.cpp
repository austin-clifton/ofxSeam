#include "seam/seamGraph.h"
#include "seam/hash.h"
#include "seam/properties/nodeProperty.h"

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
	    
	using PinInputsBuilder 	= ::capnp::List<::seam::schema::PinIn, ::capnp::Kind::STRUCT>::Builder;
	using PinOutputsBuilder = ::capnp::List<::seam::schema::PinOut, ::capnp::Kind::STRUCT>::Builder;
	using PinInputsReader	= ::capnp::List<::seam::schema::PinIn, ::capnp::Kind::STRUCT>::Reader;
	using PinOutputsReader 	= ::capnp::List<::seam::schema::PinOut, ::capnp::Kind::STRUCT>::Reader;

	void PrintInputPins(const PinInputsReader& children) { 
		for (const auto& pinIn : children) {
			std::stringstream ss;
			ss << "\t\t" << pinIn.getId() << "\t" << pinIn.getName().cStr() << " [";
			for (const auto& v : pinIn.getValues()) {
				if (v.isBoolValue()) {
					ss << v.getBoolValue() << " ";
				} else if (v.isFloatValue()) {
					ss << v.getFloatValue() << " ";
				} else if (v.isIntValue()) {
					ss << v.getIntValue() << " ";
				} else if (v.getUintValue()) {
					ss << v.getUintValue() << " ";
				} else if (v.isStringValue()) {
					ss << v.getStringValue().cStr() << " ";
				}
			}
			ss << "]\n";

			printf("%s", ss.str().c_str());

			PrintInputPins(pinIn.getChildren());
		}
	}
	
	void PrintOutputPins(const PinOutputsReader& children) {
		for (const auto& pinOut : children) {
			printf("\t\t%llu\t%s\n", pinOut.getId(), pinOut.getName().cStr());
			PrintOutputPins(pinOut.getChildren());
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
			PrintInputPins(node.getInputPins());

			printf("\tOutputs:\n");
			PrintOutputPins(node.getOutputPins());
		}

		// Now list Connections.
		printf("\tConnections (out id, in id):\n");
		for (const auto& conn : reader.getConnections()) {
			printf("\t\t(%llu, %llu)\n", conn.getOutId(), conn.getInId());
		}

		std::cout << std::endl;
	}

	void SerializePinInputsList(PinInputsBuilder& inputsBuilder, PinInput* inputs, size_t size) 
	{
		// Serialize input pins
		for (size_t i = 0; i < size; i++) {
			auto& pinIn = inputs[i];
			auto builder = inputsBuilder[i];
			builder.setName(pinIn.name);
			builder.setId(pinIn.id);
			builder.setNumCoords(pinIn.NumCoords());

			size_t childrenSize;
			PinInput* children = pinIn.PinInputs(childrenSize);

			if (childrenSize > 0) {
				auto serializedChildren = builder.initChildren(childrenSize);
				SerializePinInputsList(serializedChildren, children, childrenSize);
			}

			// Don't serialize channels if this input type doesn't have serializable state
			if (pinIn.Type() == PinType::Flow
				|| pinIn.Type() == PinType::FboRgba
				|| pinIn.Type() == PinType::FboRgba16F
				|| pinIn.Type() == PinType::FboRed
				|| pinIn.Type() == PinType::NoteEvent
				|| pinIn.Type() == PinType::Struct
			) {
				continue;
			}

			size_t totalElements;
			pinIn.Buffer(totalElements);
			auto valuesBuilder = builder.initValues(totalElements * pinIn.NumCoords());

			seam::props::Serialize(valuesBuilder, PinTypeToPropType(pinIn.Type()), &pinIn);
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
			builder.setType((uint16_t)pinOut.Type());
			builder.setNumCoords(pinOut.NumCoords());

			// Remember what connections this output has so we can serialize all the connections after this loop.
			for (size_t conn_index = 0; conn_index < pinOut.connections.size(); conn_index++) {
				auto& conn = pinOut.connections[conn_index];
				connections.push_back(std::make_pair(pinOut.id, conn.pinIn->id));
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
		
		// TODO this probably shouldn't be done by default!
		// Maybe a pin flag instead, PIN_FLAG_DYNAMIC_NUM_COORDS ?
		pinIn->SetNumCoords(serializedPin.getNumCoords());

		seam::props::Deserialize(serializedPin, pinIn);

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

	PinId UpdatePinMap(PinInput* pinIn, std::map<PinId, INode*>& pinMap) {
		auto emplaced = pinMap.emplace(pinIn->id, pinIn->node);
		// IDs are expected to be unique
		assert(emplaced.second);
		size_t maxId = pinIn->id;

		size_t childrenSize;
		PinInput* children = pinIn->PinInputs(childrenSize);
		for (size_t i = 0; i < childrenSize; i++) {
			maxId = std::max(UpdatePinMap(&children[i], pinMap), maxId);
		}

		return maxId;
	}

	PinId UpdatePinMap(PinOutput* pinOut, std::map<PinId, INode*>& pinMap) {
		auto emplaced = pinMap.emplace(pinOut->id, pinOut->node);
		// IDs are expected to be unique
		assert(emplaced.second);
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
		pinOut->SetNumCoords(serializedPin.getNumCoords());
		
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

SeamGraph::SeamGraph() {
	updateParams.push_patterns = &pushPatterns;
    updateParams.alloc_pool = &allocPool;
}

SeamGraph::~SeamGraph() {
	destructing = true;
    NewGraph();
}

void SeamGraph::Draw() {
    DrawParams params = GetDrawParams();
	for (auto n : nodesInDrawChain) {
		n->Draw(&params);
	}
}

void SeamGraph::UpdateNodeGraph(INode* n, UpdateParams* params) {
    // Traverse parents and update them before traversing this node.
    // Parents are sorted by update order, so that any "shared parents" are updated first
	auto& parents = n->GetParents();

    for (auto p : parents) {
		if (p.activeConnections > 0) {
			assert(p.node->update_order < n->update_order);
			UpdateNodeGraph(p.node, params);
		}
    }

    if (n->dirty || n->UpdatesOverTime()) {
        n->Update(params);
		if (n->IsVisual()) {
			nodesInDrawChain.push_back(n);
		}
		n->dirty = false;
    }
}

void SeamGraph::Update() {
    // Clear the per-frame allocation pool
    allocPool.Clear();

	UpdateParams* params = GetUpdateParams();

    // Traverse Nodes which must be updated every frame;
    // these are usually nodes which handle some kind of external input and/or can be dirtied by other threads
    for (auto n : nodesUpdateEveryFrame) {
        // Assume the node will dirty itself if it needs to Update()
        if (n->dirty) {
            n->Update(params);
        }
    }

	// Clear draw chain, the dirtied update chain will determine it.
	nodesInDrawChain.clear();

    // Traverse the visible node graph and update nodes that need to be updated
	if (visualOutputNode != nullptr) {
		UpdateNodeGraph(visualOutputNode, params);
	}

	// Update the last selected visual node in the GUI;
	// this is mostly a convenience feature for users building graphs
	if (lastSelectedVisualNode != nullptr) {
		UpdateNodeGraph(lastSelectedVisualNode, params);
	}

	// Also update the last selected node in the GUI, again for user convenience
	if (selectedNode != nullptr) {
		UpdateNodeGraph(selectedNode, params);
	}
}

void SeamGraph::LockAudio() {
	// Tell the audio thread to stop processing,
	// and then wait for it to confirm it's bailed out.
	audioLock.store(true);
	while (processingAudio.load()) {
		std::this_thread::yield();
	}
}

void SeamGraph::ProcessAudio(ofSoundBuffer& buffer) {
	// The audio thread checks if audio nodes need to be cleared and will clear them,
	// so that the main thread doesn't have to synchronize when deleting audio nodes.
	if (clearAudioNodes.load()) {
		audioNodes.clear();
		clearAudioNodes.store(false);
	}

	processingAudio.store(true);

	if (audioLock.load()) {
		processingAudio.store(false);
		return;
	}

    for (auto n : audioNodes) {
        n->ProcessAudio(buffer);
    }

	processingAudio.store(false);
}

void SeamGraph::NewGraph() {
	LockAudio();
	if (!destructing) {
		clearAudioNodes.store(true);
	} else {
		audioNodes.clear();
	}

	// Clear all the various lists that keep track of nodes.
    nodesInDrawChain.clear();
    nodesUpdateEveryFrame.clear();

	selectedNode = nullptr;
	visualOutputNode = nullptr;
	lastSelectedVisualNode = nullptr;

#if BUILD_AUDIO_ANALYSIS
	while (!destructing && clearAudioNodes.load() == true) {
		std::this_thread::yield();
	}
#endif

	// Finally, actually delete the nodes themselves so the list of all nodes can be cleared.
    for (size_t i = 0; i < nodes.size(); i++) {
        delete nodes[i];
    }
    nodes.clear();

	audioLock.store(false);

    IdsDistributor::GetInstance().ResetIds();
}

INode* SeamGraph::CreateAndAdd(seam::nodes::NodeId node_id) {
	INode* node = factory.Create(node_id);
	if (node != nullptr) {
		node->seamState.pushPatterns = &pushPatterns;
		node->seamState.texLocResolver = &texLocResolver;

		node->OnWindowResized(glm::ivec2(ofGetWidth(), ofGetHeight()));
		node->Setup(&setupParams);

		nodes.push_back(node);

		if (node->UpdatesEveryFrame()) {
			nodesUpdateEveryFrame.push_back(node);
		} 

		// Visual nodes should be drawn once after creation,
		// otherwise their frame buffers won't display anything.
		if (node->IsVisual()) {
			node->Update(GetUpdateParams());
			DrawParams drawParams = GetDrawParams();
			node->Draw(&drawParams);
		}

		// Does this Node process audio?
		IAudioNode* audioNode = dynamic_cast<IAudioNode*>(node);
		if (audioNode != nullptr) {
			LockAudio();
			// TODO this needs some kind of guard against the audio thread,
			// but a mutex can't be used since it's the audio thread...
			audioNodes.push_back(audioNode);
			audioLock.store(false);
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
            Disconnect(pinOutputs[i].connections[j - 1].pinIn, &pinOutputs[i]);
        }
    }

    // Remove the Node from whatever lists it's in...
    Erase(nodes, node);
    Erase(nodesInDrawChain, node);
    Erase(nodesUpdateEveryFrame, node);
    
    IAudioNode* audioNode = dynamic_cast<IAudioNode*>(node);
    if (audioNode != nullptr) {
		LockAudio();
        Erase(audioNodes, audioNode);
		audioLock.store(false);
    }

	if (selectedNode == node) {
		selectedNode = nullptr;
	}

	if (lastSelectedVisualNode == node) {
		lastSelectedVisualNode = nullptr;
	}

	if (visualOutputNode == node) {
		visualOutputNode = nullptr;
	}

    // Finally, delete the Node.
    delete node;
}

INode* SeamGraph::GetVisualOutputNode() {
	return visualOutputNode;
}

void SeamGraph::SetVisualOutputNode(INode* node) {
	assert(node->IsVisual());
	visualOutputNode = node;
}

INode* SeamGraph::GetLastSelectedVisualNode() {
	return lastSelectedVisualNode;
}

void SeamGraph::SetLastSelectedVisualNode(INode* node) {
	assert(node->IsVisual());
	lastSelectedVisualNode = node;
}

INode* SeamGraph::GetSelectedNode() {
	return selectedNode;
}

void SeamGraph::SetSelectedNode(INode* node) {
	selectedNode = node;
}

bool SeamGraph::Connect(PinInput* pinIn, PinOutput* pinOut) {
	assert((pinIn->flags & PinFlags::Input) == PinFlags::Input);
	assert((pinOut->flags & PinFlags::Output) == PinFlags::Output);

	INode* parent = pinOut->node;
	INode* child = pinIn->node;

	// Validations: make sure pin_out is actually an output pin of node_out, and same for the input
	assert(child->FindPinInput(pinIn->id) != nullptr);
	assert(parent->FindPinOutput(pinOut->id) != nullptr);

	// create the connection

	pinOut->connections.push_back(PinConnection(pinIn, pinOut));
	pinIn->connection = pinOut;

	PinConnectedArgs connectedArgs;
	connectedArgs.pinIn = pinIn;
	connectedArgs.pinOut = pinOut;
	connectedArgs.pushPatterns = &pushPatterns;

	pinIn->OnConnected(connectedArgs);
	pinOut->OnConnected(connectedArgs);

	// give the input pin the default push pattern
	pinIn->push_id = pushPatterns.Default().id;

	// add to each node's parents and children list
	// if this connection rearranged the node graph, 
	// its traversal order will need to be recalculated
	const bool isNewChild = parent->AddChild(child);
	const bool isNewParent = child->AddParent(parent, pinIn);
	const bool rearranged = isNewParent || isNewChild;

	if (rearranged) {
		RecalculateTraversalOrder(child);
	}

	return true;
}

bool SeamGraph::Disconnect(PinInput* pinIn, PinOutput* pinOut) {
    assert((pinIn->flags & PinFlags::Input) == PinFlags::Input);
	assert((pinOut->flags & PinFlags::Output) == PinFlags::Output);

	INode* parent = pinOut->node;
	INode* child = pinIn->node;

	assert(child->FindPinInput(pinIn->id) != nullptr);
	assert(parent->FindPinOutput(pinOut->id) != nullptr);

	pinIn->connection = nullptr;

	// remove from pinOut's connections list
	size_t i = 0;
	for (; i < pinOut->connections.size(); i++) {
		if (pinOut->connections[i].pinIn == pinIn) {
			break;
		}
	}
	assert(i < pinOut->connections.size());
	pinOut->connections.erase(pinOut->connections.begin() + i);
	 
	PinConnectedArgs args;
	args.pinIn = pinIn;
	args.pinOut = pinOut;
	args.pushPatterns = &pushPatterns;

	pinIn->OnDisconnected(args);
	pinOut->OnDisconnected(args);

	// Null out disconnected FBO pin pointers; this will need to be done for other pointer types!
	// This is probably a temporary solution.
	if (pins::IsFboPin(pinIn->Type())) {
		size_t fbosSize;
		void* pinBuff = pinIn->Buffer(fbosSize);
		std::memset(pinBuff, 0, fbosSize * sizeof(ofFbo*));
	}

	// track if we need to update traversal order
	bool rearranged = false;

	// remove or decrement from receivers / transmitters lists
	{
		auto it = std::find(parent->children.begin(), parent->children.end(), child);
		assert(it != parent->children.end());
		if (it->connCount == 1) {
			parent->children.erase(it);
			rearranged = true;
		} else {
			it->connCount -= 1;
		}
	}

	{
		auto it = std::find(child->parents.begin(), child->parents.end(), parent);
		assert(it != child->parents.end());
		if (it->connCount == 1) {
			child->parents.erase(it);
			rearranged = true;
		} else {
			it->connCount -= 1;
		}
	}

	if (rearranged) {
		// the child node and its children need to recalculate draw and update order now
		RecalculateTraversalOrder(child);
	}

	return true;
}

void SeamGraph::OnWindowResized(int w, int h) {
	assert(w > 0 && h > 0);
	for (auto n : nodes) {
		n->OnWindowResized(glm::ivec2(w, h));
	}
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
    std::map<seam::pins::PinId, INode*> inputPinMap;
    std::map<seam::pins::PinId, INode*> outputPinMap;
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

            SerializeProperty(valuesBuilder, prop.type, propValues, valuesSize);
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
	serialized_graph.setMaxNodeId(IdsDistributor::GetInstance().NextNodeId());
	serialized_graph.setMaxPinId(IdsDistributor::GetInstance().NextPinId());
	serialized_graph.setVisualOutputNodeId(visualOutputNode != nullptr ? visualOutputNode->id : 0);

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
	
	// Make sure the IdsDistributor knows where to start assigning IDs from.
	// Note we set the max node/pin ID BEFORE deserializing, so that
	// any new pins since the last load (for instance from shader uniforms)
	// get new pin IDs based on the last maximum.
	IdsDistributor::GetInstance().ResetIds();
	IdsDistributor::GetInstance().SetNextNodeId(node_graph.getMaxNodeId());
	IdsDistributor::GetInstance().SetNextPinId(node_graph.getMaxPinId());

	// Keep a pin id to node map so pins can be looked up for connecting shortly.
	// Pins have to be looked up through the Node each time we want them,
	// since pin pointers are prone to change during deserialization.
	std::map<PinId, INode*> inputPinMap;
	std::map<PinId, INode*> outputPinMap;

	// First deserialize nodes and pins
	for (const auto& serialized_node : node_graph.getNodes()) {
		auto node = CreateAndAdd(serialized_node.getNodeName().cStr());
		node->id = serialized_node.getId();
		node->instance_name = serialized_node.getDisplayName();

		auto position = ImVec2(serialized_node.getPosition().getX(), serialized_node.getPosition().getY());
		ed::SetNodePosition((ed::NodeId)node, position);

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
						case props::NodePropertyType::Int: {
							std::vector<int32_t> values = props::Deserialize<int32_t>(propValues, prop.type);
							prop.setValues(&values[0], values.size());
							break;
						}
						case props::NodePropertyType::Uint: {
							std::vector<uint32_t> values = props::Deserialize<uint32_t>(propValues, prop.type);
							prop.setValues(&values[0], values.size());
							break;
						}
						case props::NodePropertyType::Float: {
							std::vector<float> values = props::Deserialize<float>(propValues, prop.type);
							prop.setValues(&values[0], values.size());
							break;
						}
						case props::NodePropertyType::String: {
							std::vector<std::string> values = props::Deserialize<std::string>(propValues, prop.type);
							prop.setValues(&values[0], values.size());
							break;
						}
						case props::NodePropertyType::Bool: {
							// Can't use std::vector<bool> since it packs data tighter than is desirable here.
							bool* buff = new bool[propValues.size()];
							props::DeserializeProperty(propValues, prop.type, buff, propValues.size());
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

		// Now that props have been set (possibly creating pins), get the pin in / out lists.
		size_t inputs_size;
		auto input_pins = node->PinInputs(inputs_size);

		auto dynamicPinsNode = dynamic_cast<IDynamicPinsNode*>(node);

		// Deserialize inputs
		uint32_t inputIndex = 0;
		for (const auto& serialized_pin_in : serialized_node.getInputPins()) {
			std::string serialized_pin_name = serialized_pin_in.getName().cStr();
			std::transform(serialized_pin_name.begin(), serialized_pin_name.end(), serialized_pin_name.begin(), 
				[](unsigned char c) {return std::tolower(c); });

			// Try to find an input pin on the node with a matching name.
			auto matchPos = FindPinInByName(input_pins, inputs_size, serialized_pin_name);

			const auto serialized_values = serialized_pin_in.getValues();

			if (matchPos != nullptr) {
				PinInput* match = matchPos;
				// If this is a vector pin, first set its size.
				if ((match->flags & PinFlags::Vector) == PinFlags::Vector) {
					VectorPinInput* vectorPin = (VectorPinInput*)match->seamp;
					vectorPin->UpdateSize(serialized_pin_in.getChildren().size());
				}
				
				DeserializePinInput(serialized_pin_in, match);
				UpdatePinMap(match, inputPinMap);

			} else if (dynamicPinsNode != nullptr) {
				PinType pinType = (PinType)serialized_pin_in.getType();
				IDynamicPinsNode::PinInArgs pinArgs(pinType, serialized_pin_name, 
					serialized_values.size(), inputIndex);

				PinInput* added = dynamicPinsNode->AddPinIn(pinArgs);

				if (added == nullptr) {
					printf("Dynamic Pins Node %s refused to add an input pin named %s\n",
						node->NodeName().data(), serialized_pin_name.c_str());
				} else {
					DeserializePinInput(serialized_pin_in, added);
					UpdatePinMap(added, inputPinMap);

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
		size_t outputs_size;
		auto output_pins = node->PinOutputs(outputs_size);
		for (const auto& serialized_pin_out : serialized_node.getOutputPins()) {
			std::string_view pin_name = serialized_pin_out.getName().cStr();

			// Try to find an output pin on the node with a matching name.
			auto match = FindPinOutByName(output_pins, outputs_size, pin_name);

			if (match != nullptr) {
				DeserializePinOutput(serialized_pin_out, match);
				UpdatePinMap(match, outputPinMap);

			} else if (dynamicPinsNode != nullptr) {
				PinType pinType = (PinType)serialized_pin_out.getType();
				PinOutput pinOut = SetupOutputPin(dynamicPinsNode, pinType, 
					serialized_pin_out.getName().cStr(), serialized_pin_out.getNumCoords());

				DeserializePinOutput(serialized_pin_out, &pinOut);

				PinOutput* added = dynamicPinsNode->AddPinOut(std::move(pinOut), 0);

				if (added == nullptr) {
					printf("Dynamic Pins Node %s refused to add an output pin named %s\n",
						node->NodeName().data(), serialized_pin_out.getName().cStr());
				} else {
					assert(added->id == serialized_pin_out.getId());

					UpdatePinMap(added, outputPinMap);

					// Refresh the output pins list
					output_pins = node->PinOutputs(outputs_size);
				}
			} else {
				printf("Could not match serialized output pin with name %s on node %s\n",
					pin_name.data(), serialized_node.getDisplayName().cStr());
			}
		};
	}

	// Now add any valid connections.
	links.clear();
	for (const auto& conn : node_graph.getConnections()) {
		size_t outId = conn.getOutId();
		size_t inId = conn.getInId();
		auto outNode = outputPinMap.find(outId);
		auto inNode = inputPinMap.find(inId);
		if (inNode != inputPinMap.end() && outNode != outputPinMap.end()) {
			// Look up the input and output pins.
			auto outPin = outNode->second->FindPinOutput(outId);
			auto inPin = inNode->second->FindPinInput(inId);

			if (Connect(inPin, outPin)) {
				links.push_back(Link(outNode->second, outId, inNode->second, inId));
			} else {
				printf("LoadGraph(): failed to connect pin out %llu to pin in %llu\n",
					outId, inId);
			}
		} else {
			printf("LoadGraph(): could not match connection for output id %llu and input id %llu\n",
				outId, inId);
		}
	}

	ed::NavigateToContent();

	// Ensure that window-size related pins use the current resolution instead of the file's resolution.
	OnWindowResized(ofGetWidth(), ofGetHeight());

	auto visualOutputNodeId = node_graph.getVisualOutputNodeId();
	if (visualOutputNodeId != 0) {
		auto it = std::find_if(nodes.begin(), nodes.end(), [visualOutputNodeId](const INode* n) {
			return n->id == visualOutputNodeId;
		});

		assert(it != nodes.end());
		if (it != nodes.end()) {
			SetVisualOutputNode(*it);
		}
	}
	
    return true;
}