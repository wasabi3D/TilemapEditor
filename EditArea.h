#ifndef TILEMAPEDITOR_EDITAREA_H
#define TILEMAPEDITOR_EDITAREA_H

#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <iostream>
#include <vector>
#include <map>
#include "chomusuke/common.h"
#include "useful.h"

constexpr int FOCUSED_EDIT{ 1 };

class EditArea {
	int tile_pixel_size{ 16 };
	std::vector<TileMap> tilemap{};
	TileID topleft{};
	TileID focused{-1, -1};
	int tilemap_width = 0;
	int tilemap_height = 0;

// PUBLIC MEMBERS
public:
	cho::Vector2f camera_pos{ 0, 0 };
	float view_scale{ 1 };
	SDL_Color line_color{ 150, 150, 150, 255 };
	SDL_Color highlight_line_color{ 200, 200, 200, 255 };
	SDL_Color clear_color{ 0, 0, 0, 255 };
	size_t selected_layer{ 0 };

// PUBLIC FUNCTIONS
public:
	EditArea() = default;
	EditArea(int tile_size, int tilemap_w, int tilemap_h):
		tile_pixel_size(tile_size),
		tilemap{},
		tilemap_width(tilemap_w),
		tilemap_height(tilemap_h)
	{}
	TileID getTileID(int mouse_x, int mouse_y);
	void drawToTexture(SDL_Renderer* renderer, SDL_Texture* texture, const std::map<int, Texture>& ref_textures, std::map<int, Tilemap_visible>& visibles);
	void onLeftClick(Tile selected_tile);
	void onAddLayer();
	void onDeleteLayer();
	void onSwap(int a, int b);
};


SDL_Texture* draw_edit_area_texture(
	SDL_Renderer* renderer,
	Uint32 format,
	int window_w,
	int window_h,
	EditArea& editarea,
	int& focusflag,
	const std::map<int, Texture>& ref_textures,
	std::map<int, Tilemap_visible>& visibles);

#endif