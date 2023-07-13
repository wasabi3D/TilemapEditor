#include "chomusuke/infrastructure.h"
#include "tilemapeditor.h"
#undef main


constexpr int window_w = 1800;
constexpr int window_h = 800;

int main() {
	cho::LoopManager manager;
	manager.init("Tilemap Editor", 100, 100, window_w, window_h, {10, 2, 2, 255}, SDL_WINDOW_SHOWN, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC, SDL_BLENDMODE_BLEND);
	auto data = std::make_shared<TileMapStartupData>();
	data->window_w = window_w;
	data->window_h = window_h;
	manager.executeScene(std::make_shared<TileMapEditor>(), data);
	return 0;
}
