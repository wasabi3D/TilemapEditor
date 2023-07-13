#ifndef TILEMAPEDITOR_PALETTEAREA_H
#define TILEMAPEDITOR_PALETTEAREA_H

#include <SDL.h>
#include <SDL_image.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <iostream>
#include <vector>
#include <filesystem>
#include <map>
#include <functional>
#include <nfd.h>
#include "chomusuke/common.h"
#include "useful.h"
#include "tinyxml2.h"


struct Camera {
	cho::Vector2f camera_pos{};
	float view_scale{ 1.0f };
};

struct TextureData {
	int first_tile_id;
	int texture_tile_width;
	int texture_tile_height;
};

class PaletteArea {
	const int max_texture{ 100 };
	int tile_pixel_size{ 16 };
	int current_texture{ -1 };
	std::map<int, Camera> cameras{};
	std::map<int, Texture> textures{};

	TileID focused{ -1, -1 };
	int selected_texture{ -1 };
	TileID dragOrigin{ -1, -1 };
	TileSelection selection;
	SDL_Texture* cur_texture_ptr{ nullptr };
	int texture_width{ 1 };
	int texture_height{ 1 };
	cho::Vector2f on_screen_origin{};
	float on_screen_tile_size{ 1 };
	int texture_tile_w{ 1 };
	int texture_tile_h{ 1 };
	float view_scale{ DEFAULT_VIEW_SCALE };
	cho::Vector2f camera_pos{DEFAULT_CAM_POS};

	bool deleting_texture{ false };
	int delete_texture_id{ -1 };

	bool replace_warning{ false };
	Texture replace_new_texture{};
	int replace_target_id{ -1 };

public:
	SDL_Color line_color{ 140, 140, 140, 255 };
	SDL_Color highlight_line_color{ 240, 240, 240, 255 };
	SDL_Color selected_color{ 190, 190, 190, 255 };
	SDL_Color clear_color{ 0, 0, 0, 255 };
	std::function<void(int)> editOnCloseTexture;
	std::function<void(int, int, int)> editOnReplaceRemoveTiles;

	PaletteArea() = default;
	PaletteArea(int tile_pixel_size_) :
		tile_pixel_size{ tile_pixel_size_ }
	{}

	void drawCurrent(SDL_Renderer* renderer, SDL_Texture* texture, int texture_id);
	void drawToTexture(SDL_Renderer* renderer, SDL_Texture* texture, int view_w, int view_h);
	void drawFocused(SDL_Color color, SDL_Renderer* renderer);
	void drawSelection(SDL_Color color, SDL_Renderer* renderer);
	int getAvailableID();
	const std::map<int, Texture>& getTextures() const { return textures; }
	TileID getTileID(int mouse_x, int mouse_y);
	void setCameraPosition(const cho::Vector2f& new_pos) { cameras[current_texture].camera_pos = new_pos; }
	void setViewScale(float new_scale) { cameras[current_texture].view_scale = new_scale; }
	void onLeftClick();
	void onStartDrag();
	void onDrag();
	bool isValidFocus();
	void addTexture(int id, Texture texture) { textures[id] = texture; }
	void deleteTexture();
	void precalculateEssentials();
	void destroy();
	void askTexture(SDL_Renderer* renderer, bool replace_mode);
	bool allowControl() { return !(deleting_texture || replace_warning); }
	TileSelection getTileSelection() const { return selection; }
	Camera getCurrentCamera() { 
		if (cameras.count(current_texture) == 0) {
			Camera cm = { .camera_pos = DEFAULT_CAM_POS, .view_scale = DEFAULT_VIEW_SCALE };
			cameras.insert({ current_texture, cm });
		}
		return cameras.at(current_texture); 
	}
	std::map<int, TextureData> saveTextureToTMX(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* map_elm_ptr);


private:
	void initialize_selection();
	void processTexture(Texture& texture, bool replace_mode);
	bool askDeleteTexture(int view_w, int view_h);
	void askReplaceTexture();
};


SDL_Texture* draw_palette_area_texture(
	SDL_Renderer* renderer,
	Uint32 format,
	int window_w,
	int window_h,
	PaletteArea& palette_area,
	int& focusflag);


#endif