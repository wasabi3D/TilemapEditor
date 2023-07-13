#include "tilemapeditor.h"


void TileMapEditor::start(std::shared_ptr<void> data, cho::SDLPointers pointers) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	io = &ImGui::GetIO();
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForSDLRenderer(pointers.window, pointers.renderer);
	ImGui_ImplSDLRenderer2_Init(pointers.renderer);
	io->ConfigWindowsMoveFromTitleBarOnly = true;

	if (data == nullptr) {
		std::cerr << "Startup data not specified" << std::endl;
		running = false;
		return;
	}
	start_data = std::static_pointer_cast<TileMapStartupData>(data);
	window_w = start_data->window_w;
	window_h = start_data->window_h;

	auto nothing = [](){};
	auto always = []() {return true; };
	fsm.clear();
	fsm.addState(TM_State(UC_States::DEFAULT, nothing));
	fsm.addState(TM_State(UC_States::EDIT_DRAGGING_LEFT, nothing));
	fsm.addState(TM_State(UC_States::EDIT_DRAGGING_RIGHT, nothing));
	fsm.addState(TM_State(UC_States::PALETTE_DRAGGING, nothing));

	// Left Click drag on edit view
	fsm.addTransition(
		TM_Transition(
			UC_Transitions::EDIT_START_DRAG_LEFT,
			UC_States::DEFAULT,
			UC_States::EDIT_DRAGGING_LEFT,
			[this]() {return allow_input_to_canvas && edit_area != nullptr && mouse.focused_window == FOCUSED_EDIT; },
			[this]() {edit_area->onStartDrag(false); })
	);
	fsm.addTransition(
		TM_Transition(
			UC_Transitions::EDIT_END_DRAG_LEFT,
			UC_States::EDIT_DRAGGING_LEFT,
			UC_States::DEFAULT,
			always,
			[this]() {edit_area->onEndDrag(mouse.start_dragging_right); }
		)
	);

	// Right click drag on edit view
	fsm.addTransition(
		TM_Transition(
			UC_Transitions::EDIT_START_DRAG_RIGHT,
			UC_States::DEFAULT,
			UC_States::EDIT_DRAGGING_RIGHT,
			[this]() {return (allow_input_to_canvas && edit_area != nullptr && mouse.focused_window == FOCUSED_EDIT); },
			[this]() {this->edit_area->onStartDrag(true); })
	);
	fsm.addTransition(
		TM_Transition(
			UC_Transitions::EDIT_END_DRAG_RIGHT,
			UC_States::EDIT_DRAGGING_RIGHT,
			UC_States::DEFAULT,
			always,
			[this]() {edit_area->onEndDrag(mouse.start_dragging); }
		)
	);

	// Left click drag on palette view
	fsm.addTransition(
		TM_Transition(
			UC_Transitions::PALETTE_START_DRAG,
			UC_States::DEFAULT,
			UC_States::PALETTE_DRAGGING,
			[this]() {return (allow_input_to_canvas && palette_area != nullptr && mouse.focused_window == FOCUSED_PALETTE); },
			[this]() {
				palette_area->onLeftClick();
				palette_area->onStartDrag(); 
			}
		)
	);
	fsm.addTransition(
		TM_Transition(
			UC_Transitions::PALETTE_END_DRAG,
			UC_States::PALETTE_DRAGGING,
			UC_States::DEFAULT,
			always,
			nothing
		)
	);
}

void TileMapEditor::init_viewport() {
	if (edit_area != nullptr) {
		edit_area.reset();
	}
	if (palette_area != nullptr) {
		palette_area->destroy();
		palette_area.reset();
	}
	if (inspector_area != nullptr) {
		inspector_area.reset();
	}

	edit_area = std::make_unique<EditArea>(tile_size, map_w, map_h);
	edit_area->clear_color = { 20, 20, 20, 255 };
	edit_area->line_color = { 50, 50, 50, 255 };
	edit_area->camera_pos = { -1, 0 };

	palette_area = std::make_unique<PaletteArea>(tile_size);
	palette_area->clear_color = { 20, 20, 20, 255 };
	palette_area->line_color = { 50, 50, 50, 255 };
	palette_area->setCameraPosition({ -1, 0 });
	palette_area->editOnCloseTexture =
		[this](int id) {this->edit_area->onDeleteTexture(id); };
	palette_area->editOnReplaceRemoveTiles =
		[this](int id, int max_x, int max_y)
	{
		this->edit_area->editOnReplaceRemoveTiles(id, max_x, max_y);
	};

	inspector_area = std::make_unique<InspectorArea>();
	inspector_area->on_add_layer = [this]() {this->edit_area->onAddLayer(); };
	inspector_area->on_delete_layer = [this](int layer) {this->edit_area->onDeleteLayer(layer); };
	inspector_area->on_swap = [this](int a, int b) {this->edit_area->onSwap(a, b);  };
	inspector_area->addNewLayer();
	inspector_area->io = io;

	edit_area->visibles = &inspector_area->visible_layers;
}


