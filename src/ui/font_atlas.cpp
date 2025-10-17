#include "font_atlas.h"
#include <imgui.h>

bool fontatlas_init_default(HostFont& out, float pixel_height) {
  if (ImGui::GetCurrentContext() == nullptr) return false; // Safety check
  ImGuiIO& io = ImGui::GetIO();
  ImFontConfig cfg; cfg.FontDataOwnedByAtlas = false;
  ImFont* f = io.Fonts->AddFontDefault();
  if (!f) return false;
  io.Fonts->Build();
  out.imgui_font = f;
  out.ascent = f->Ascent; out.descent = f->Descent;
  out.line_gap = 0.0f;
  out.atlas_px_height = f->FontSize;
  return true;
}
