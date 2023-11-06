#pragma once

#include <vector>

#include "seam/seam-graph.h"
#include "nodes/iNode.h"
#include "pins/push.h"
#include "seam/factory.h"
#include "frame-pool.h"

namespace seam {

	using namespace nodes;
	using namespace pins;

	/// node graph editor manages the event nodes and connections that make up a seam scene
	class Editor {
	public:
		/// to be called by ofApp::setup()
		void Setup(const ofSoundStreamSettings& soundSettings);

		~Editor();

		/// to be called by ofApp::draw()
		void Draw();

		void GuiDraw();

		void DrawSelectedNode();

		/// to be called by ofApp::update()
		void Update();

		void ProcessAudio(ofSoundBuffer& buffer);

		seam::EventNodeFactory* GetFactory() { return graph->GetFactory(); }
	
	private:


		void NewGraph();
		void SaveGraph(const std::string_view filename, const std::vector<INode*>& nodesToSave);
		void LoadGraph(const std::string_view filename);
		
		bool Connect(PinInput* pinIn, PinOutput* pinOut);

		bool Disconnect(PinInput* pinIn, PinOutput* pinOut);

		void GuiDrawPopups();

		ax::NodeEditor::EditorContext* nodeEditorContext = nullptr;

		// List of all links between pins, mostly for GUI display + interactions
		std::vector<SeamGraph::Link> links;

		// book keeping for GUI interactions
		Pin* new_link_pin;
		INode* selected_node = nullptr;

		bool show_create_dialog = false;

		std::string loaded_file;

		SeamGraph* graph = nullptr;
	};
}