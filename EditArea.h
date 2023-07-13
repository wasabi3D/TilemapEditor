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
#include "chomusuke/math.h"
#include "useful.h"


class EditArea {
	int tile_pixel_size{ 16 };
	std::vector<TileMap> tilemap{};
	TileID topleft{};
	TileID focused{ -1, -1 };
	int tilemap_width = 0;
	int tilemap_height = 0;

	float on_screen_tile_size{ 1 };
	cho::Vector2f on_screen_origin{};

	TileID dragOrigin{ -1, -1 };
	TileID dragTopLeft{ -1, -1 };
	TileID dragBottomRight{ -1, -1 };
	TileMap rect_preview{};
	int rect_preview_layer{ 0 };
	int selection_width{ 1 };
	int selection_height{ 1 };
	bool rect_clear{ false };

	// PUBLIC MEMBERS
public:
	cho::Vector2f camera_pos{ DEFAULT_CAM_POS };
	float view_scale{ DEFAULT_VIEW_SCALE };
	SDL_Color line_color{ 150, 150, 150, 255 };
	SDL_Color highlight_line_color{ 200, 200, 200, 255 };
	SDL_Color clear_color{ 0, 0, 0, 255 };
	size_t selected_layer{ 0 };
	TileSelection selection;
	int selected_brush{ BRUSH_BASIC };
	std::map<int, Tilemap_visible>* visibles{ nullptr };

	// PUBLIC FUNCTIONS
public:
	EditArea() = default;
	EditArea(int tile_size, int tilemap_w, int tilemap_h) :
		tile_pixel_size(tile_size),
		tilemap{},
		tilemap_width(tilemap_w),
		tilemap_height(tilemap_h)
	{}
	TileID getTileID(int mouse_x, int mouse_y);
	void drawToTexture(
		SDL_Renderer* renderer,
		SDL_Texture* texture,
		const std::map<int, Texture>& ref_textures,
		int view_w,
		int view_h);
	void onPlace(bool clear = false);
	void onAddLayer();
	void onDeleteLayer(int layer);
	void onSwap(int a, int b);
	void onStartDrag(bool clear = false);
	void onDrag(bool clear_if_basic_brush = false);
	void onEndDrag(bool cancelled = false);
	// Whether the mouse cursor is inside the grid
	bool isValidHover();
	// Whether the entire brush is inside the grid
	bool isValidFocus();
	void renderFocus(SDL_Color color, SDL_Renderer* renderer);
	void onDeleteTexture(int id);
	void editOnReplaceRemoveTiles(int texture_id, int max_x, int max_y);

private:
	void renderTilemap(
		SDL_Renderer* renderer,
		const std::map<int, Texture>& ref_textures,
		cho::Vector2i topleft,
		cho::Vector2i bottomright,
		TileMap& target, 
		bool is_preview_layer);
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