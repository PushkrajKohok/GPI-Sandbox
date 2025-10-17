#pragma once
#include <string>

struct HostFont {
  void* imgui_font = nullptr;  // ImFont*
  float ascent=0, descent=0, line_gap=0, atlas_px_height=0;
};

bool fontatlas_init_default(HostFont& out, float pixel_height = 18.0f);
