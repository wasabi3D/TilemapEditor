#include "Inspector.h"

static char new_name[32] = "";


void InspectorArea::addNewLayer() {
	on_add_layer();
	layer_names.push_back("Layer " + std::to_string(layer_names.size()));
	selected = layer_names.size() - 1;
}

void InspectorArea::deleteLayer() {
	if (layer_names.size() < 2) return;
	on_delete_layer(deleting_layer);
	layer_names.erase(layer_names.begin() + deleting_layer);

	// Unless the user has selected another layer, bring the selection 1 layer below for natural behaviour
	if(selected == deleting_layer){
		visible_layers[deleting_layer].visible = true;
		selected = (size_t)std::max(0, (int)deleting_layer - 1);
	}

	show_delete_warn = !_tmp_do_not_show_again;
	deleting_layer = -1;
}

void InspectorArea::drawToTexture(int view_w, int view_h) {
	ImGui::Text("Layers");
	ImGui::Separator();

	ImGui::BeginChild("layers child", ImVec2(375, 200), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
	ImVec2 pos(200, 0);
	ImGuiSelectableFlags flags = ImGuiSelectableFlags_SpanAllColumns;
	for (int i = layer_names.size() - 1; i >= 0; i--) {
		std::string name = layer_names[i] + (visible_layers[i].visible ? "" : " (Hidden)");
		if (ImGui::Selectable(name.c_str(), selected == i, flags))
			selected = i;
	}
	ImGui::EndChild();

	/* Layer operations */
	if (ImGui::Button("Add layer") && deleting_layer == -1) {
		addNewLayer();
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete")) {
		deleting_layer = selected;
	}
	ImGui::SameLine();
	if (ImGui::Button("Rename")) {
		renaming = true;
		layer_names[selected].copy(new_name, layer_names[selected].size() + 1);
	}

	/* Move up/down */
	if (ImGui::Button("Move up") && deleting_layer == -1) {
		if (swap(selected, selected + 1))
			selected++;
	}
	ImGui::SameLine();
	if (ImGui::Button("Move down") && deleting_layer == -1) {
		if (swap(selected, selected - 1))
			selected--;
	}

	if (ImGui::Button("Toggle visibility")) {
		visible_layers[selected] = { .visible = !visible_layers[selected].visible };
	}

	/* Renaming window */
	if (renaming) {
		ImGui::Begin("Rename", &renaming, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::SetWindowPos(ImVec2(view_w / 2, view_h / 2), ImGuiCond_Once);
		ImGui::InputText("New name", new_name, 32);
		if (ImGui::Button("Confirm")) {
			layer_names[selected] = std::string(new_name);
			renaming = false;
		}
		ImGui::End();
	}
	
	/* Confirm delete layer */
	if (deleting_layer != -1) {
		if (show_delete_warn) {
			ImGui::Begin("Delete layer?", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
			ImGui::SetWindowPos(ImVec2(view_w / 2, view_h / 2), ImGuiCond_Once);
			std::string msg = "Do you really want to delete the \"" + layer_names[deleting_layer] + "\" layer?";
			ImGui::Text(msg.c_str());
			if (ImGui::Button("Yes")) {
				deleteLayer();
			}
			ImGui::SameLine();
			if (ImGui::Button("No"))
				deleting_layer = -1;
			ImGui::Text("Warning: You will NOT be able to undo this change so be careful!");
			ImGui::NewLine();
			ImGui::Separator();
			ImGui::Checkbox("Do not show this warning again in this session (restart to reset)", &_tmp_do_not_show_again);
			ImGui::End();
		}
		else {
			deleteLayer();
		}
	}

	/* Brush settings */
	ImGui::NewLine();
	ImGui::NewLine();
	ImGui::Text("Brush");
	ImGui::Separator();
	ImGui::RadioButton("Basic", &selected_brush, BRUSH_BASIC); ImGui::SameLine();
	ImGui::RadioButton("Rectangle", &selected_brush, BRUSH_RECTANGLE);
	ImGui::Text("* Left click to draw, Right click to erase");

	/* misc */
	ImGui::NewLine();
	ImGui::NewLine();
	ImGui::Text("Miscellaneous");
	ImGui::Separator();
	ImGui::Checkbox("Show application framerate", &show_framerate);
	if (show_framerate)
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
}

bool InspectorArea::swap(int a, int b) {
	if (a < 0 || a >= layer_names.size() || b < 0 || b >= layer_names.size()) 
		return false;
	on_swap(a, b);
	std::swap(layer_names.at(a), layer_names.at(b));
	return true;
}

SDL_Texture* draw_inspector_area_texture(
	SDL_Renderer* renderer,
	Uint32 format,
	int window_w,
	int window_h,
	InspectorArea& inspector
	) 
{
	Uint32 window_flags = ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
	ImGui::Begin("Inspector", (bool*)0, window_flags);
	float view_w = window_w * INSPECTOR_WIDTH;
	ImGui::SetWindowPos(ImVec2(window_w * (EDIT_WIDTH + PALETTE_WIDTH), 0), ImGuiCond_Once);
	ImGui::SetWindowSize(ImVec2(view_w, (float)window_h), ImGuiCond_Once);
	ImVec2 size = ImGui::GetWindowSize();

	SDL_Texture* texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_TARGET, (int)size.x, (int)size.y);
	if (!texture) {
		std::cerr << "Failed to create edit texture" << std::endl;
		return nullptr;
	}

	/* ACTUAL RENDERING */
	SDL_SetRenderTarget(renderer, texture);
	inspector.drawToTexture(view_w, window_h);
	ImGui::Image((void*)texture, ImVec2(size.x, size.y));
	ImGui::End();

	SDL_SetRenderTarget(renderer, nullptr);

	return texture;
}