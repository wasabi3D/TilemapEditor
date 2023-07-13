#ifndef TILEMAPEDITOR_USEFUL_H
#define TILEMAPEDITOR_USEFUL_H

#include <vector>
#include <imgui.h>
#include "chomusuke/common.h"

constexpr int BRUSH_BASIC{ 0 };
constexpr int BRUSH_RECTANGLE{ 1 };
constexpr int FOCUSED_EDIT{ 1 };
constexpr int FOCUSED_PALETTE{ 2 };
constexpr float EDIT_WIDTH{ 0.5f };
constexpr float PALETTE_WIDTH{ 0.3f };
constexpr float INSPECTOR_WIDTH{ 0.2f };

constexpr float DEFAULT_VIEW_SCALE{ 1.0f };
const cho::Vector2f DEFAULT_CAM_POS{ -1, 0 };


const ImGuiWindowFlags popup_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;

struct TileID {
	int x{ 0 };
	int y{ 0 };
	TileID() = default;
	TileID(int x_, int y_) :
		x{ x_ },
		y{ y_ }
	{}
};

struct Tile {
	int texture_id{ -1 };
	TileID id_on_texture{ -1, -1 };

	Tile() = default;
};

struct TileSelection {
	Tile topleft;
	TileID bottomright{ -1, -1 };
};

struct Texture {
	std::string name{};
	std::string path{};
	SDL_Texture* texture{ nullptr };
};

inline bool isCursorInsideWindow(ImVec2 cursor, ImVec2 window_pos, ImVec2 window_dim) {
	return (window_pos.x <= cursor.x && cursor.x <= window_pos.x + window_dim.x) &&
		(window_pos.y <= cursor.y && cursor.y <= window_pos.y + window_dim.y);
}

struct Tilemap_visible {
	bool visible = true;
};

inline bool isValidSelection(TileSelection selection) {
	return !(selection.topleft.id_on_texture.x < 0 || selection.topleft.id_on_texture.y < 0 ||
		selection.bottomright.x < 0 || selection.bottomright.y < 0);
}

using TileMap = std::vector<std::vector<Tile>>;

#endif
