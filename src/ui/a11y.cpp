#include "a11y.h"
#include <imgui.h>

namespace a11y {

A11ySettings g_a11y;

void init() {
  // TODO: Load from config file
  g_a11y.high_contrast = false;
  g_a11y.large_text = false;
  g_a11y.screen_reader = false;
  g_a11y.ui_scale = 1.0f;
}

void apply_to_imgui() {
  ImGuiStyle& style = ImGui::GetStyle();
  ImGuiIO& io = ImGui::GetIO();
  
  // Apply UI scale
  style.ScaleAllSizes(g_a11y.ui_scale);
  io.FontGlobalScale = g_a11y.ui_scale;
  
  // High contrast mode
  if (g_a11y.high_contrast) {
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
  }
  
  // Large text mode
  if (g_a11y.large_text) {
    io.FontGlobalScale *= 1.5f;
  }
}

} // namespace a11y

