#include "PaletteArea.h"


TileID PaletteArea::getTileID(int mouse_x, int mouse_y) {
	Camera cm = getCurrentCamera();
	float view_scale = cm.view_scale;
	cho::Vector2f camera_pos = cm.camera_pos;
	int
		tile_x = std::floor((float)mouse_x / (tile_pixel_size * view_scale)),
		tile_y = std::floor((float)mouse_y / (tile_pixel_size * view_scale));

	return TileID(tile_x, tile_y);
}

void PaletteArea::onLeftClick() {
	if (textures.count(current_texture) == 0) return;

	SDL_Texture* cur_texture = textures[current_texture].texture;
	int texture_width, texture_height;
	SDL_QueryTexture(cur_texture, nullptr, nullptr, &texture_width, &texture_height);
	int
		texture_tile_w = texture_width / tile_pixel_size,
		texture_tile_h = texture_height / tile_pixel_size;
	if (focused.x < 0 || focused.x >= texture_tile_w || focused.y < 0 || focused.y >= texture_tile_h)
		return;

	selected_texture = current_texture;
	selected = focused;
	return;
}

void PaletteArea::drawFocused(TileID focused, SDL_Color color, SDL_Renderer* renderer, const cho::Vector2f& on_screen_origin,
	int texture_tile_w, int texture_tile_h, float view_scale) 
{
	if (!(focused.x < 0 || focused.x >= texture_tile_w || focused.y < 0 || focused.y >= texture_tile_h)) {
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		int
			topleft_x = focused.x * tile_pixel_size * view_scale + on_screen_origin.x,
			topleft_y = focused.y * tile_pixel_size * view_scale + on_screen_origin.y;
		int dim = tile_pixel_size * view_scale;

		SDL_RenderDrawLine(renderer, topleft_x, topleft_y, topleft_x + dim, topleft_y);
		SDL_RenderDrawLine(renderer, topleft_x, topleft_y + dim, topleft_x + dim, topleft_y + dim);
		SDL_RenderDrawLine(renderer, topleft_x, topleft_y, topleft_x, topleft_y + dim);
		SDL_RenderDrawLine(renderer, topleft_x + dim, topleft_y, topleft_x + dim, topleft_y + dim);
	}
}


void PaletteArea::drawCurrent(SDL_Renderer* renderer, SDL_Texture* texture, int texture_id) {
	// Precalculate some info
	Camera cm = getCurrentCamera();
	float view_scale = cm.view_scale;
	cho::Vector2f camera_pos = cm.camera_pos;
	float on_screen_tile_size = view_scale * tile_pixel_size;
	cho::Vector2f on_screen_origin = camera_pos * -view_scale;
	SDL_Texture* cur_texture = textures[texture_id].texture;
	int texture_width, texture_height;
	SDL_QueryTexture(cur_texture, nullptr, nullptr, &texture_width, &texture_height);
	int
		on_screen_w = texture_width * view_scale,
		on_screen_h = texture_height * view_scale;
	int 
		texture_tile_w = texture_width / tile_pixel_size, 
		texture_tile_h = texture_height / tile_pixel_size;

	SDL_SetRenderDrawColor(renderer, line_color.r, line_color.g, line_color.b, line_color.a);
	// Vertical lines
	for (size_t i = 0; i < texture_tile_w + 1; i++) {
		int
			x1 = i * on_screen_tile_size + on_screen_origin.x,
			y1 = on_screen_origin.y,
			x2 = x1,
			y2 = y1 + texture_tile_h * on_screen_tile_size;
		SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	}
	// Horizontal lines
	for (size_t i = 0; i < texture_tile_h + 1; i++) {
		int
			x1 = on_screen_origin.x,
			y1 = i * on_screen_tile_size + on_screen_origin.y,
			x2 = x1 + texture_tile_w * on_screen_tile_size,
			y2 = y1;
		SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	}

	SDL_Rect dst_rect{ on_screen_origin.x, on_screen_origin.y, on_screen_w, on_screen_h };
	SDL_RenderCopy(renderer, cur_texture, nullptr, &dst_rect);

	// Focused tile
	ImVec2 pos = ImGui::GetMousePos();
	pos.x -= (on_screen_origin.x + ImGui::GetWindowPos().x);
	pos.y -= (on_screen_origin.y + ImGui::GetWindowPos().y + 35 + 27);  // Menu bar offset + Tab bar offset
	focused = getTileID(pos.x, pos.y);
	drawFocused(focused, highlight_line_color, renderer, on_screen_origin, texture_tile_w, texture_tile_h, view_scale);

	if (selected_texture == current_texture) {
		drawFocused(selected, selected_color, renderer, on_screen_origin, texture_tile_w, texture_tile_h, view_scale);
	}

}

