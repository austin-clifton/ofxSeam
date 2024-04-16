#include "seam/nodes/iNode.h"

#include "ofxImGui.h"

#include "blueprints/widgets.h"
#include "seam/imguiUtils/properties.h"

using namespace seam;
using namespace seam::nodes;
using namespace seam::pins;
namespace im = ImGui;
namespace ed = ax::NodeEditor;

// TODO make this configurable
float icon_size = 24.f;

namespace {
	// next two are stripped from the blueprints example in imgui-node-editor
	// TODO make the pin icons more meaningful (circles and colors are meaningless)

	ImColor GetIconColor(PinType type)
	{
		switch (type) {
		default:
		case PinType::TYPE_NONE:
		case PinType::FLOW:     	return ImColor(255, 255, 255);
		case PinType::BOOL:     	return ImColor(220, 48, 48);
		case PinType::INT:      	return ImColor(68, 201, 156);
		case PinType::FLOAT:    	return ImColor(147, 226, 74);
		case PinType::STRING:   	return ImColor(124, 21, 153);
		case PinType::FBO_RGBA:  	return ImColor(51, 150, 215);
		case PinType::FBO_RGBA16F:  return ImColor(215, 150, 51);
		case PinType::FBO_RED:  	return ImColor(215, 0, 0);
		case PinType::STRUCT: 		return ImColor(128);
		// case PinType::Function: return ImColor(218, 0, 183);
		// case PinType::Delegate: return ImColor(255, 48, 48);
		}
	};

	void DrawPinIcon(PinType type, bool connected, float alpha)
	{
		using IconType = ax::Drawing::IconType;

		IconType icon_type;
		ImColor color = GetIconColor(type);
		color.Value.w = alpha;
		switch (type) {
		case PinType::FLOW:		    icon_type = IconType::Flow;   break;
		case PinType::BOOL:			icon_type = IconType::Circle; break;
		case PinType::INT:			icon_type = IconType::Circle; break;
		case PinType::UINT:			icon_type = IconType::Circle; break;
		case PinType::FLOAT:		icon_type = IconType::Circle; break;
		case PinType::STRING:		icon_type = IconType::Circle; break;
		case PinType::FBO_RGBA:		icon_type = IconType::Square; break;
		case PinType::FBO_RGBA16F:	icon_type = IconType::Square; break;
		case PinType::FBO_RED:		icon_type = IconType::Square; break;
		case PinType::NOTE_EVENT:	icon_type = IconType::Grid; break;
		case PinType::ANY:			icon_type = IconType::Diamond; break;
		case PinType::STRUCT:		icon_type = IconType::Square; break;

		// case PinType::Object:   icon_type = IconType::Circle; break;
		// case PinType::Function: icon_type = IconType::Circle; break;
		// case PinType::Delegate: icon_type = IconType::Square; break;
		case PinType::TYPE_NONE:
			// pins which are drawn should not have type none
			// did you forget to set it?
			assert(false);
		default:
			// hey you forgot to say how to draw this pin type
			assert(false);
			return;
		}

		ax::Widgets::Icon(ImVec2(icon_size, icon_size), icon_type, connected, color, ImColor(.125f, .125f, .125f, alpha));
	};
}

bool INode::CompareUpdateOrder(const INode* l, const INode* r) {
	return l->update_order < r->update_order;
}

bool INode::CompareConnUpdateOrder(const NodeConnection& l, const NodeConnection& r) {
	return l.node->update_order < r.node->update_order;
}

void INode::SortParents() {
	std::sort(parents.begin(), parents.end(), &INode::CompareConnUpdateOrder);
}

bool INode::AddParent(INode* parent) {
	// the parents list is sorted to keep update traversal in order without any further sorting
	// insert to a sorted list, if this parent isn't already in the parents list
	NodeConnection conn;
	conn.node = parent;
	auto it = std::lower_bound(parents.begin(), parents.end(), conn, &INode::CompareConnUpdateOrder);
	if (it == parents.end() || it->node != parent) {
		// new parent, add the new NodeConnection for it
		conn.conn_count = 1;
		parents.insert(it, conn);
		return true;
	} else {
		// parent already exists, increase its connection count
		it->conn_count += 1;
		return false;
	}
}

