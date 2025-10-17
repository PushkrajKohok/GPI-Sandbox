#include "draw_prim.h"
#include <imgui.h>

static ImU32 to_col(unsigned int rgba) {
  unsigned r = (rgba & 0x000000FFu);
  unsigned g = (rgba & 0x0000FF00u) >> 8;
  unsigned b = (rgba & 0x00FF0000u) >> 16;
  unsigned a = (rgba & 0xFF000000u) >> 24;
  return IM_COL32(r,g,b,a);
}

void draw2d::draw_rects(const GPI_DrawRect* r, int count) {
  if (!r || count<=0) return;
  if (ImGui::GetCurrentContext() == nullptr) return; // Safety check
  ImDrawList* dl = ImGui::GetBackgroundDrawList();
  for (int i=0;i<count;++i) {
    ImVec2 p1(r[i].x, r[i].y);
    ImVec2 p2(r[i].x + r[i].w, r[i].y + r[i].h);
    dl->AddRectFilled(p1, p2, to_col(r[i].rgba), 4.0f);
  }
}
