#pragma once

#include <vector>

#include "seam/seam-graph.h"
#include "nodes/iNode.h"
#include "pins/push.h"
#include "seam/factory.h"
#include "frame-pool.h"

#include "INIReader.h"

namespace seam {

	using namespace nodes;
	using namespace pins;

	/// node graph editor manages the event nodes and connections that make up a seam scene
	class Editor {
	public:
		const static char* CONFIG_FILE_NAME;

		/// to be called by ofApp::setup()
		void Setup(ofSoundStreamSettings* soundSettings);

		~Editor();

		/// to be called by ofApp::draw()
		void Draw();

		void GuiDraw();

		void DrawSelectedNode();

		/// to be called by ofApp::update()
		void Update();

		void OnWindowResized(int w, int h);

		void ProcessAudio(ofSoundBuffer& buffer);

		seam::EventNodeFactory* GetFactory() { return graph.GetFactory(); }
	
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
		INode* lastSelectedVisualNode = nullptr;

		bool show_create_dialog = false;

		std::string loadedFile;

		INIReader iniReader = INIReader(CONFIG_FILE_NAME);

		SeamGraph graph;
	};
}