bool INode::AddChild(INode* child) {
	// the children list is not sorted for now, don't think there is any reason to sort it
	auto it = std::find(children.begin(), children.end(), child);
	if (it == children.end()) {
		// new child, make a NodeConnection for it
		NodeConnection conn;
		conn.node = child;
		conn.conn_count = 1;
		children.push_back(conn);
		return true;
	} else {
		// child already exists, increase its connection count
		it->conn_count += 1;
		return false;
	}
}

void INode::GuiDrawInputPin(ed::Utilities::BlueprintNodeBuilder& builder, pins::PinInput* pinIn) 
{
	builder.Input((ed::PinId)pinIn);

	// Check if this Pin has children to be drawn.
	size_t childrenSize;
	PinInput* children = pinIn->PinInputs(childrenSize);
	bool showChildren = false;

	DrawPinIcon(pinIn->type, pinIn->connection != nullptr, 1.0f);
	ImGui::Spring(0.f);

	if (childrenSize > 0) {
		showChildren = ImGui::TreeNode(pinIn->name.data());
	} else {
		ImGui::TextUnformatted(pinIn->name.data());
	}

	ImGui::Spring(0.f);
	builder.EndInput();

	if (showChildren) {
		for (size_t i = 0; i < childrenSize; i++) {
			GuiDrawInputPin(builder, &children[i]);
		}
		ImGui::TreePop();
	}
}

void INode::GuiDrawOutputPin(ed::Utilities::BlueprintNodeBuilder& builder, pins::PinOutput* pinOut) 
{
	builder.Output((ed::PinId)pinOut);

	// Check if this Pin has children to be drawn.
	size_t childrenSize;
	PinOutput* children = pinOut->PinOutputs(childrenSize);
	bool showChildren = false;

	DrawPinIcon(pinOut->type, pinOut->connections.size() > 0, 1.0f);
	ImGui::Spring(0.f);

	if (childrenSize > 0) {
		showChildren = ImGui::TreeNode(pinOut->name.data());
	} else {
		ImGui::TextUnformatted(pinOut->name.data());
	}

	ImGui::Spring(0.f);
	builder.EndOutput();

	if (showChildren) {
		for (size_t i = 0; i < childrenSize; i++) {
			GuiDrawOutputPin(builder, &children[i]);
		}
		ImGui::TreePop();
	}
}

void INode::GuiDraw(ed::Utilities::BlueprintNodeBuilder& builder) {
	builder.Begin(ed::NodeId(this));

	size_t size;

	// draw the header
	// TODO header colors?
	builder.Header();
	ImGui::TextUnformatted(NodeName().data());
	ImGui::Dummy(ImVec2(0, 28));
	builder.EndHeader();

	PinInput* inputs = PinInputs(size);
	for (size_t i = 0; i < size; i++) {
		GuiDrawInputPin(builder, &inputs[i]);
	}

	// draw the node's center
	builder.Middle();
	GuiDrawNodeCenter();

	PinOutput* outputs = PinOutputs(size);
	for (size_t i = 0; i < size; i++) {
		GuiDrawOutputPin(builder, &outputs[i]);
	}

	builder.End();
}

void INode::GuiDrawNodeCenter() {
	// TODO how does spring really work...?
	// im::Spring(1, 0);
	if (gui_display_fbo != nullptr && gui_display_fbo->isAllocated()) {
		props::DrawFbo(*gui_display_fbo);
	}

	// im::Spring(1, 0);
}

pins::PinInput* INode::FindPinInput(PinId id) {
	return IInPinnable::FindPinIn(this, id);
}

pins::PinOutput* INode::FindPinOutput(PinId id) {
	return IOutPinnable::FindPinOut(this, id);
}

