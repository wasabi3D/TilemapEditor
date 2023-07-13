#include "EditArea.h"

TileID EditArea::getTileID(int mouse_x, int mouse_y) {
	int
		tile_x = std::floor((float)mouse_x / (tile_pixel_size * view_scale)),
		tile_y = std::floor((float)mouse_y / (tile_pixel_size * view_scale));

	return TileID(tile_x, tile_y);
}

bool EditArea::isValidHover() {
	return !(focused.x < 0 || focused.x >= tilemap_width || focused.y < 0 || focused.y >= tilemap_height);
}

bool EditArea::isValidFocus() {
	if (!isValidSelection(selection))
		return false;

	if (!isValidHover())
		return false;

	return (focused.x + selection_width - 1 < tilemap_width) && (focused.y + selection_height - 1 < tilemap_height);
}

void EditArea::onPlace(bool clear) {
	if (!isValidFocus())
		return;

	Tile tile;
	for (int h = 0; h < selection_height; h++) for (int w = 0; w < selection_width; w++) {
		if (!clear) {
			tile.texture_id = selection.topleft.texture_id;
			tile.id_on_texture.x = selection.topleft.id_on_texture.x + w;
			tile.id_on_texture.y = selection.topleft.id_on_texture.y + h;
		}
		tilemap[selected_layer][(size_t)focused.y + h][(size_t)focused.x + w] = tile;
	}
}

void EditArea::onStartDrag(bool clear) {
	if (!isValidFocus())
		return;
	if (!visibles->operator[](selected_layer).visible)
		return;
	if (dragOrigin.x != -1)
		return;
	
	if (selected_brush == BRUSH_RECTANGLE) {
		rect_preview = TileMap(tilemap_height, std::vector<Tile>(tilemap_width, Tile()));
		dragOrigin = focused;
		dragTopLeft = focused;
		dragBottomRight = focused;
		rect_preview_layer = selected_layer;
		rect_clear = clear;
	}
}

void EditArea::onDrag(bool clear_if_basic_brush) {
	if (!visibles->operator[](selected_layer).visible)
		return;
	if (!isValidFocus())
		return;
	if (selected_brush == BRUSH_BASIC) {
		onPlace(clear_if_basic_brush);
		return;
	}

	if (selected_brush != BRUSH_RECTANGLE)
		return;
	if (dragOrigin.x == -1)
		return;
	dragTopLeft.x = std::min(dragOrigin.x, focused.x);
	dragTopLeft.y = std::min(dragOrigin.y, focused.y);
	dragBottomRight.x = std::max(dragOrigin.x, focused.x);
	dragBottomRight.y = std::max(dragOrigin.y, focused.y);

	int
		diff_x = dragBottomRight.x - dragTopLeft.x,
		diff_y = dragBottomRight.y - dragTopLeft.y;

	// Variation of w and h per iteration
	int
		dw = (focused.x >= dragOrigin.x ? 1 : -1),
		dh = (focused.y >= dragOrigin.y ? 1 : -1);
	// How much we moved in the x and y directions respectively
	int
		delta_w = 0,
		delta_h = 0;

	bool
		at_least_once_x = true,
		at_least_once_y = true;
	
	for (int h = dragOrigin.y; (h < dragOrigin.y + diff_y) || at_least_once_y;) {
		delta_w = 0;
		at_least_once_x = true;
		for (int w = dragOrigin.x; (w < dragOrigin.x + diff_x) || at_least_once_x;) {
			if (!rect_clear) {
				Tile drawn_tile;
				drawn_tile.texture_id = selection.topleft.texture_id;
				drawn_tile.id_on_texture.x = mod(delta_w + (focused.x >= dragOrigin.x ? 0 : -1), selection_width) + selection.topleft.id_on_texture.x;
				drawn_tile.id_on_texture.y = mod(delta_h + (focused.y >= dragOrigin.y ? 0 : -1), selection_height) + selection.topleft.id_on_texture.y;
				rect_preview[(size_t)(dragOrigin.y + delta_h)][(size_t)(dragOrigin.x + delta_w)] = drawn_tile;
			}

			w = dragOrigin.x + std::abs(delta_w);
			at_least_once_x = false;
			delta_w += dw;
		}

		h = dragOrigin.y + std::abs(delta_h);
		at_least_once_y = false;
		delta_h += dh;
	}
}

void EditArea::onEndDrag(bool cancelled) {
	if (!visibles->operator[](selected_layer).visible)
		return;
	if (dragOrigin.x == -1)
		return;
	if(!cancelled)
		for (int h = dragTopLeft.y; h <= dragBottomRight.y; h++) for (int w = dragTopLeft.x; w <= dragBottomRight.x; w++)
			tilemap[rect_preview_layer][h][w] = rect_preview[h][w];
	rect_preview.clear();
	dragOrigin = TileID(-1, -1);
	dragTopLeft = TileID(-1, -1);
	dragBottomRight = TileID(-1, -1);
	rect_preview_layer = -1;
}

void EditArea::onAddLayer() {
	TileMap new_map(tilemap_height, std::vector<Tile>(tilemap_width, Tile()));
	tilemap.push_back(new_map);
}

void EditArea::onDeleteLayer(int layer) {
	tilemap.erase(tilemap.begin() + layer);
}

void EditArea::onSwap(int a, int b) {
	std::swap(tilemap.at(a), tilemap.at(b));
}

