#pragma once
#include <cstdint>

struct InputSnapshot {
  bool quit = false;
  bool key_escape = false;
  bool toggle_hud = false;

  // Mouse
  int mouse_x = 0, mouse_y = 0;
  bool mouse_left = false;
  bool mouse_right = false;

  // Gamepad summary (extend later)
  bool gamepad_connected = false;
  float axis_left_x = 0.0f;
  float axis_left_y = 0.0f;
  bool button_a = false;
};
