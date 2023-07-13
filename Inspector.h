#ifndef TILEMAPEDITOR_INSPECTOR_H
#define TILEMAPEDITOR_INSPECTOR_H

#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <iostream>
#include <functional>
#include <vector>
#include <map>
#include "chomusuke/common.h"
#include "useful.h"


class InspectorArea {
	bool renaming{ false };
	bool _tmp_do_not_show_again{ false };
	bool show_delete_warn{ true };
	bool show_framerate{ false };
	int deleting_layer{ -1 };

public:
	std::function<void()> on_add_layer;
	std::function<void(int)> on_delete_layer;
	std::function<void(int, int)> on_swap;
	std::vector<std::string> layer_names;
	std::map<int, Tilemap_visible> visible_layers;
	ImGuiIO* io{ nullptr };
	size_t selected = 0;
	int selected_brush{ 0 };
	InspectorArea() = default;

	void addNewLayer();
	void deleteLayer();
	void drawToTexture(int view_w, int view_h);
	bool swap(int a, int b);
	bool allowControl(){ return !(renaming || (deleting_layer != -1)); }
};

SDL_Texture* draw_inspector_area_texture(
	SDL_Renderer* renderer,
	Uint32 format,
	int window_w,
	int window_h,
	InspectorArea& inspector
	);

#endif