void PaletteArea::drawToTexture(SDL_Renderer* renderer, SDL_Texture* texture) {
	SDL_SetRenderDrawColor(renderer, clear_color.r, clear_color.g, clear_color.b, clear_color.a);
	SDL_RenderClear(renderer);

	/* TAB BAR */
	ImGuiTabBarFlags tb_flags = ImGuiTabBarFlags_NoCloseWithMiddleMouseButton;
	if (ImGui::BeginTabBar("palette", tb_flags)) {
		for (const std::pair<int, Texture>& texture_obj : textures) {
			if (ImGui::BeginTabItem(texture_obj.second.name.c_str())) {
				current_texture = texture_obj.first; /// kore dupe
				drawCurrent(renderer, texture, texture_obj.first);
				ImGui::EndTabItem();
			}
		}
	ImGui::EndTabBar();
	}
}

int PaletteArea::getAvailableID() {
	for (int i = 0; i < max_texture; i++) {
		if (textures.count(i) == 0) {
			return i;
		}
	}

	return -1;
}

void askTexture(SDL_Renderer* renderer, PaletteArea& palette_area) {
	nfdchar_t* outPath = nullptr;
	nfdresult_t result = NFD_OpenDialog("png", NULL, &outPath);
	if (result == NFD_OKAY) {
		std::cout << "Success: " << outPath << std::endl;
		Texture load;
		load.name = std::filesystem::path(std::string(outPath)).filename().string();
		load.name = std::to_string(palette_area.getTextures().size()) + ". " + load.name;
		load.texture = cho::loadTexture(outPath, renderer);
		if (load.texture) {
			int texture_id = palette_area.getAvailableID();
			if (texture_id != -1) {
				palette_area.addTexture(texture_id, load);
			}
			else {
				std::cout << "No texture slot available!" << std::endl;
				SDL_DestroyTexture(load.texture);
				load.texture = nullptr;
			}
		}
		else {
			std::cout << "Texture allocation failed!" << std::endl;
		}
		free(outPath);
	}
	else if (result == NFD_CANCEL) {
		std::cout << "Cancelled" << std::endl;
	}
	else {
		std::cout << "Error: " << NFD_GetError() << std::endl;
	}
}

void PaletteArea::destroy() {
	for (auto& p : textures) {
		SDL_DestroyTexture(p.second.texture);
	}
}

SDL_Texture* draw_palette_area_texture(
	SDL_Renderer* renderer,
	Uint32 format,
	int window_w,
	int window_h,
	PaletteArea& palette_area,
	int& focusflag)
{
	/* PREPARATION */
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	Uint32 window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
	ImGui::Begin("Palette", (bool*)0, window_flags);
	ImGui::SetWindowPos(ImVec2(600, 0), ImGuiCond_Once);
	ImGui::SetWindowSize(ImVec2(600, (float)window_h), ImGuiCond_Once);
	ImVec2 size = ImGui::GetWindowSize();
	ImVec2 pos = ImGui::GetWindowPos();
	if (!ImGui::IsItemHovered() && isCursorInsideWindow(ImGui::GetMousePos(), pos, size))
		focusflag = FOCUSED_PALETTE;

	/* MENU BAR */
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("Texture")) {
			if (ImGui::MenuItem("Load new texture...")) {
				askTexture(renderer, palette_area);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Reset view...")) {
				palette_area.setCameraPosition({ -1, 0 });
				palette_area.setViewScale(1.0f);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	SDL_Texture* texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_TARGET, (int)size.x, (int)size.y);
	if (!texture) {
		std::cerr << "Failed to create edit texture" << std::endl;
		return nullptr;
	}

	/* ACTUAL RENDERING */
	SDL_SetRenderTarget(renderer, texture);
	palette_area.drawToTexture(renderer, texture);

	ImGui::Image((void*)texture, ImVec2(size.x, size.y));

	/* END */
	ImGui::End();
	ImGui::PopStyleVar();
	SDL_SetRenderTarget(renderer, nullptr);
	return texture;
}