void TileMapEditor::processEvent(const SDL_Event& event) {
	ImGui_ImplSDL2_ProcessEvent(&event);
	if (event.type == SDL_QUIT) 
		running = false;
	
	if (event.type == SDL_MOUSEMOTION) {
		mouse.motion.x = event.motion.xrel;
		mouse.motion.y = event.motion.yrel;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN) {
		switch (event.button.button) {
		case SDL_BUTTON_MIDDLE:
			mouse.middle_click = true;
			break;


		case SDL_BUTTON_LEFT:
			if (!mouse.left_click) {
				mouse.start_dragging = true;
			}
			mouse.left_click = true;
			fsm.executeTransition(UC_Transitions::EDIT_START_DRAG_LEFT);
			fsm.executeTransition(UC_Transitions::EDIT_END_DRAG_RIGHT);
			fsm.executeTransition(UC_Transitions::PALETTE_START_DRAG);
			break;


		case SDL_BUTTON_RIGHT:
			if (!mouse.right_click) {
				mouse.start_dragging_right = true;
			}
			mouse.right_click = true;
			fsm.executeTransition(UC_Transitions::EDIT_START_DRAG_RIGHT);
			fsm.executeTransition(UC_Transitions::EDIT_END_DRAG_LEFT);
			break;
		}
	}
	if (event.type == SDL_MOUSEBUTTONUP) {
		switch (event.button.button) {
		case SDL_BUTTON_MIDDLE:
			mouse.middle_click = false;
			break;


		case SDL_BUTTON_LEFT:
			mouse.left_click = false;

			// Test everything for simplicity 
			fsm.executeTransition(UC_Transitions::EDIT_END_DRAG_LEFT);
			fsm.executeTransition(UC_Transitions::PALETTE_END_DRAG);
			break;


		case SDL_BUTTON_RIGHT:
			mouse.right_click = false;
			fsm.executeTransition(UC_Transitions::EDIT_END_DRAG_RIGHT);
		}
	}

	if (event.type == SDL_MOUSEWHEEL)
		mouse.wheel_motion = event.wheel.y;
}