void EditArea::onDeleteTexture(int id) {
	for (TileMap& layer : tilemap)
		for (std::vector<Tile>& row : layer)
			for (Tile& tile : row)
				if (tile.texture_id == id)
					tile = Tile();
}

void EditArea::editOnReplaceRemoveTiles(int texture_id, int max_x, int max_y) {
	for (TileMap& layer : tilemap)
		for (std::vector<Tile>& row : layer)
			for (Tile& tile : row)
				if (tile.texture_id == texture_id && (tile.id_on_texture.x > max_x || tile.id_on_texture.y > max_y))
					tile = Tile();
}

void EditArea::renderFocus(SDL_Color color, SDL_Renderer* renderer) {
	if (!isValidFocus())
		return;

	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	int
		topleft_x = focused.x * tile_pixel_size * view_scale + on_screen_origin.x,
		topleft_y = focused.y * tile_pixel_size * view_scale + on_screen_origin.y;

	int
		dim_x = tile_pixel_size * view_scale * selection_width,
		dim_y = tile_pixel_size * view_scale * selection_height;

	SDL_RenderDrawLine(renderer, topleft_x, topleft_y, topleft_x + dim_x, topleft_y);
	SDL_RenderDrawLine(renderer, topleft_x, topleft_y + dim_y, topleft_x + dim_x, topleft_y + dim_y);
	SDL_RenderDrawLine(renderer, topleft_x, topleft_y, topleft_x, topleft_y + dim_y);
	SDL_RenderDrawLine(renderer, topleft_x + dim_x, topleft_y, topleft_x + dim_x, topleft_y + dim_y);
}

void EditArea::renderTilemap(
	SDL_Renderer* renderer,
	const std::map<int, Texture>& ref_textures,
	cho::Vector2i topleft,
	cho::Vector2i bottomright,
	TileMap& target, 
	bool is_preview_layer)
{
	for (size_t h = topleft.y; h <= std::min(bottomright.y, tilemap_height - 1); h++)
	for (size_t w = topleft.x; w <= std::min(bottomright.x, tilemap_width - 1); w++) {
		bool use_preview = is_preview_layer && (dragOrigin.x != -1) &&
			(dragTopLeft.x <= w && w <= dragBottomRight.x) && (dragTopLeft.y <= h && h <= dragBottomRight.y);

		Tile tile = (use_preview ? rect_preview[h][w] : target[h][w]);
		if (tile.texture_id == -1) continue;

		float rend_size = tile_pixel_size * view_scale;
		SDL_Rect
			src_rect{ tile_pixel_size * tile.id_on_texture.x, tile_pixel_size * tile.id_on_texture.y, tile_pixel_size, tile_pixel_size },
			target_rect{ on_screen_origin.x + rend_size * w, on_screen_origin.y + rend_size * h, std::ceil(rend_size), std::ceil(rend_size) };
		SDL_RenderCopy(renderer, ref_textures.at(tile.texture_id).texture, &src_rect, &target_rect);
	}
}

void EditArea::drawToTexture(SDL_Renderer* renderer, SDL_Texture* texture, const std::map<int, Texture>& ref_textures, int view_w, int view_h) {
	selection_width = selection.bottomright.x - selection.topleft.id_on_texture.x + 1;
	selection_height = selection.bottomright.y - selection.topleft.id_on_texture.y + 1;

	// Draw lines
	on_screen_tile_size = view_scale * tile_pixel_size;
	on_screen_origin = camera_pos * -view_scale;

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

	// TopLeft
	int
		render_area_tl_x = std::max(0, (int)(-on_screen_origin.x / on_screen_tile_size)),
		render_area_tl_y = std::max(0, (int)(-on_screen_origin.y / on_screen_tile_size));
	// BottomRight
	int
		render_area_br_x = std::min(tilemap_width, (int)(render_area_tl_x + view_w / (int)on_screen_tile_size) + 1),
		render_area_br_y = std::min(tilemap_height, (int)(render_area_tl_y + view_h / (int)on_screen_tile_size) + 1);

	cho::Vector2i
		render_area_topleft(render_area_tl_x, render_area_tl_y),
		render_area_bottomright(render_area_br_x, render_area_br_y);

	// Render tiles
	for (size_t layer = 0; layer < tilemap.size(); layer++) {
		if (!(*visibles)[layer].visible)
			continue;

		renderTilemap(
			renderer,
			ref_textures,
			render_area_topleft,
			render_area_bottomright,
			tilemap[layer],
			layer == rect_preview_layer
		);
	}

	// Focused tile
	ImVec2 pos = ImGui::GetMousePos();
	pos.x -= (on_screen_origin.x + ImGui::GetWindowPos().x);
	pos.y -= (on_screen_origin.y + ImGui::GetWindowPos().y + 35);  // Menu bar offset
	focused = getTileID(pos.x, pos.y);
	renderFocus(highlight_line_color, renderer);

	SDL_SetRenderTarget(renderer, nullptr);
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
	ImGui::SetWindowSize(ImVec2(window_w * EDIT_WIDTH, (float)window_h), ImGuiCond_Once);
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
	editarea.drawToTexture(renderer, texture, ref_textures, window_w * EDIT_WIDTH, window_h);

	ImGui::Image((void*)texture, ImVec2(size.x, size.y));
	
	/* END */
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Reset view...")) {
				editarea.camera_pos = DEFAULT_CAM_POS;
				editarea.view_scale = DEFAULT_VIEW_SCALE;
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
