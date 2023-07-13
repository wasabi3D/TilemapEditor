#include "PaletteArea.h"

void PaletteArea::precalculateEssentials() {
	// Precalculate some info
	Camera cm = getCurrentCamera();
	camera_pos = cm.camera_pos;
	view_scale = cm.view_scale;
	cur_texture_ptr = textures[current_texture].texture;
	SDL_QueryTexture(cur_texture_ptr, nullptr, nullptr, &texture_width, &texture_height);
	on_screen_origin = camera_pos * -view_scale;
	on_screen_tile_size = view_scale * tile_pixel_size;
	texture_tile_w = texture_width / tile_pixel_size;
	texture_tile_h = texture_height / tile_pixel_size;

	ImVec2 pos = ImGui::GetMousePos();
	pos.x -= (on_screen_origin.x + ImGui::GetWindowPos().x);
	pos.y -= (on_screen_origin.y + ImGui::GetWindowPos().y + 35 + 27);  // Menu bar offset + Tab bar offset
	focused = getTileID(pos.x, pos.y);
}

void PaletteArea::initialize_selection() {
	cur_texture_ptr = nullptr;
	focused = TileID(-1, -1);
	current_texture = -1;
	selection = TileSelection();
}

bool PaletteArea::isValidFocus() {
	if (current_texture == -1) 
		return false;
	return !(focused.x < 0 || focused.x >= texture_tile_w || focused.y < 0 || focused.y >= texture_tile_h);
}

TileID PaletteArea::getTileID(int mouse_x, int mouse_y) {
	int
		tile_x = std::floor((float)mouse_x / (tile_pixel_size * view_scale)),
		tile_y = std::floor((float)mouse_y / (tile_pixel_size * view_scale));

	return TileID(tile_x, tile_y);
}

void PaletteArea::onLeftClick() {
	if (textures.count(current_texture) == 0) return;

	if (!isValidFocus())
		return;
	selected_texture = current_texture;
	return;
}

void PaletteArea::onStartDrag() {
	if (!isValidFocus())
		return;

	Tile topleft;
	topleft.id_on_texture = focused;
	topleft.texture_id = selected_texture;
	TileID bottomright = focused;

	selection.topleft = topleft;
	selection.bottomright = bottomright;
	dragOrigin = focused;
}

void PaletteArea::onDrag() {
	if (!isValidFocus())
		return;
	if (selection.topleft.id_on_texture.x < 0 || selection.topleft.id_on_texture.y < 0)
		return;
	
	// tl = TopLeft   br = BottomRight
	int
		new_tl_x = std::min(dragOrigin.x, focused.x),
		new_tl_y = std::min(dragOrigin.y, focused.y),
		new_br_x = std::max(dragOrigin.x, focused.x),
		new_br_y = std::max(dragOrigin.y, focused.y);
	selection.topleft.id_on_texture = TileID(new_tl_x, new_tl_y);
	selection.bottomright = TileID(new_br_x, new_br_y);
}

void PaletteArea::processTexture(Texture& texture, bool replace_mode) {
	if (replace_mode) {
		int cur_texture_w, cur_texture_h, new_texture_w, new_texture_h;
		SDL_QueryTexture(textures[current_texture].texture, nullptr, nullptr, &cur_texture_w, &cur_texture_h);
		SDL_QueryTexture(texture.texture, nullptr, nullptr, &new_texture_w, &new_texture_h);

		if (cur_texture_w == new_texture_h && cur_texture_h == new_texture_h) {
			SDL_DestroyTexture(textures[current_texture].texture);
			textures[current_texture] = texture;
		}
		else {
			replace_warning = true;
			replace_new_texture = texture;
			replace_target_id = current_texture;
		}
	}
	else {
		int texture_id = getAvailableID();
		if (texture_id != -1) {
			addTexture(texture_id, texture);
		}
		else {
			std::cout << "No texture slot available!" << std::endl;
			SDL_DestroyTexture(texture.texture);
			texture.texture = nullptr;
		}
	}
}

