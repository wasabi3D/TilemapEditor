#include "EditArea.h"

TileID EditArea::getTileID(int mouse_x, int mouse_y) {
	int
		tile_x = std::floor((float)mouse_x / (tile_pixel_size * view_scale)),
		tile_y = std::floor((float)mouse_y / (tile_pixel_size * view_scale));

	return TileID(tile_x, tile_y);
}

void EditArea::drawToTexture(SDL_Renderer* renderer, SDL_Texture* texture, const std::map<int, Texture>& ref_textures, std::map<int, Tilemap_visible>& visibles) {
	// Draw lines
	float on_screen_tile_size = view_scale * tile_pixel_size;
	cho::Vector2f on_screen_origin = camera_pos * -view_scale;

	SDL_SetRenderDrawColor(renderer, clear_color.r, clear_color.g, clear_color.b, clear_color.a);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, line_color.r, line_color.g, line_color.b, line_color.a);
	// Vertical lines
	for (int i = 0; i < tilemap_width + 1; i++) {
		int
			x1 = i * on_screen_tile_size + on_screen_origin.x,
			y1 = on_screen_origin.y,
			x2 = x1,
			y2 = y1 + tilemap_height * on_screen_tile_size;
		SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	}

	// Horizontal lines
	for (int i = 0; i < tilemap_height + 1; i++) {
		int
			x1 = on_screen_origin.x,
			y1 = i * on_screen_tile_size + on_screen_origin.y,
			x2 = x1 + tilemap_width * on_screen_tile_size,
			y2 = y1;
		SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	}

	// Render tiles
	for (size_t layer = 0; layer < tilemap.size(); layer++) {
		if (!visibles[layer].visible) continue;
		for (size_t h = 0; h < tilemap[layer].size(); h++) for (size_t w = 0; w < tilemap[layer][h].size(); w++) {
			Tile tile = tilemap[layer][h][w];
			if (tile.texture_id == -1) continue;

			float rend_size = tile_pixel_size * view_scale;
			SDL_Rect
				src_rect{ tile_pixel_size * tile.id_on_texture.x, tile_pixel_size * tile.id_on_texture.y, tile_pixel_size, tile_pixel_size },
				target_rect{ on_screen_origin.x + rend_size * w, on_screen_origin.y + rend_size * h, std::ceil(rend_size), std::ceil(rend_size) };
			SDL_RenderCopy(renderer, ref_textures.at(tile.texture_id).texture, &src_rect, &target_rect);
		}
	}

	// Focused tile
	ImVec2 pos = ImGui::GetMousePos();
	pos.x -= (on_screen_origin.x + ImGui::GetWindowPos().x);
	pos.y -= (on_screen_origin.y + ImGui::GetWindowPos().y + 35);  // Menu bar offset
	focused = getTileID(pos.x, pos.y);
	if (!(focused.x < 0 || focused.x >= tilemap_width || focused.y < 0 || focused.y >= tilemap_height)) {
		SDL_SetRenderDrawColor(renderer, highlight_line_color.r, highlight_line_color.g, highlight_line_color.b, highlight_line_color.a);
		int
			topleft_x = focused.x * tile_pixel_size * view_scale + on_screen_origin.x,
			topleft_y = focused.y * tile_pixel_size * view_scale + on_screen_origin.y;
		int dim = tile_pixel_size * view_scale;

		SDL_RenderDrawLine(renderer, topleft_x, topleft_y, topleft_x + dim, topleft_y);
		SDL_RenderDrawLine(renderer, topleft_x, topleft_y + dim, topleft_x + dim, topleft_y + dim);
		SDL_RenderDrawLine(renderer, topleft_x, topleft_y, topleft_x, topleft_y + dim);
		SDL_RenderDrawLine(renderer, topleft_x + dim, topleft_y, topleft_x + dim, topleft_y + dim);
	}

	SDL_SetRenderTarget(renderer, nullptr);
}


void EditArea::onLeftClick(Tile selected_tile) {
	if (focused.x < 0 || focused.x >= tilemap_width || focused.y < 0 || focused.y >= tilemap_height) {
		return;
	}
	tilemap[selected_layer][focused.y][focused.x] = selected_tile;
}

void EditArea::onAddLayer() {
	TileMap new_map(tilemap_height, std::vector<Tile>(tilemap_width, Tile()));
	tilemap.push_back(new_map);
}

void EditArea::onDeleteLayer() {
	tilemap.erase(tilemap.begin() + selected_layer);
}

void EditArea::onSwap(int a, int b) {
	std::swap(tilemap.at(a), tilemap.at(b));
}

SDL_Texture* draw_edit_area_texture(
	SDL_Renderer* renderer, Uint32 format, int window_w, int window_h,
	EditArea& editarea, int& focusflag, const std::map<int, Texture>& ref_textures,
	std::map<int, Tilemap_visible>& visibles)
{
	/* PREPARATION */
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	Uint32 window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove | 
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
	ImGui::Begin("Edit area", (bool*)0, window_flags);
	ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Once);
	ImGui::SetWindowSize(ImVec2(600, (float)window_h), ImGuiCond_Once);
	ImVec2 size = ImGui::GetWindowSize();
	ImVec2 pos = ImGui::GetWindowPos();
	if (!ImGui::IsItemHovered() && isCursorInsideWindow(ImGui::GetMousePos(), pos, size))
		focusflag = FOCUSED_EDIT;

	SDL_Texture* texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_TARGET, (int)size.x, (int)size.y);
	if (!texture) {
		std::cerr << "Failed to create edit texture" << std::endl;
		return nullptr;
	}

	/* ACTUAL RENDERING */
	SDL_SetRenderTarget(renderer, texture);
	editarea.drawToTexture(renderer, texture, ref_textures, visibles);

	ImGui::Image((void*)texture, ImVec2(size.x, size.y));
	
	/* END */
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Reset view...")) {
				editarea.camera_pos = { 0, 0 };
				editarea.view_scale = 1.0f;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
	ImGui::End();
	ImGui::PopStyleVar();
	SDL_SetRenderTarget(renderer, nullptr);
	return texture;
}


