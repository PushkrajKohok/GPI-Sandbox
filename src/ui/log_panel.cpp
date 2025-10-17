#include "log_panel.h"
#include <imgui.h>
#include <deque>

static bool pass(int filter, LogLvl lvl) {
  if (filter==0) return true;
  if (filter==1) return lvl==LogLvl::Info;
  if (filter==2) return lvl==LogLvl::Warn;
  return lvl==LogLvl::Error;
}

void LogPanel::draw(LogBus& bus) {
  ImGui::SetNextWindowSize({560,320}, ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Log")) {
    ImGui::TextUnformatted("Filter:");
    ImGui::SameLine();
    ImGui::RadioButton("All",   &filter_, 0); ImGui::SameLine();
    ImGui::RadioButton("Info",  &filter_, 1); ImGui::SameLine();
    ImGui::RadioButton("Warn",  &filter_, 2); ImGui::SameLine();
    ImGui::RadioButton("Error", &filter_, 3);

    ImGui::Separator();
    std::deque<LogMsg> snap; bus.snapshot(snap, 1000);
    ImGui::BeginChild("logscroll", ImVec2(0,0), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (auto& m : snap) {
      if (!pass(filter_, m.lvl)) continue;
      ImVec4 col = (m.lvl==LogLvl::Info) ? ImVec4(0.7f,0.9f,1,1):
                  (m.lvl==LogLvl::Warn) ? ImVec4(1,0.8f,0.4f,1):
                                          ImVec4(1,0.4f,0.4f,1);
      ImGui::PushStyleColor(ImGuiCol_Text, col);
      ImGui::TextUnformatted(m.text.c_str());
      ImGui::PopStyleColor();
    }
    ImGui::EndChild();
  }
  ImGui::End();
}
