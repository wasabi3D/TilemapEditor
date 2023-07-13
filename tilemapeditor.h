#ifndef __TILEMAPEDITOR_H
#define __TILEMAPEDITOR_H

#include <tinyxml2.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <memory>
#include <iostream>
#include <string>
#include "chomusuke/infrastructure.h"
#include "chomusuke/FSM.h"
#include "EditArea.h"
#include "PaletteArea.h"
#include "Inspector.h"

const std::string TMX = ".tmx";
const std::string PNG = ".png";

const std::map<std::string, std::string> format_tooltip = {
	{TMX, "Uses the TMX format provided by Tiled. \nIt keeps texture/layer/name/hitbox data, and can be edited later."},
	{PNG, "Renders the entire tilemap to a png file. \nIt becomes a literal image, so you'll just be able to display it and nothing more."}
};

constexpr bool canDrag(int window, int drag_window) { return drag_window == -1 || window == drag_window;  }

enum class UserControlStates {
	DEFAULT,
	EDIT_DRAGGING_LEFT,
	EDIT_DRAGGING_RIGHT,
	PALETTE_DRAGGING
};

enum class UserControlTransitions {
	EDIT_START_DRAG_LEFT,
	EDIT_END_DRAG_LEFT,
	EDIT_START_DRAG_RIGHT,
	EDIT_END_DRAG_RIGHT,
	PALETTE_START_DRAG,
	PALETTE_END_DRAG
};

using UC_States = UserControlStates;
using UC_Transitions = UserControlTransitions;
using TM_FSM = cho::FSM<UC_States, UC_Transitions>;
using TM_State = cho::State<UC_States>;
using TM_Transition = cho::Transition<UC_Transitions, UC_States>;

struct MouseMotion {
	int
		focused_window = -1,
		drag_window = -1;
	cho::Vector2f motion{};
	float wheel_motion = 0;
	bool
		middle_click = false,
		left_click = false,
		right_click = false,
		start_dragging = false,
		start_dragging_right = false;
	
	void resetOnFrameEnd() {
		wheel_motion = 0;
		focused_window = -1;
		start_dragging = false;
		start_dragging_right = false;
	}
};

struct TileMapStartupData {
	int window_w = 1;
	int window_h = 1;
	SDL_Color clear_color{ 10, 2, 2, 255 };
};

class TileMapEditor : public cho::IScene {
	ImGuiIO* io{ nullptr };
	SDL_Texture* edit_area_rend{ nullptr };
	SDL_Texture* select_area_rend{ nullptr };
	SDL_Texture* inspector_area_rend{ nullptr };
	std::unique_ptr<EditArea> edit_area;
	std::unique_ptr<PaletteArea> palette_area;
	std::unique_ptr<InspectorArea> inspector_area;
	TM_FSM fsm;
	
	std::shared_ptr<TileMapStartupData> start_data{ nullptr };
	int window_w = 1;
	int window_h = 1;
	int map_w = 1;
	int map_h = 1;
	int tile_size = 1;

	std::string cur_format = TMX;
	
	const float wheel_speed = 1.2f;
	bool 
		creating_new = true,	
		saving = false,
		running = true,
		allow_input_to_canvas = false;
	MouseMotion mouse;
	
	void init_viewport();
public:
	void start(std::shared_ptr<void> data, cho::SDLPointers pointers) override;
	void processEvent(const SDL_Event& event) override;
	void update(float delta) override;
	void draw(cho::SDLPointers pointers) override;
	std::shared_ptr<void> processDeath() override;
	bool isEndOfScene() override;
	void lateUpdate(float delta) override;
};


#endif