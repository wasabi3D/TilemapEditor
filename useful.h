#ifndef TILEMAPEDITOR_USEFUL_H
#define TILEMAPEDITOR_USEFUL_H

#include <vector>
#include <imgui.h>

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
	TileID id_on_texture{};

	Tile() = default;
};

struct Texture {
	std::string name{};
	SDL_Texture* texture{ nullptr };
};


inline bool isCursorInsideWindow(ImVec2 cursor, ImVec2 window_pos, ImVec2 window_dim) {
	return (window_pos.x <= cursor.x && cursor.x <= window_pos.x + window_dim.x) &&
		(window_pos.y <= cursor.y && cursor.y <= window_pos.y + window_dim.y);
}

struct Tilemap_visible {
	bool visible = true;
};


using TileMap = std::vector<std::vector<Tile>>;

#endif
