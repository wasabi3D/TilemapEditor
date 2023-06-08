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
public:
	std::function<void()> on_add_layer;
	std::function<void()> on_delete_layer;
	std::function<void(int, int)> on_swap;
	std::vector<std::string> layer_names;
	std::map<int, Tilemap_visible> visible_layers;
	size_t selected = 0;
	bool renaming = false;
	InspectorArea() = default;

	void addNewLayer();
	void deleteLayer();
	void drawToTexture();
	bool swap(int a, int b);
};

SDL_Texture* draw_inspector_area_texture(
	SDL_Renderer* renderer,
	Uint32 format,
	int window_w,
	int window_h,
	InspectorArea& inspector
	);

#endif