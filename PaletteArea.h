#ifndef TILEMAPEDITOR_SELECTAREA_H
#define TILEMAPEDITOR_SELECTAREA_H

#include <SDL.h>
#include <SDL_image.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <iostream>
#include <vector>
#include <filesystem>
#include <map>
#include <nfd.h>
#include "chomusuke/common.h"
#include "useful.h"

constexpr int FOCUSED_PALETTE{ 2 };


struct Camera {
	cho::Vector2f camera_pos{};
	float view_scale{ 1.0f };
};

class PaletteArea {
	const int max_texture{ 100 };
	int tile_pixel_size{ 16 };
	int current_texture{ -1 };
	std::map<int, Camera> cameras;
	std::map<int, Texture> textures;

	TileID focused{ -1, -1 };
	int selected_texture{ -1 };
	TileID selected{ -1, -1 };

public:
	SDL_Color line_color{ 140, 140, 140, 255 };
	SDL_Color highlight_line_color{ 240, 240, 240, 255 };
	SDL_Color selected_color{ 190, 190, 190, 255 };
	SDL_Color clear_color{ 0, 0, 0, 255 };

	PaletteArea() = default;
	PaletteArea(int tile_pixel_size_) :
		tile_pixel_size{ tile_pixel_size_ }
	{}

	void drawCurrent(SDL_Renderer* renderer, SDL_Texture* texture, int texture_id);
	void drawToTexture(SDL_Renderer* renderer, SDL_Texture* texture);
	int getAvailableID();
	void addTexture(int id, Texture texture) { textures[id] = texture; }
	Camera getCurrentCamera() { 
		if (cameras.count(current_texture) == 0) {
			Camera cm = { .camera_pos = {-1, 0}, .view_scale = 1.0f };
			cameras.insert({ current_texture, cm });
		}
		return cameras.at(current_texture); 
	}
	void setCameraPosition(const cho::Vector2f& new_pos) { cameras[current_texture].camera_pos = new_pos; }
	void setViewScale(float new_scale) { cameras[current_texture].view_scale = new_scale; }
	TileID getTileID(int mouse_x, int mouse_y);
	void drawFocused(TileID focused, SDL_Color color, SDL_Renderer* renderer, const cho::Vector2f& on_screen_origin,
		int texture_tile_w, int texture_tile_h, float view_scale);
	void onLeftClick();
	Tile getSelectedTile() const {
		Tile tile;
		tile.texture_id = selected_texture;
		tile.id_on_texture = selected;
		return tile;
	}
	const std::map<int, Texture>& getTextures() const { return textures; }
	void destroy();
};


SDL_Texture* draw_palette_area_texture(
	SDL_Renderer* renderer,
	Uint32 format,
	int window_w,
	int window_h,
	PaletteArea& palette_area,
	int& focusflag);


#endif