void PaletteArea::askTexture(SDL_Renderer* renderer, bool replace_mode) {
	if (replace_mode && (deleting_texture || textures.size() == 0))
		return;

	nfdchar_t* outPath = nullptr;
	nfdresult_t result = NFD_OpenDialog("png", nullptr, &outPath);
	if (result == NFD_OKAY) {
		std::cout << "Success: " << outPath << std::endl;
		Texture load;
		auto path = std::filesystem::path(std::string(outPath));
		load.path = path.string();
		load.name = path.filename().string();
		int id = (replace_mode ? current_texture : (int)textures.size());
		load.name = std::to_string(id) + ". " + load.name;
		load.texture = cho::loadTexture(outPath, renderer);
		if (load.texture) {
			processTexture(load, replace_mode);
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

bool PaletteArea::askDeleteTexture(int view_w, int view_h) {
	if (deleting_texture) {
		ImGui::Begin("Delete texture", &deleting_texture, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::SetWindowPos({ (float)view_w / 2, (float)view_h / 2 }, ImGuiCond_Once);
		ImGui::NewLine();
		std::string message = "Do you really want to delete the texture? (" + textures[delete_texture_id].name + ")";
		ImGui::Text(message.c_str());
		ImGui::Text("All tiles using this texture will also be deleted.");
		ImGui::Separator();
		if (ImGui::Button("Yes")) {
			deleting_texture = false;
			ImGui::End();
			return true;
		}

		ImGui::SameLine();
		if (ImGui::Button("No"))
			deleting_texture = false;
		ImGui::NewLine();
		ImGui::End();
	}

	return false;
}

void PaletteArea::deleteTexture() {
	editOnCloseTexture(delete_texture_id);
	SDL_DestroyTexture(textures[delete_texture_id].texture);
	textures.erase(delete_texture_id);
	initialize_selection();
}

void PaletteArea::askReplaceTexture() {
	if (replace_warning) {
		ImGui::Begin("Replace texture", &replace_warning, popup_flags);
		ImGui::NewLine();
		ImGui::Text("Currently selected texture (%s)", textures[replace_target_id].name.c_str());
		ImGui::Text("and newly selected texture don't have the same dimensions.");
		ImGui::Text("Some tiles might be lost but do you want to replace anyway? ");

		ImGui::Separator();
		
		if (ImGui::Button("Yes")) {
			int new_texture_w, new_texture_h;
			SDL_QueryTexture(replace_new_texture.texture, nullptr, nullptr, &new_texture_w, &new_texture_h);
			editOnReplaceRemoveTiles(replace_target_id, new_texture_w / tile_pixel_size, new_texture_h / tile_pixel_size);
			SDL_DestroyTexture(textures[replace_target_id].texture);
			textures[replace_target_id] = replace_new_texture;
			replace_warning = false;
			initialize_selection();
		}
		if (ImGui::Button("No"))
			replace_warning = false;

		ImGui::NewLine();
		ImGui::End();
	}
}

void PaletteArea::drawFocused(SDL_Color color, SDL_Renderer* renderer) 
{
	if (!isValidFocus())
		return;
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

void PaletteArea::drawSelection(SDL_Color color, SDL_Renderer* renderer) {
	if (!isValidSelection(selection))
		return;
	TileID
		topleft = selection.topleft.id_on_texture,
		bottomright = selection.bottomright;

	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	int
		topleft_x = topleft.x * tile_pixel_size * view_scale + on_screen_origin.x,
		topleft_y = topleft.y * tile_pixel_size * view_scale + on_screen_origin.y;
	int
		dim_x = tile_pixel_size * view_scale * (bottomright.x - topleft.x + 1),
		dim_y = tile_pixel_size * view_scale * (bottomright.y - topleft.y + 1);

	SDL_RenderDrawLine(renderer, topleft_x, topleft_y, topleft_x + dim_x, topleft_y);
	SDL_RenderDrawLine(renderer, topleft_x, topleft_y + dim_y, topleft_x + dim_x, topleft_y + dim_y);
	SDL_RenderDrawLine(renderer, topleft_x, topleft_y, topleft_x, topleft_y + dim_y);
	SDL_RenderDrawLine(renderer, topleft_x + dim_x, topleft_y, topleft_x + dim_x, topleft_y + dim_y);
}

void PaletteArea::drawCurrent(SDL_Renderer* renderer, SDL_Texture* texture, int texture_id) {
	precalculateEssentials();
	int
		on_screen_w = texture_width * view_scale,
		on_screen_h = texture_height * view_scale;

	SDL_Rect dst_rect{ on_screen_origin.x, on_screen_origin.y, on_screen_w, on_screen_h };
	SDL_RenderCopy(renderer, cur_texture_ptr, nullptr, &dst_rect);

	SDL_SetRenderDrawColor(renderer, line_color.r, line_color.g, line_color.b, line_color.a);
	// Vertical lines
	for (int i = 0; i < texture_tile_w + 1; i++) {
		int
			x1 = i * on_screen_tile_size + on_screen_origin.x,
			y1 = on_screen_origin.y,
			x2 = x1,
			y2 = y1 + texture_tile_h * on_screen_tile_size;
		SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	}
	// Horizontal lines
	for (int i = 0; i < texture_tile_h + 1; i++) {
		int
			x1 = on_screen_origin.x,
			y1 = i * on_screen_tile_size + on_screen_origin.y,
			x2 = x1 + texture_tile_w * on_screen_tile_size,
			y2 = y1;
		SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	}

	// Focused tile
	drawFocused(highlight_line_color, renderer);

	if (selected_texture == current_texture) {
		drawSelection(selected_color, renderer);
	}
}

void PaletteArea::drawToTexture(SDL_Renderer* renderer, SDL_Texture* texture,int view_w, int view_h) {
	SDL_SetRenderDrawColor(renderer, clear_color.r, clear_color.g, clear_color.b, clear_color.a);
	SDL_RenderClear(renderer);

	/* TAB BAR */
	ImGuiTabBarFlags tb_flags = ImGuiTabBarFlags_NoCloseWithMiddleMouseButton;
	if (ImGui::BeginTabBar("palette", tb_flags)) {
		for (const std::pair<int, Texture>& texture_obj : textures) {
			bool texture_open{ true };
			if (ImGui::BeginTabItem(texture_obj.second.name.c_str(), &texture_open)) {
				current_texture = texture_obj.first;
				drawCurrent(renderer, texture, current_texture);
				ImGui::EndTabItem();
			}

			if (!texture_open && !replace_warning) {
				delete_texture_id = texture_obj.first;
				deleting_texture = true;
			}

		}
	ImGui::EndTabBar();
	}

	if (askDeleteTexture(view_w, view_h))
		deleteTexture();

	askReplaceTexture();
}

int PaletteArea::getAvailableID() {
	for (int i = 0; i < max_texture; i++) {
		if (textures.count(i) == 0) {
			return i;
		}
	}

	return -1;
}

void PaletteArea::destroy() {
	for (auto& p : textures) {
		SDL_DestroyTexture(p.second.texture);
	}
}

std::map<int, TextureData> PaletteArea::saveTextureToTMX(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* map_elm_ptr) {
	std::map<int, TextureData> out;
	int current_first_tile_id = 1;
	for (auto texture : textures) {
		tinyxml2::XMLElement* tileset = doc.NewElement("tileset");
		tileset->SetAttribute("firstgid", current_first_tile_id);
		tileset->SetAttribute("name", texture.second.name.c_str());
		tileset->SetAttribute("tilewidth", tile_pixel_size);
		tileset->SetAttribute("tileheight", tile_pixel_size);
		
		int texture_w_px, texture_h_px;
		SDL_QueryTexture(texture.second.texture, nullptr, nullptr, &texture_w_px, &texture_h_px);
		int
			tile_w = texture_w_px / tile_pixel_size,
			tile_h = texture_h_px / tile_pixel_size;
		int tile_count = tile_w * tile_h;

		tileset->SetAttribute("tilecount", tile_count);
		tileset->SetAttribute("columns", tile_w);
		tileset->SetText("\n");

		tinyxml2::XMLElement* image = doc.NewElement("image");
		image->SetAttribute("source", texture.second.path.c_str());
		image->SetAttribute("width", texture_w_px);
		image->SetAttribute("height", texture_h_px);
		tileset->InsertEndChild(image);

		map_elm_ptr->InsertEndChild(tileset);

		out[texture.first] = 
		{
			.first_tile_id = current_first_tile_id,
			.texture_tile_width = tile_w,
			.texture_tile_height = tile_h 
		};
		current_first_tile_id += tile_count;
	}

	return out;
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
	ImGui::SetWindowPos(ImVec2(window_w * EDIT_WIDTH, 0), ImGuiCond_Once);
	ImGui::SetWindowSize(ImVec2(window_w * PALETTE_WIDTH, (float)window_h), ImGuiCond_Once);
	ImVec2 size = ImGui::GetWindowSize();
	ImVec2 pos = ImGui::GetWindowPos();
	if (!ImGui::IsItemHovered() && isCursorInsideWindow(ImGui::GetMousePos(), pos, size))
		focusflag = FOCUSED_PALETTE;

	/* MENU BAR */
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("Texture")) {
			if (ImGui::MenuItem("Load new texture...")) {
				palette_area.askTexture(renderer, false);
			}
			if (ImGui::MenuItem("Replace texture...")) {
				palette_area.askTexture(renderer, true);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Reset view...")) {
				palette_area.setCameraPosition(DEFAULT_CAM_POS);
				palette_area.setViewScale(DEFAULT_VIEW_SCALE);
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
	palette_area.drawToTexture(renderer, texture, window_w * PALETTE_WIDTH, window_h);

	ImGui::Image((void*)texture, ImVec2(size.x, size.y));

	/* END */
	ImGui::End();
	ImGui::PopStyleVar();
	SDL_SetRenderTarget(renderer, nullptr);
	return texture;
}