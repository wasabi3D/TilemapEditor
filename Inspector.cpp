#include "Inspector.h"

void InspectorArea::drawToTexture() {
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
	if (ImGui::Button("Add layer")) {
		addNewLayer();
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete")) {
		deleteLayer();
	}
	ImGui::SameLine();
	if (ImGui::Button("Rename")) {
		renaming = true;
	}

	/* Move up/down */
	if (ImGui::Button("Move up")) {
		if (swap(selected, selected + 1))
			selected++;
	}
	ImGui::SameLine();
	if (ImGui::Button("Move down")) {
		if (swap(selected, selected - 1))
			selected--;
	}
	ImGui::Text("Selected: %d", selected);

	if (ImGui::Button("Toggle visibility")) {
		visible_layers[selected] = { .visible = !visible_layers[selected].visible };
	}

	/* Renaming window */
	if (renaming) {
		ImGui::Begin("Rename", &renaming, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::SetWindowPos(ImVec2(600, 500), ImGuiCond_Once);
		static char new_name[32] = ""; ImGui::InputText("New name", new_name, 32);
		if (ImGui::Button("Confirm")) {
			layer_names[selected] = std::string(new_name);
			renaming = false;
		}
		ImGui::End();
	}
	
}

void InspectorArea::addNewLayer() {
	on_add_layer();
	layer_names.push_back("Layer " + std::to_string(layer_names.size()));
}

void InspectorArea::deleteLayer() {
	if (layer_names.size() < 2) return;
	on_delete_layer();
	layer_names.erase(layer_names.begin() + selected);

	visible_layers[selected].visible = true;
	selected = 0;
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
	ImGui::SetWindowPos(ImVec2(1200, 0), ImGuiCond_Once);
	ImGui::SetWindowSize(ImVec2(400, (float)window_h), ImGuiCond_Once);
	ImVec2 size = ImGui::GetWindowSize();

	SDL_Texture* texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_TARGET, (int)size.x, (int)size.y);
	if (!texture) {
		std::cerr << "Failed to create edit texture" << std::endl;
		return nullptr;
	}

	/* ACTUAL RENDERING */
	SDL_SetRenderTarget(renderer, texture);
	inspector.drawToTexture();
	ImGui::Image((void*)texture, ImVec2(size.x, size.y));
	ImGui::End();

	SDL_SetRenderTarget(renderer, nullptr);

	return texture;
}