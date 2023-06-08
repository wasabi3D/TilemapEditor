#include "chomusuke/infrastructure.h"
#include "EditArea.h"
#include "PaletteArea.h"
#include "Inspector.h"
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <memory>
#undef main

constexpr int window_w = 1600;
constexpr int window_h = 800;

int main();

class Application : public cho::Game {
	ImGuiIO* io{ nullptr };
	SDL_Texture* edit_area_rend{ nullptr };
	SDL_Texture* select_area_rend{ nullptr };
	SDL_Texture* inspector_area_rend{ nullptr };
	std::unique_ptr<EditArea> edit_area;
	std::unique_ptr<PaletteArea> palette_area;
	std::unique_ptr<InspectorArea> inspector_area;
	bool middle_click = false;
	bool left_click = false;
	bool right_click = false;
	int focused_window = -1;
	cho::Vector2f motion{};
	float mouse_wheel_motion = 0;
	const float wheel_speed = 0.5f;
	bool creating_new = true;

	void init_viewport(int tile_size, int map_w, int map_h) {
		if (edit_area != nullptr) {
			edit_area.reset();
		}
		if (palette_area != nullptr) {
			palette_area->destroy();
			palette_area.reset();
		}
		if (inspector_area != nullptr) {
			inspector_area.reset();
		}

		edit_area = std::make_unique<EditArea>(tile_size, map_w, map_h);
		edit_area->clear_color = { 20, 20, 20, 255 };
		edit_area->line_color = { 50, 50, 50, 255 };
		edit_area->camera_pos = { -1, 0 };

		palette_area = std::make_unique<PaletteArea>(tile_size);
		palette_area->clear_color = { 20, 20, 20, 255 };
		palette_area->line_color = { 50, 50, 50, 255 };
		palette_area->setCameraPosition({ -1, 0 });

		inspector_area = std::make_unique<InspectorArea>();
		inspector_area->on_add_layer = [this]() {this->edit_area->onAddLayer(); };
		inspector_area->on_delete_layer = [this]() {this->edit_area->onDeleteLayer(); };
		inspector_area->on_swap = [this](int a, int b) {this->edit_area->onSwap(a, b);  };
		inspector_area->addNewLayer();
	}

	void start() override {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		io = &ImGui::GetIO(); 
		io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		//ImGui::StyleColorsDark();
		ImGui::StyleColorsClassic();

		// Setup Platform/Renderer backends
		ImGui_ImplSDL2_InitForSDLRenderer(window, win_renderer);
		ImGui_ImplSDLRenderer2_Init(win_renderer);
		io->ConfigWindowsMoveFromTitleBarOnly = true;
	}
	
	void processEvent(const SDL_Event& event) override {
		ImGui_ImplSDL2_ProcessEvent(&event);

		if (event.type == SDL_MOUSEMOTION) {
			motion.x = event.motion.xrel;
			motion.y = event.motion.yrel;
		}
		if (event.type == SDL_MOUSEBUTTONDOWN) {
			if(event.button.button == SDL_BUTTON_MIDDLE)
				middle_click = true;
			if (event.button.button == SDL_BUTTON_LEFT)
				left_click = true;
			if (event.button.button == SDL_BUTTON_RIGHT)
				right_click = true;
		}
		if (event.type == SDL_MOUSEBUTTONUP) {
			if (event.button.button == SDL_BUTTON_MIDDLE)
				middle_click = false;
			if(event.button.button == SDL_BUTTON_LEFT)
				left_click = false;
			if (event.button.button == SDL_BUTTON_RIGHT)
				right_click = false;
		}
		if (event.type == SDL_MOUSEWHEEL) {
			mouse_wheel_motion = event.wheel.y;
		}
	}
	
