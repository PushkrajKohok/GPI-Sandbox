#include "render_drawlist.h"
#include "font_atlas.h"
#include <imgui.h>

static ImU32 to_col(unsigned rgba){
  unsigned r= rgba & 0xFFu, g=(rgba>>8)&0xFFu, b=(rgba>>16)&0xFFu, a=(rgba>>24)&0xFFu;
  return IM_COL32(r,g,b,a);
}

void render_drawlist_v1(const GPI_DrawListV1* dl){
  if (!dl || dl->magic!=GPI_DL_MAGIC) return;
  if (ImGui::GetCurrentContext() == nullptr) return; // Safety check
  ImDrawList* bg = ImGui::GetBackgroundDrawList();
  const uint32_t n = (dl->quad_count <= dl->max_quads) ? dl->quad_count : dl->max_quads;
  for (uint32_t i=0;i<n;++i){
    const auto& q = dl->quads[i];
    bg->AddRectFilled(ImVec2(q.x, q.y), ImVec2(q.x+q.w, q.y+q.h), to_col(q.rgba), 4.0f);
  }
}

void render_drawlist_v15(const GPI_DrawListV15* dl, const HostFont& font) {
  if (!dl || dl->magic != GPI_DL15_MAGIC) return;
  if (ImGui::GetCurrentContext() == nullptr) return; // Safety check
  ImDrawList* bg = ImGui::GetBackgroundDrawList();

  /* --- quads --- */
  const uint32_t nq = (dl->quad_count <= dl->max_quads) ? dl->quad_count : dl->max_quads;
  auto* quads = (const GPI_QuadV1*)((const uint8_t*)dl + sizeof(GPI_DrawListV15));
  for (uint32_t i=0; i<nq; ++i) {
    const auto& q = quads[i];
    bg->AddRectFilled(ImVec2(q.x, q.y), ImVec2(q.x+q.w, q.y+q.h), to_col(q.rgba), 4.0f);
  }

  /* --- text --- */
  auto* runs = (const GPI_TextRunV15*)((const uint8_t*)quads + dl->max_quads*sizeof(GPI_QuadV1));
  auto* utf8 = (const char*)((const uint8_t*)runs + dl->max_text*sizeof(GPI_TextRunV15));

  ImFont* ifont = (ImFont*)font.imgui_font;
  if (!ifont) return;

  const uint32_t nt = (dl->text_count <= dl->max_text) ? dl->text_count : dl->max_text;

  for (uint32_t i=0; i<nt; ++i) {
    const auto& tr = runs[i];
    if (tr.utf8_off + tr.utf8_len > dl->utf8_capacity) continue;
    ImVec2 pos(tr.x, tr.y);

    // alignment: pre-measure width in pixels
    const char* s = utf8 + tr.utf8_off;
    const char* e = s + tr.utf8_len;
    ImVec2 sz = ifont->CalcTextSizeA(tr.size_px, FLT_MAX, 0.0f, s, e);

    if (tr.align == GPI_TALIGN_CENTER) pos.x -= sz.x * 0.5f;
    else if (tr.align == GPI_TALIGN_RIGHT) pos.x -= sz.x;

    bg->AddText(ifont, tr.size_px, pos, to_col(tr.rgba), s, e);
  }
}
