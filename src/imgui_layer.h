#pragma once
#include <SDL.h>

struct ImGuiConfig {
  bool show_hud = true;
};

class ImGuiLayer {
public:
  bool init(SDL_Window* window, void* gl_ctx);
  void shutdown();
  void new_frame(SDL_Window* window);
  void render();
  ImGuiConfig& cfg() { return cfg_; }

private:
  ImGuiConfig cfg_{};
};