	void update() override {
		ImGui_ImplSDLRenderer2_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		/*
		ImGui::Begin("Performance");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
		ImGui::End();
		*/

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New...")) {
					creating_new = true;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		if (creating_new) {
			ImGui::Begin("Create a new tilemap", &creating_new, ImGuiWindowFlags_AlwaysAutoResize);
			static char tile_dim[10] = ""; ImGui::InputText("Tile dimension(in pixels)", tile_dim, 10, ImGuiInputTextFlags_CharsDecimal);
			static char width[10] = ""; ImGui::InputText("Width", width, 10, ImGuiInputTextFlags_CharsDecimal);
			static char height[10] = ""; ImGui::InputText("Height", height, 10, ImGuiInputTextFlags_CharsDecimal);

			if (ImGui::Button("Create")) {
				try {
					std::string s_width(width), s_height(height), s_tile_dim(tile_dim);
					int i_width = std::stoi(s_width), i_height = std::stoi(s_height), i_tile_dim = std::stoi(s_tile_dim);
					init_viewport(i_tile_dim, i_width, i_height);
					creating_new = false;
				}
				catch (std::invalid_argument ex) {
					std::cout << "Conversion error" << std::endl;;
				}
			}
			ImGui::Text("Warning: Any unsaved data will be lost forever.");
			ImGui::End();
		}

		if (edit_area == nullptr || palette_area == nullptr) return;
		edit_area->selected_layer = inspector_area->selected;
		edit_area_rend = draw_edit_area_texture(win_renderer, SDL_PIXELFORMAT_RGBA8888, window_w, window_h, *edit_area, focused_window, palette_area->getTextures(), inspector_area->visible_layers);
		select_area_rend = draw_palette_area_texture(win_renderer, SDL_PIXELFORMAT_RGBA8888, window_w, window_h, *palette_area, focused_window);
		inspector_area_rend = draw_inspector_area_texture(win_renderer, SDL_PIXELFORMAT_RGBA8888, window_w, window_h, *inspector_area);

		if (left_click) {
			if (focused_window == FOCUSED_EDIT) {
				edit_area->onLeftClick(palette_area->getSelectedTile());
			}
			if (focused_window == FOCUSED_PALETTE) {
				palette_area->onLeftClick();
			}
		}

		if (middle_click) {
			if (focused_window == FOCUSED_EDIT) {
				cho::Vector2f final_motion = motion * (-1 / edit_area->view_scale);
				edit_area->camera_pos = edit_area->camera_pos + final_motion;
			}
			if (focused_window == FOCUSED_PALETTE) {
				Camera camera = palette_area->getCurrentCamera();
				cho::Vector2f final_motion = motion * (-1 / camera.view_scale);
				palette_area->setCameraPosition(camera.camera_pos + final_motion);
			}
		}

		if (right_click) {
			if (focused_window == FOCUSED_EDIT) {
				edit_area->onLeftClick(Tile());
			}
		}

		float scale_multiplier = 1.0f - (mouse_wheel_motion * delta * wheel_speed);
		if (focused_window == FOCUSED_EDIT)
			edit_area->view_scale *= scale_multiplier;
		if (focused_window == FOCUSED_PALETTE)
			palette_area->setViewScale(palette_area->getCurrentCamera().view_scale * scale_multiplier);

		mouse_wheel_motion = 0;
		focused_window = -1;
	}

	void draw() override {
		ImGui::Render();
		SDL_RenderSetScale(win_renderer, io->DisplayFramebufferScale.x, io->DisplayFramebufferScale.y);
		SDL_SetRenderDrawColor(win_renderer, clear_color.r, clear_color.g, clear_color.b, clear_color.a);
		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());

		SDL_DestroyTexture(edit_area_rend);
		SDL_DestroyTexture(select_area_rend);
		SDL_DestroyTexture(inspector_area_rend);
	}

	void death() override {
		palette_area->destroy();
		ImGui_ImplSDLRenderer2_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}
};


int main() {
	Application app;
	app.init("Tilemap Editor", 100, 100, window_w, window_h, {10, 2, 2, 255}, SDL_WINDOW_SHOWN, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC, SDL_BLENDMODE_BLEND);
	app.mainloop();
	return 0;
}
