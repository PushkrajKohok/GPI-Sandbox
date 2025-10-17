#include "hud_perf.h"
#include <imgui.h>

static void spark(const std::vector<double>& s, float h=36.0f) {
  if (s.empty()) { ImGui::TextDisabled("no data"); return; }
  ImGui::PlotLines("##spark", [](void* d,int i)->float{
    auto* v = (std::vector<double>*)d; return (float)(*v)[i];
  }, (void*)&s, (int)s.size(), 0, nullptr, 0.0f, 24.0f, ImVec2(180,h));
}

void PerCallHud::draw_small() {
  ImGui::SetNextWindowBgAlpha(0.90f);
  ImGui::Begin("Calls", nullptr,
    ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|
    ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoNav);
  auto us = upd.stats(), rs = ren.stats();
  ImGui::Text("Update  avg %.2f  p95 %.2f  p99 %.2f  last %.2f", us.avg, us.p95, us.p99, us.last);
  spark(upd.samples());
  ImGui::Separator();
  ImGui::Text("Render  avg %.2f  p95 %.2f  p99 %.2f  last %.2f", rs.avg, rs.p95, rs.p99, rs.last);
  spark(ren.samples());
  ImGui::End();
}