void TileMapEditor::update(float delta) {
	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New...")) {
				creating_new = true;
			}
			if(!creating_new && ImGui::MenuItem("Save...")){
				saving = true;
			}

			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	if (creating_new) {
		ImGui::Begin("Create a new tilemap", &creating_new, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::SetWindowPos({ (float)window_w / 2, (float)window_h / 2 }, ImGuiCond_Once);
		static char tile_dim[10] = ""; ImGui::InputText("Tile dimension(in pixels)", tile_dim, 10, ImGuiInputTextFlags_CharsDecimal);
		static char width[10] = ""; ImGui::InputText("Width", width, 10, ImGuiInputTextFlags_CharsDecimal);
		static char height[10] = ""; ImGui::InputText("Height", height, 10, ImGuiInputTextFlags_CharsDecimal);

		if (ImGui::Button("Create")) {
			try {
				std::string s_width(width), s_height(height), s_tile_dim(tile_dim);
				tile_size = std::stoi(s_tile_dim);
				map_w = std::stoi(s_width); 
				map_h = std::stoi(s_height);
				init_viewport();
				creating_new = false;
			}
			catch (std::invalid_argument ex) {
				std::cout << "Conversion error" << std::endl;;
			}
		}
		ImGui::Text("Warning: Any unsaved data will be lost forever.");
		ImGui::End();
	}

	if (saving) {
		auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;
		ImGui::Begin("Select format", &saving, flags);
		ImGui::Text("Please select saving format");
		ImGui::Separator();
		ImGui::NewLine();
		if (ImGui::BeginCombo("format", cur_format.c_str())) {
			for (std::string format : {TMX, PNG}) {
				const bool selected = (cur_format == format);
				if (ImGui::Selectable(format.c_str(), &selected))
					cur_format = format;
				if (selected)
					ImGui::SetItemDefaultFocus();
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip(format_tooltip.at(format).c_str());
			}
			ImGui::EndCombo();
		}

		if (ImGui::Button("Proceed")) {
			nfdchar_t* save_path = nullptr;
			nfdresult_t result = NFD_SaveDialog("", nullptr, &save_path);
			if (result == NFD_OKAY) {
				std::cout << "Save path: " << std::string(save_path) << cur_format << std::endl;
				
				tinyxml2::XMLDocument doc;
				tinyxml2::XMLDeclaration* decl = doc.NewDeclaration();
				doc.InsertFirstChild(decl);

				tinyxml2::XMLElement* root_ptr = doc.NewElement("map");
				root_ptr->SetAttribute("version", "1.10");
				root_ptr->SetAttribute("tiledversion", "1.10.1");
				root_ptr->SetAttribute("orientation", "orthogonal");
				root_ptr->SetAttribute("renderorder", "right-down");
				root_ptr->SetAttribute("width", map_w);
				root_ptr->SetAttribute("height", map_h);
				root_ptr->SetAttribute("tilewidth", tile_size);
				root_ptr->SetAttribute("tileheight", tile_size);
				root_ptr->SetText("\n");
				doc.InsertEndChild(root_ptr);

				palette_area->saveTextureToTMX(doc, root_ptr);

				doc.SaveFile("test.xml");
				free(save_path);
			}
			else if (result == NFD_CANCEL) 
				std::cout << "Save cancelled " << std::endl;
			
			else
				std::cout << "Save error" << std::endl;

			saving = false;
		}
		ImGui::End();
	}

	if (edit_area == nullptr || palette_area == nullptr) 
		return;
	allow_input_to_canvas = palette_area->allowControl() && inspector_area->allowControl();

	// Interpret mouse motion
	if (allow_input_to_canvas) {
		// Handle dragging
		if(fsm.getCurState() == UC_States::EDIT_DRAGGING_LEFT)
			edit_area->onDrag(false);
		if (fsm.getCurState() == UC_States::EDIT_DRAGGING_RIGHT)
			edit_area->onDrag(true);
		if (fsm.getCurState() == UC_States::PALETTE_DRAGGING)
			palette_area->onDrag();

		// Movement
		if (mouse.middle_click) {
			if (mouse.focused_window == FOCUSED_EDIT) {
				cho::Vector2f final_motion = mouse.motion * (-1 / edit_area->view_scale);
				edit_area->camera_pos = edit_area->camera_pos + final_motion;

			}
			if (mouse.focused_window == FOCUSED_PALETTE) {
				Camera camera = palette_area->getCurrentCamera();
				cho::Vector2f final_motion = mouse.motion * (-1 / camera.view_scale);
				palette_area->setCameraPosition(camera.camera_pos + final_motion);
			}
		}

		// Mouse wheel
		float scale_multiplier = 1.0f - (mouse.wheel_motion * delta * wheel_speed);
		if (mouse.focused_window == FOCUSED_EDIT)
			edit_area->view_scale = std::max(0.1f, edit_area->view_scale * scale_multiplier);
		if (mouse.focused_window == FOCUSED_PALETTE)
			palette_area->setViewScale(std::max(0.1f, palette_area->getCurrentCamera().view_scale * scale_multiplier));
	}

	mouse.resetOnFrameEnd();
}

void TileMapEditor::draw(cho::SDLPointers pointers) {
	if (edit_area != nullptr && palette_area != nullptr) {
		edit_area->selected_layer = inspector_area->selected;
		edit_area->selection = palette_area->getTileSelection();
		edit_area->selected_brush = inspector_area->selected_brush;
		edit_area_rend = draw_edit_area_texture(pointers.renderer, SDL_PIXELFORMAT_RGBA8888, window_w, window_h, *edit_area, mouse.focused_window, palette_area->getTextures(), inspector_area->visible_layers);
		select_area_rend = draw_palette_area_texture(pointers.renderer, SDL_PIXELFORMAT_RGBA8888, window_w, window_h, *palette_area, mouse.focused_window);
		inspector_area_rend = draw_inspector_area_texture(pointers.renderer, SDL_PIXELFORMAT_RGBA8888, window_w, window_h, *inspector_area);
	}

	SDL_Color clear_color = start_data->clear_color;
	ImGui::Render();
	SDL_RenderSetScale(pointers.renderer, io->DisplayFramebufferScale.x, io->DisplayFramebufferScale.y);
	SDL_SetRenderDrawColor(pointers.renderer, clear_color.r, clear_color.g, clear_color.b, clear_color.a);
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
}

void TileMapEditor::lateUpdate(float delta) {
	SDL_DestroyTexture(edit_area_rend);
	SDL_DestroyTexture(select_area_rend);
	SDL_DestroyTexture(inspector_area_rend);
}

std::shared_ptr<void> TileMapEditor::processDeath() {
	if(palette_area)
		palette_area->destroy();
	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	return nullptr;
}

bool TileMapEditor::isEndOfScene() {
	return !running;
}

