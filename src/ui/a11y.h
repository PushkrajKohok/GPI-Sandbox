#pragma once
#include <string>

namespace a11y {

// Accessibility settings
struct A11ySettings {
  bool high_contrast = false;
  bool large_text = false;
  bool screen_reader = false;
  float ui_scale = 1.0f;
};

// Global accessibility state
extern A11ySettings g_a11y;

// Initialize from config
void init();

// Apply settings to ImGui
void apply_to_imgui();

} // namespace a11y