void INode::OnWindowResized(glm::uvec2 resolution) {
	for (auto& windowFbo : windowFbos) {
		glm::ivec2 expected = resolution * windowFbo.ratio;
		if (windowFbo.fbo->getWidth() != expected.x || windowFbo.fbo->getHeight() != expected.y) {

			Seam().texLocResolver->ReleaseAll(&windowFbo.fbo->getTexture());

			// Destroy and reallocate the FBO.
			windowFbo.fbo->clear();
			windowFbo.fboSettings.width = expected.x;
			windowFbo.fboSettings.height = expected.y;
			windowFbo.fbo->allocate(windowFbo.fboSettings);

			UpdateResolutionPin(expected);
			SetDirty();

			// Refresh pin connections since the fbo changed
			if (windowFbo.pinOutFbo != nullptr) {
				windowFbo.pinOutFbo->Reconnect(Seam().pushPatterns);
			}
		}
	}
}

void INode::UpdateResolutionPin(glm::uvec2 resolution) {
	PinInput* resolutionPin = FindPinInByName(this, "resolution");
	if (resolutionPin != nullptr) {
		// Expect resolution to be an ivec2 or uvec2
		assert(resolutionPin->type == PinType::UINT || resolutionPin->type == PinType::INT);
		size_t size;
		void* buffer = resolutionPin->Buffer(size);
		assert(size * resolutionPin->NumCoords() == 2);

		resolutionPin->OnValueChanging();
		*((glm::uvec2*)buffer) = resolution;
		resolutionPin->OnValueChanged();
	}
}

bool INode::GuiDrawPropertiesList(UpdateParams* params) {
	using namespace seam::props;

	bool changed = false;

	// Changes from ImGui are written to this buffer and then passed to prop.setValues(),
	// instead of directly setting the data returned from prop.getValues().
	// This way when setValues() is called, the Node still has the "old" value for any cleanup logic etc.
	std::vector<char> tmpBuf;

	auto props = GetProperties();
	for (auto& prop : props) {
		// Figure out how much space this property requires.
		size_t bytesPerElement = PropTypeToByteSize(prop.type);
		size_t totalElements;
		void* vbuf = prop.getValues(totalElements);

		// Allocate enough space and copy current values to the tmpBuf.
		tmpBuf.resize(bytesPerElement * totalElements);
		std::copy((char*)vbuf, (char*)vbuf + bytesPerElement * totalElements, tmpBuf.data());

		ImGuiDataType imguiType = -1;
		switch (prop.type) {
			case NodePropertyType::PROP_BOOL: {
				for (size_t i = 0; i < totalElements; i++) {
					std::string name = prop.name + " " + std::to_string(i);
					changed = ImGui::Checkbox(name.c_str(), (bool*)&tmpBuf[i]) || changed;
				}

				if (changed) {
					prop.setValues(tmpBuf.data(), totalElements);
				}
				break;
			}
			case NodePropertyType::PROP_FLOAT: {
				imguiType = ImGuiDataType_Float;
				break;
			}
			case NodePropertyType::PROP_INT: {
				imguiType = ImGuiDataType_S32;
				break;
			}
			case NodePropertyType::PROP_UINT: {
				imguiType = ImGuiDataType_U32;
				break;
			}
			case NodePropertyType::PROP_STRING: {
				// TODO
				assert(false);
			}
			case NodePropertyType::PROP_STRUCT: {
				// TODO
				assert(false);
			}
			case NodePropertyType::PROP_NONE: {
				// Why do you have a property with type none??
				assert(false);
			}
			default: {
				throw std::logic_error("Unimplemented property type! Please fix!");
			}
		}

		// Draw a drag input for the prop if possible.
		if (imguiType != -1) {
			// Note that tmpBuf is modified here...
			changed = ImGui::DragScalarN(
				prop.name.c_str(), 
				imguiType,
				tmpBuf.data(),
				totalElements
			);

			// ...and then sent to setValues() if anything changed.
			if (changed) {
				prop.setValues(tmpBuf.data(), totalElements);
			}
		}
	}
	
	return changed;
}