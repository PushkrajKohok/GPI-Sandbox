#include "imgui_layer.h"
#include "metrics.h"
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>

bool ImGuiLayer::init(SDL_Window* window, void* /*gl_ctx*/) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  auto& style = ImGui::GetStyle();
  style.WindowRounding = 8.0f;
  style.FrameRounding  = 6.0f;

  if (!ImGui_ImplSDL2_InitForOpenGL(window, nullptr)) return false;
  if (!ImGui_ImplOpenGL3_Init("#version 330")) return false;
  return true;
}

void ImGuiLayer::shutdown() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiLayer::new_frame(SDL_Window* window) {
  (void)window;
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::render() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
