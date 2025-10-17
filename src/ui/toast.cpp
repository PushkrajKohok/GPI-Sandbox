#include "toast.h"
#include <imgui.h>

static ImU32 color_for(Toast::Type t) {
  switch (t) {
    case Toast::Info:  return IM_COL32( 80,180,255,230);
    case Toast::Warn:  return IM_COL32(255,190, 50,230);
    case Toast::Error: return IM_COL32(255, 80, 80,230);
  }
  return IM_COL32_WHITE;
}

void push(Toast::Type tp, std::deque<Toast>& q, const std::string& s, float sec) {
  Toast t; t.text = s; t.seconds = sec; t.type = tp; t.start = std::chrono::steady_clock::now();
  q.push_back(std::move(t));
}

void Toasts::info(const std::string& s, float sec)  { push(Toast::Info,  q_, s, sec); }
void Toasts::warn(const std::string& s, float sec)  { push(Toast::Warn,  q_, s, sec); }
void Toasts::error(const std::string& s, float sec) { push(Toast::Error, q_, s, sec); }

void Toasts::render() {
  if (q_.empty()) return;
  // Check if ImGui context is available (not in headless mode)
  if (!ImGui::GetCurrentContext()) return;
  const float pad = 10.0f;
  const float y0  = 20.0f;
  float y = y0;

  auto now = std::chrono::steady_clock::now();
  for (size_t i = 0; i < q_.size();) {
    auto& t = q_[i];
    float age = std::chrono::duration<float>(now - t.start).count();
    float alpha = 1.0f - std::min(1.0f, std::max(0.0f, (age - (t.seconds - 0.4f)) / 0.4f));
    if (age > t.seconds) { q_.erase(q_.begin()+i); continue; }

    ImGui::SetNextWindowBgAlpha(0.85f * alpha);
    ImGui::SetNextWindowPos(ImVec2(pad, y), ImGuiCond_Always);
    ImGui::Begin(("toast##"+std::to_string((long long)(&t))).c_str(), nullptr,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
      ImGuiWindowFlags_NoNav);
    ImGui::PushStyleColor(ImGuiCol_Text, color_for(t.type));
    ImGui::TextUnformatted(t.text.c_str());
    ImGui::PopStyleColor();
    y += ImGui::GetWindowHeight() + 6.0f;
    ImGui::End();
    ++i;
  }
}
