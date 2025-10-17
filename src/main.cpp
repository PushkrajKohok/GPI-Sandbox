#include <SDL.h>
#include <OpenGL/gl.h>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <memory>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>
#include "metrics.h"
#include "input.h"
#include "imgui_layer.h"
#include "runtime/plugin_runtime.h"
#include "platform/fs.h"
#include "config.h"
#include "ui/toast.h"
#include "runtime/watchdog.h"
#include "services/save_store.h"
#include "services/log_bus.h"
#include "services/telemetry.h"
#include "services/artifacts.h"
#include "services/replay.h"
#include "runtime/runner.h"
#include "ui/log_panel.h"
#include "ui/draw_prim.h"
#include "ui/store_panel.h"
#include "ui/hud_perf.h"
#include "ui/render_drawlist.h"
#include "ui/font_atlas.h"
#include "services/screenshot.h"
#include "runtime/drawlist_shm.h"
// #include "qa/golden.h"
// #include "qa/diff.h"
#include "version.h"
#include <ctime>
#include <random>
// #include <filesystem>

namespace {
struct AppConfig {
  const char* title = "GPI Sandbox — Phase 10";
  int width = 1280;
  int height = 720;
  int target_fps = 60;
  bool fixed_timestep = true;
};

struct Cli {
  bool headless=false;
  int  frames=0;
  std::string plugin;
  std::string record_path;
  std::string replay_path;
  bool demo=false;
  std::string win_size;
  std::string golden_capture;
  std::string golden_verify;
};

static Cli parse_cli(int argc, char** argv) {
  Cli c{};
  for (int i=1;i<argc;i++){
    std::string a=argv[i];
    auto next = [&](int& i){ return (i+1<argc)? argv[++i] : (char*)""; };
    if (a=="--version") {
      std::printf("GPI Sandbox %s\n", GPI_VERSION_STR);
      exit(0);
    }
    else if (a=="--headless") c.headless=true;
    else if (a=="--frames") c.frames = std::atoi(next(i));
    else if (a=="--plugin") c.plugin = next(i);
    else if (a=="--record") c.record_path = next(i);
    else if (a=="--replay") c.replay_path = next(i);
    else if (a=="--demo") c.demo = true;
    else if (a=="--win-size") c.win_size = next(i);
    else if (a=="--golden-capture") c.golden_capture = next(i);
    else if (a=="--golden-verify") c.golden_verify = next(i);
  }
  return c;
}

struct HostServices {
  static LogBus* LB;
  static Telemetry* TM;
  static const std::string* PLUGIN_NS;

  static void log_info(const char* msg)  { if (LB) LB->push(LogLvl::Info,  msg?msg:"");  std::fprintf(stdout, "[INFO] %s\n",  msg?msg:""); }
  static void log_warn(const char* msg)  { if (LB) LB->push(LogLvl::Warn,  msg?msg:"");  std::fprintf(stderr, "[WARN] %s\n",  msg?msg:""); }
  static void log_error(const char* msg) { if (LB) LB->push(LogLvl::Error, msg?msg:"");  std::fprintf(stderr, "[ERROR] %s\n", msg?msg:""); }

  static int save_put(const char* key, const void* data, int32_t size) {
    const std::string ns = (PLUGIN_NS && !PLUGIN_NS->empty()) ? *PLUGIN_NS : "unknown";
    return save::put(ns, key, data, size) ? 0 : -1;
  }
  static int save_get(const char* key, void* out, int32_t cap) {
    const std::string ns = (PLUGIN_NS && !PLUGIN_NS->empty()) ? *PLUGIN_NS : "unknown";
    return save::get(ns, key, out, cap);
  }
  static void telemetry_mark(const char* key, double value) {
    if (TM) TM->mark(key?key:"(null)", value);
  }
  static void draw_rects(const GPI_DrawRect* r, int count) {
    draw2d::draw_rects(r, count);
  }
};
LogBus*    HostServices::LB = nullptr;
Telemetry* HostServices::TM = nullptr;
const std::string* HostServices::PLUGIN_NS = nullptr;

// Forward declaration
struct AppState;

static GPI_InputV1 make_input_blob(const InputSnapshot& in) {
  GPI_InputV1 b{};
  b.input_version  = GPI_INPUT_VERSION;
  b.key_escape     = in.key_escape ? 1 : 0;
  b.mouse_x        = in.mouse_x; b.mouse_y = in.mouse_y;
  b.mouse_left     = in.mouse_left ? 1 : 0;
  b.mouse_right    = in.mouse_right ? 1 : 0;
  b.gamepad_connected = in.gamepad_connected ? 1 : 0;
  b.pad_axis_left_x   = (int8_t)(in.axis_left_x * 127.0f);
  b.pad_axis_left_y   = (int8_t)(in.axis_left_y * 127.0f);
  b.pad_button_a      = in.button_a ? 1 : 0;
  return b;
}

struct AppState {
  SDL_Window* window = nullptr;
  SDL_GLContext gl_ctx = nullptr;
  bool running = true;
  bool hud_visible = true;
  AppConfig cfg{};
  FrameHistogram hist{600, 16.6};
  ImGuiLayer imgui;
  PluginRuntime runtime{};
  std::string plugins_dir = "./plugins/bin";
  double deadline_ms_cfg = 12.0;
  bool plugin_loaded = false;
  char plugin_status[128] = "No plugin loaded";
  
  Settings settings;
  Toasts toasts;
  Watchdog watchdog;
  LogBus logs;
  Telemetry telemetry;
  LogPanel log_panel;
  
  std::atomic<bool> app_running{true};
  std::atomic<bool> in_plugin_call{false};
  std::atomic<std::chrono::steady_clock::time_point> plugin_call_start{
    std::chrono::steady_clock::now()
  };
  
  std::vector<std::string> available_plugins;
  int selected_idx = -1;
  std::string current_plugin_leaf;
  bool simulate_stall = false;
  double stall_ms = 200.0;
  
  // Phase 8: Replay/Record, Isolation, Store
  Recorder recorder;
  Replayer replayer;
  std::unique_ptr<IPluginRunner> runner;
  std::vector<PluginMeta> plugin_metas;
  bool show_store = true;
  
  // Phase 9: Per-call metrics, Screenshots
  PerCallHud perf_calls;
  bool record_video = false;
  int video_frame_idx = 0;
  std::string video_dir;
  
  // Phase 10: Demo mode
  bool demo_mode = false;
  double demo_t = 0.0;
  bool demo_swapped = false;
  
  // Phase 12: Draw list
  DrawListMap dl_map;
  GPI_DrawListV1* dl_host = nullptr;
  
  // Phase 13: Draw list v1.5 + font
  GPI_DrawListV15* dl15_host = nullptr;
  HostFont host_font;
};

static GPI_HostApi make_host_api(AppState& s) {
  HostServices::LB = &s.logs;
  HostServices::TM = &s.telemetry;
  HostServices::PLUGIN_NS = &s.current_plugin_leaf;
  GPI_HostApi api{};
  api.log_info  = &HostServices::log_info;
  api.log_warn  = &HostServices::log_warn;
  api.log_error = &HostServices::log_error;
  api.save_put  = &HostServices::save_put;
  api.save_get  = &HostServices::save_get;
  api.telemetry_mark = &HostServices::telemetry_mark;
  api.draw_rects = &HostServices::draw_rects;
  
  // Phase 12: Draw list getter
  static GPI_DrawListV1* DL_PTR = nullptr; 
  static uint32_t DL_BYTES = 0;
  DL_PTR = s.dl_host; DL_BYTES = s.dl_map.bytes;
  api.get_drawlist_v1 = [](GPI_DrawListV1** out, uint32_t* bytes){ *out = DL_PTR; *bytes = DL_BYTES; };
  
  // Phase 13: Draw list v1.5 + font metrics
  static GPI_DrawListV15* DL15_PTR = nullptr;
  static uint32_t DL15_BYTES = 0;
  static HostFont* FONT_PTR = nullptr;
  DL15_PTR = s.dl15_host; DL15_BYTES = 0; // Will be set during allocation
  FONT_PTR = &s.host_font;
  api.get_drawlist_v15 = [](GPI_DrawListV15** out, uint32_t* bytes){ *out = DL15_PTR; *bytes = DL15_BYTES; };
  api.get_font_metrics_v15 = [](float* asc, float* desc, float* gap, float* pxh){
    if (asc) *asc = FONT_PTR->ascent;
    if (desc) *desc = FONT_PTR->descent;
    if (gap) *gap = FONT_PTR->line_gap;
    if (pxh) *pxh = FONT_PTR->atlas_px_height;
  };
  
  return api;
}

static std::string timestamp() {
  std::time_t t = std::time(nullptr);
  char buf[32]; std::strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", std::localtime(&t));
  return buf;
}

static void save_artifacts_now(AppState& s) {
  auto samples = s.hist.samples();
  auto stats   = s.hist.stats_for_summary();
  FrameSummary sum{ stats.avg_ms, stats.p95_ms, stats.p99_ms, stats.dropped_pct, (int)samples.size() };

  SessionInfo info{};
  #if defined(GPI_WIN)
    info.os = "Windows";
  #elif defined(GPI_MAC)
    info.os = "macOS";
  #else
    info.os = "Linux";
  #endif
  info.app_version = "1.0.0";
  info.plugin      = s.current_plugin_leaf;
  info.target_fps  = s.cfg.target_fps;

  std::string ts = timestamp();
  std::string base = "artifacts/session-" + ts;
  (void)std::system("mkdir -p artifacts");

  artifacts::write_frame_csv(base + "-frames.csv", samples);
  artifacts::write_session_json(base + "-summary.json", info, sum);

  s.telemetry.flush_csv(base + "-telemetry.csv");
  s.telemetry.flush_json(base + "-telemetry.json");
  s.toasts.info("Artifacts saved");
}

void sdl_quit() { SDL_Quit(); }

bool init_window(AppState& s, const Cli& cli) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) != 0) {
    std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#if defined(__APPLE__)
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

  // Phase 14: Handle window size override
  // if (!cli.win_size.empty()) {
  //   size_t x_pos = cli.win_size.find('x');
  //   if (x_pos != std::string::npos) {
  //     s.cfg.width = std::stoi(cli.win_size.substr(0, x_pos));
  //     s.cfg.height = std::stoi(cli.win_size.substr(x_pos + 1));
  //   }
  // }
  
  s.window = SDL_CreateWindow(
      s.cfg.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, s.cfg.width, s.cfg.height,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  if (!s.window) {
    std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    return false;
  }

  s.gl_ctx = SDL_GL_CreateContext(s.window);
  if (!s.gl_ctx) {
    std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
    return false;
  }

  // OpenGL context is ready

  SDL_GL_SetSwapInterval(1);

  // Initialize ImGui only in GUI mode
  if (!cli.headless) {
    if (!s.imgui.init(s.window, s.gl_ctx)) {
      std::fprintf(stderr, "ImGui init failed\n");
      return false;
    }
    
    // Phase 13: Initialize font atlas
    if (!fontatlas_init_default(s.host_font, 18.0f)) {
      std::fprintf(stderr, "Font atlas init failed\n");
      return false;
    }
  }
  
  return true;
}

void shutdown(AppState& s) {
  // Only shutdown ImGui if it was initialized
  if (ImGui::GetCurrentContext() != nullptr) {
    s.imgui.shutdown();
  }
  if (s.gl_ctx) { SDL_GL_DeleteContext(s.gl_ctx); s.gl_ctx = nullptr; }
  if (s.window) { SDL_DestroyWindow(s.window); s.window = nullptr; }
  sdl_quit();
}

void poll_input(InputSnapshot& snap, AppState& s) {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    // Only process ImGui events if ImGui is initialized
    if (ImGui::GetCurrentContext() != nullptr) {
      ImGui_ImplSDL2_ProcessEvent(&e);
    }
    if (e.type == SDL_QUIT) snap.quit = true;
    if (e.type == SDL_KEYDOWN) {
      if (e.key.keysym.sym == SDLK_ESCAPE) snap.key_escape = true;
      if (e.key.keysym.sym == SDLK_F10)    snap.toggle_hud = true;
    }
    if (e.type == SDL_MOUSEMOTION) { snap.mouse_x = e.motion.x; snap.mouse_y = e.motion.y; }
    if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
      const bool down = (e.type == SDL_MOUSEBUTTONDOWN);
      if (e.button.button == SDL_BUTTON_LEFT)  snap.mouse_left = down;
      if (e.button.button == SDL_BUTTON_RIGHT) snap.mouse_right = down;
    }
  }
}

void draw_hud(AppState& s, const FrameStats& fs) {
  if (!s.hud_visible) return;

  ImGui::SetNextWindowBgAlpha(0.85f);
  ImGui::Begin("Perf HUD", nullptr,
               ImGuiWindowFlags_NoDecoration |
               ImGuiWindowFlags_AlwaysAutoResize |
               ImGuiWindowFlags_NoSavedSettings |
               ImGuiWindowFlags_NoFocusOnAppearing |
               ImGuiWindowFlags_NoNav);
  ImGui::Text("FPS: %.1f", fs.fps);
  ImGui::Text("avg: %.2f ms  p95: %.2f ms  p99: %.2f ms", fs.avg_ms, fs.p95_ms, fs.p99_ms);
  ImGui::Text("Dropped: %.2f%% (>%0.1f ms)", fs.dropped_pct, s.hist.budget_ms());
      ImGui::Separator();
      if (ImGui::Button("Save Artifacts (F9)")) save_artifacts_now(s);
      ImGui::SameLine();
      if (ImGui::Button("Screenshot (F12)")) {
        std::string ts = timestamp();
        std::string path = "artifacts/snap-" + ts + ".png";
        int w, h; SDL_GetWindowSize(s.window, &w, &h);
        if (save_screenshot_png(path, w, h)) {
          s.toasts.info("Screenshot saved: " + path);
        }
      }
      ImGui::Separator();
      if (ImGui::Button(s.record_video ? "Stop Video" : "Start Video")) {
        if (!s.record_video) {
          s.video_dir = "artifacts/video-" + timestamp();
          s.video_frame_idx = 0;
          s.record_video = true;
          s.toasts.info("Video recording started: " + s.video_dir);
        } else {
          s.record_video = false;
          s.toasts.info("Video recording stopped. Use: ffmpeg -framerate 60 -i " + s.video_dir + "/frame_%06d.png -pix_fmt yuv420p out.mp4");
        }
      }
      ImGui::Separator();
      ImGui::Text("F9: artifacts  |  F12: screenshot  |  F10: toggle HUD  |  Esc: quit");
  ImGui::End();
}

void draw_plugin_panel(AppState& s) {
  if (!s.hud_visible) return;
  ImGui::SetNextWindowBgAlpha(0.90f);
  ImGui::SetNextWindowPos(ImVec2(10, 200), ImGuiCond_FirstUseEver);
  ImGui::Begin("Plugin", nullptr,
    ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|
    ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing|
    ImGuiWindowFlags_NoNav);

  ImGui::Text("Dir: %s", s.plugins_dir.c_str());
  if (ImGui::Button("Refresh")) {
    s.plugin_metas = scan_with_metadata(s.plugins_dir);
    if (s.plugin_metas.empty()) s.selected_idx = -1;
    else if (s.selected_idx < 0 || s.selected_idx >= (int)s.plugin_metas.size()) s.selected_idx = 0;
  }

  if (s.plugin_metas.empty()) {
    ImGui::TextColored(ImVec4(1,0.5f,0.5f,1), "No plugins found.");
  } else {
    if (ImGui::BeginCombo("Plugins", s.selected_idx>=0 ? s.plugin_metas[s.selected_idx].name.c_str() : "<none>")) {
      for (int i=0;i<(int)s.plugin_metas.size();++i) {
        bool sel = (i == s.selected_idx);
        if (ImGui::Selectable(s.plugin_metas[i].name.c_str(), sel)) s.selected_idx = i;
        if (sel) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
  }

  ImGui::Separator();
  ImGui::Text("Deadline: %.1f ms  |  Stall: %.1f ms", s.deadline_ms_cfg, s.settings.stall_ms);
  if (!s.plugin_loaded) {
    ImGui::BeginDisabled(s.selected_idx < 0);
    if (ImGui::Button("Load")) {
      if (s.runner && s.selected_idx >= 0) {
        auto path = s.plugin_metas[s.selected_idx].path;
        if (s.runner->load(path)) {
          s.plugin_loaded = true;
          auto pos = path.find_last_of("/\\");
          s.current_plugin_leaf = (pos==std::string::npos)? path : path.substr(pos+1);
          std::snprintf(s.plugin_status, sizeof(s.plugin_status), "Loaded: %s", s.plugin_metas[s.selected_idx].name.c_str());
          s.toasts.info("Plugin loaded");
          
          // Phase 12: Initialize draw list
          std::random_device rd;
          std::string dl_name = "gpi_dl_" + std::to_string(rd());
          uint32_t dl_bytes = sizeof(GPI_DrawListV1) + 4096 * sizeof(GPI_QuadV1);
          if (dl_create_host(s.dl_map, dl_name, dl_bytes)) {
            s.dl_host = (GPI_DrawListV1*)s.dl_map.base;
            s.dl_host->magic = GPI_DL_MAGIC;
            s.dl_host->version = 0x00010000;
            s.dl_host->max_quads = 4096;
            s.dl_host->quad_count = 0;
          }
          
          // Phase 13: Initialize draw list v1.5
          const uint32_t MAXQ=4096, MAXT=1024, UTF8=64*1024;
          uint32_t dl15_bytes = sizeof(GPI_DrawListV15) + MAXQ*sizeof(GPI_QuadV1) + MAXT*sizeof(GPI_TextRunV15) + UTF8;
          s.dl15_host = (GPI_DrawListV15*)std::malloc(dl15_bytes);
          if (s.dl15_host) {
            s.dl15_host->magic = GPI_DL15_MAGIC;
            s.dl15_host->version = GPI_DL15_VERSION;
            s.dl15_host->max_quads = MAXQ; s.dl15_host->quad_count = 0;
            s.dl15_host->max_text = MAXT; s.dl15_host->text_count = 0;
            s.dl15_host->utf8_capacity = UTF8; s.dl15_host->utf8_size = 0;
          }
        } else {
          std::snprintf(s.plugin_status, sizeof(s.plugin_status), "Load failed: %s", s.runner->last_error());
          s.toasts.error("Load failed");
        }
      }
    }
    ImGui::EndDisabled();
  } else {
    if (ImGui::Button("Unload")) {
      if (s.runner) s.runner->unload();
      s.plugin_loaded = false;
      std::snprintf(s.plugin_status, sizeof(s.plugin_status), "Unloaded");
      s.toasts.info("Plugin unloaded");
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(s.plugin_metas.empty());
    if (ImGui::Button("Hot-swap")) {
      if (!s.plugin_metas.empty() && s.runner) {
        int next = (s.selected_idx + 1) % (int)s.plugin_metas.size();
        s.runner->unload();
        if (s.runner->load(s.plugin_metas[next].path)) {
          s.selected_idx = next;
          s.plugin_loaded = true;
          auto pos = s.plugin_metas[next].path.find_last_of("/\\");
          s.current_plugin_leaf = (pos==std::string::npos)? s.plugin_metas[next].path : s.plugin_metas[next].path.substr(pos+1);
          std::snprintf(s.plugin_status, sizeof(s.plugin_status), "Loaded: %s", s.plugin_metas[next].name.c_str());
          s.toasts.info("Hot-swapped");
        } else {
          s.plugin_loaded = false;
          s.toasts.error("Hot-swap failed");
        }
      }
    }
    ImGui::EndDisabled();
  }

  ImGui::Separator();
  ImGui::Text("Status: %s", s.plugin_loaded ? "Loaded" : s.plugin_status);
  if (s.plugin_loaded) {
    if (ImGui::Checkbox("Simulate stall", &s.simulate_stall)) { }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    float stall_ms_f = (float)s.stall_ms;
    ImGui::DragFloat("stall ms", &stall_ms_f, 1.0f, 0.0f, 1000.0f, "%.0f");
    s.stall_ms = (double)stall_ms_f;
  }

  ImGui::End();
}

} // namespace

int main(int argc, char** argv) {
  Cli cli = parse_cli(argc, argv);
  
  
  
  // Phase 14: Handle golden verification
  // if (!cli.golden_verify.empty()) {
  //   GoldenMeta meta;
  //   if (!load_golden_meta(cli.golden_verify, meta)) {
  //     std::fprintf(stderr, "Failed to load golden meta from %s\n", cli.golden_verify.c_str());
  //     return 1;
  //   }
  //   
  //   std::string replay_path = cli.golden_verify + "/pong_smoke.replay";
  //   std::string out_dir = "artifacts/golden_pong_smoke_run";
  //   std::string report_path;
  //   bool pass;
  //   
  //   if (!verify_golden(cli.golden_verify, replay_path, meta, out_dir, report_path, pass)) {
  //     std::fprintf(stderr, "Golden verification failed\n");
  //     return 1;
  //   }
  //   
  //   std::printf("Golden verification %s\n", pass ? "PASSED" : "FAILED");
  //   std::printf("Report: %s\n", report_path.c_str());
  //   return pass ? 0 : 1;
  // }
  
  AppState s{};

  if (!init_window(s, cli)) { shutdown(s); return 1; }

  if (cli.headless) {
    SDL_HideWindow(s.window);
    s.hud_visible = false;
  }

  // Settings
  cfg::load_from_file("config/settings.toml", s.settings);
  s.hud_visible = s.settings.ui_hud;
  s.plugins_dir = s.settings.plugins_dir;
  s.deadline_ms_cfg = s.settings.deadline_ms;
  s.show_store = s.settings.show_store;
  
  // Phase 8: Initialize runner based on isolation setting
  auto api = make_host_api(s);
  if (s.settings.isolation) {
    s.runner = make_runner_child();
  } else {
    s.runner = make_runner_inproc(api, s.deadline_ms_cfg);
  }
  
  // Phase 8: Initialize replay/record
  if (!cli.record_path.empty()) {
    s.recorder.start(cli.record_path);
  }
  if (!cli.replay_path.empty()) {
    s.replayer.load(cli.replay_path);
  }

  // Phase 8: Discover plugins with metadata
  s.plugin_metas = scan_with_metadata(s.plugins_dir);
  if (!s.plugin_metas.empty()) {
    if (cli.headless && !cli.plugin.empty()) {
      for (size_t i=0;i<s.plugin_metas.size();++i)
        if (s.plugin_metas[i].path.find(cli.plugin) != std::string::npos)
          s.selected_idx = (int)i;
    } else if (!s.settings.last_plugin.empty()) {
      for (size_t i=0;i<s.plugin_metas.size();++i)
        if (s.plugin_metas[i].path.find(s.settings.last_plugin) != std::string::npos)
          s.selected_idx = (int)i;
    }
    if (s.selected_idx < 0) s.selected_idx = 0;
  }
  
  // Phase 10: Demo mode setup
  s.demo_mode = cli.demo;
  if (s.demo_mode) {
    s.hud_visible = false;
    if (!cli.plugin.empty()) {
      for (size_t i=0;i<s.plugin_metas.size();++i)
        if (s.plugin_metas[i].name.find(cli.plugin) != std::string::npos ||
            s.plugin_metas[i].path.find(cli.plugin) != std::string::npos)
          s.selected_idx = (int)i;
    }
  }

  // Phase 8: Load plugin using runner
  if (cli.headless && s.selected_idx >= 0 && s.runner) {
    auto path = s.plugin_metas[s.selected_idx].path;
    if (s.runner->load(path)) {
      s.plugin_loaded = true;
      auto pos = path.find_last_of("/\\");
      s.current_plugin_leaf = (pos==std::string::npos)? path : path.substr(pos+1);
    }
  }

  // Start watchdog
  s.watchdog.start(&s.app_running, &s.in_plugin_call, &s.plugin_call_start,
                   s.settings.stall_ms,
                   [&](){
                     if (s.runtime.loaded) {
                       s.runtime.unload();
                       s.plugin_loaded = false;
                       std::snprintf(s.plugin_status, sizeof(s.plugin_status),
                                     "Watchdog: plugin stalled > %.1f ms", s.settings.stall_ms);
                     }
                   });

  const double target_dt = 1.0 / static_cast<double>(s.cfg.target_fps);
  auto last = std::chrono::high_resolution_clock::now();
  double acc = 0.0;

  while (s.running) {
    auto start_frame = std::chrono::high_resolution_clock::now();

    InputSnapshot in{};
    poll_input(in, s);
        if (in.key_escape || in.quit) s.running = false;
        if (in.toggle_hud) s.hud_visible = !s.hud_visible;
        
        const Uint8* ks = SDL_GetKeyboardState(nullptr);
        if (ks[SDL_SCANCODE_F9]) save_artifacts_now(s);
        if (ks[SDL_SCANCODE_F12]) {
          std::string ts = timestamp();
          std::string path = "artifacts/snap-" + ts + ".png";
          int w, h; SDL_GetWindowSize(s.window, &w, &h);
          if (save_screenshot_png(path, w, h)) {
            s.toasts.info("Screenshot saved: " + path);
          }
        }

    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> delta = now - last;
    last = now;
    acc += delta.count();

    if (s.cfg.fixed_timestep) {
      while (acc >= target_dt) {
        acc -= target_dt;
      }
    }

    // Phase 8: Handle replay/record and plugin update
    ReplayFrame rf;
    GPI_InputV1 ib = make_input_blob(in);
    std::vector<uint8_t> ib_bytes(sizeof(GPI_InputV1));
    std::memcpy(ib_bytes.data(), &ib, sizeof(GPI_InputV1));

    FrameArgs fa{};
    if (s.replayer.active() && s.replayer.next(rf)) {
      // Replay mode: use recorded data
      fa.dt_sec = rf.dt_sec; fa.fb_w = rf.fb_w; fa.fb_h = rf.fb_h; fa.t_ns = rf.t_ns;
      fa.input_blob = rf.input.data(); fa.input_size = (uint32_t)rf.input.size(); fa.input_version = GPI_INPUT_VERSION;
    } else {
      // Live mode
      fa.dt_sec = s.cfg.fixed_timestep ? (float)target_dt : (float)delta.count();
      int w,h; SDL_GetWindowSize(s.window,&w,&h); fa.fb_w=w; fa.fb_h=h;
      fa.t_ns = (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
                  std::chrono::high_resolution_clock::now().time_since_epoch()).count();
      fa.input_blob = ib_bytes.data(); fa.input_size = (uint32_t)ib_bytes.size(); fa.input_version = GPI_INPUT_VERSION;

      if (s.recorder.active()) {
        ReplayFrame add{ fa.t_ns, fa.fb_w, fa.fb_h, fa.dt_sec, ib_bytes };
        s.recorder.add(add);
      }
    }

    // Phase 9: UPDATE using runner with per-call metrics
    if (s.plugin_loaded && s.runner) {
      s.in_plugin_call.store(true, std::memory_order_relaxed);
      s.plugin_call_start.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
      auto start = std::chrono::high_resolution_clock::now();
      bool ok = s.runner->update(fa);
      if (s.simulate_stall) SDL_Delay((Uint32)s.stall_ms);
      auto end = std::chrono::high_resolution_clock::now();
      s.in_plugin_call.store(false, std::memory_order_relaxed);
      
      if (ok) {
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        s.perf_calls.upd.push(ms);
      } else {
        s.runner->unload();
        s.plugin_loaded = false;
        std::snprintf(s.plugin_status, sizeof(s.plugin_status),
                      "Plugin update failed: %s", s.runner->last_error());
        if (!cli.headless) s.toasts.error("Plugin update failed");
      }
    }

    // Call ImGui new_frame only in GUI mode
    if (!cli.headless) {
      s.imgui.new_frame(s.window);
    }

    int w, h; SDL_GetWindowSize(s.window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Phase 12: Render draw list (only in GUI mode)
    if (!cli.headless && s.dl_host) {
      render_drawlist_v1(s.dl_host);
    }
    
    // Phase 13: Render draw list v1.5 (only in GUI mode)
    if (!cli.headless && s.dl15_host) {
      render_drawlist_v15(s.dl15_host, s.host_font);
    }

    // Phase 9: RENDER using runner with per-call metrics
    if (s.plugin_loaded && s.runner) {
      s.in_plugin_call.store(true, std::memory_order_relaxed);
      s.plugin_call_start.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
      auto start = std::chrono::high_resolution_clock::now();
      bool ok = s.runner->render();
      if (s.simulate_stall) SDL_Delay((Uint32)s.stall_ms);
      auto end = std::chrono::high_resolution_clock::now();
      s.in_plugin_call.store(false, std::memory_order_relaxed);
      
      if (ok) {
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        s.perf_calls.ren.push(ms);
        
        // Phase 9: Video recording
        if (s.record_video) {
          save_frame_sequence_png(s.video_dir, s.video_frame_idx++, w, h);
        }
      } else {
        s.runner->unload();
        s.plugin_loaded = false;
        std::snprintf(s.plugin_status, sizeof(s.plugin_status),
                      "Plugin render failed: %s", s.runner->last_error());
        if (!cli.headless) s.toasts.error("Plugin render failed");
      }
    }

    const auto end_logic = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> frame_ms = end_logic - start_frame;
    s.hist.push(frame_ms.count());
    
    // Phase 10: Demo timeline
    if (s.demo_mode) {
      s.demo_t += frame_ms.count() / 1000.0;
      
      if (s.demo_t > 12.0 && s.plugin_metas.size() > 1 && s.plugin_loaded && !s.demo_swapped) {
        // Hot-swap to next plugin
        int next = (s.selected_idx + 1) % (int)s.plugin_metas.size();
        s.runner->unload();
        if (s.runner->load(s.plugin_metas[next].path)) {
          s.selected_idx = next;
          s.plugin_loaded = true;
          auto pos = s.plugin_metas[next].path.find_last_of("/\\");
          s.current_plugin_leaf = (pos==std::string::npos)? s.plugin_metas[next].path : s.plugin_metas[next].path.substr(pos+1);
          save_artifacts_now(s);
          
          // Screenshot
          save_screenshot_png("artifacts/demo_snap.png", w, h);
          
          // Start video sequence
          s.video_dir = "artifacts/demo_video";
          s.record_video = true;
          s.video_frame_idx = 0;
          s.demo_swapped = true;
        }
      }
      
      if (s.record_video) {
        save_frame_sequence_png(s.video_dir, s.video_frame_idx++, w, h);
        if (s.video_frame_idx >= 60) {
          s.record_video = false;
          save_artifacts_now(s);
          s.running = false; // Exit demo
        }
      }
    }
    // Draw HUD and UI only in GUI mode
    if (!cli.headless) {
      draw_hud(s, s.hist.stats());
      draw_plugin_panel(s);
      if (s.show_store) {
        bool load_clicked = false;
        if (draw_store_panel(s.plugin_metas, s.selected_idx, load_clicked)) {
          // Plugin selection changed, reload if needed
        }
        if (load_clicked && s.runner && s.selected_idx >= 0) {
          auto path = s.plugin_metas[s.selected_idx].path;
          printf("DEBUG: Attempting to load plugin: %s\n", path.c_str());
          printf("DEBUG: Runner exists: %s\n", s.runner ? "yes" : "no");
          if (s.runner->load(path)) {
            s.plugin_loaded = true;
            auto pos = path.find_last_of("/\\");
            s.current_plugin_leaf = (pos==std::string::npos)? path : path.substr(pos+1);
            std::snprintf(s.plugin_status, sizeof(s.plugin_status), "Loaded: %s", s.plugin_metas[s.selected_idx].name.c_str());
            s.toasts.info("Plugin loaded");
            
            // Phase 12: Initialize draw list
            std::random_device rd;
            std::string dl_name = "gpi_dl_" + std::to_string(rd());
            uint32_t dl_bytes = sizeof(GPI_DrawListV1) + 4096 * sizeof(GPI_QuadV1);
            if (dl_create_host(s.dl_map, dl_name, dl_bytes)) {
              s.dl_host = (GPI_DrawListV1*)s.dl_map.base;
              s.dl_host->magic = GPI_DL_MAGIC;
              s.dl_host->version = 0x00010000;
              s.dl_host->max_quads = 4096;
              s.dl_host->quad_count = 0;
            }
            
            // Phase 13: Initialize draw list v1.5
            const uint32_t MAXQ=4096, MAXT=1024, UTF8=64*1024;
            uint32_t dl15_bytes = sizeof(GPI_DrawListV15) + MAXQ*sizeof(GPI_QuadV1) + MAXT*sizeof(GPI_TextRunV15) + UTF8;
            s.dl15_host = (GPI_DrawListV15*)std::malloc(dl15_bytes);
            if (s.dl15_host) {
              s.dl15_host->magic = GPI_DL15_MAGIC;
              s.dl15_host->version = GPI_DL15_VERSION;
              s.dl15_host->max_quads = MAXQ; s.dl15_host->quad_count = 0;
              s.dl15_host->max_text = MAXT; s.dl15_host->text_count = 0;
              s.dl15_host->utf8_capacity = UTF8; s.dl15_host->utf8_size = 0;
            }
          } else {
            std::snprintf(s.plugin_status, sizeof(s.plugin_status), "Load failed: %s", s.runner->last_error());
            s.toasts.error("Load failed");
          }
        }
      }
      s.perf_calls.draw_small();
      s.log_panel.draw(s.logs);
      s.toasts.render();

      // Periodically flush telemetry to artifacts
      static double last_flush = 0.0;
      static double acc_time = 0.0;
      acc_time += frame_ms.count() / 1000.0;
      if (acc_time - last_flush > 5.0) {
        s.telemetry.flush_csv("artifacts/telemetry.csv");
        s.telemetry.flush_json("artifacts/telemetry.json");
        last_flush = acc_time;
      }
    }
    
    // Call ImGui render only in GUI mode
    if (!cli.headless) {
      s.imgui.render();
    }
    SDL_GL_SwapWindow(s.window);

    // Count frames and exit if reached
    static int frame_counter=0;
    frame_counter++;
    
    // Phase 14: Golden capture
    // if (!cli.golden_capture.empty() && cli.headless) {
    //   // Check if this frame should be captured (simple check for now)
    //   if (frame_counter == 120 || frame_counter == 240) {
    //     int w, h;
    //     SDL_GetWindowSize(s.window, &w, &h);
    //     char path[256];
    //     std::snprintf(path, sizeof(path), "%s/frame_%06d.png", cli.golden_capture.c_str(), frame_counter);
    //     save_screenshot_png(path, w, h);
    //   }
    // }
    
    if (cli.headless && cli.frames>0 && frame_counter>=cli.frames) {
      save_artifacts_now(s);
      auto st = s.hist.stats();
      bool pass = (st.p99_ms <= 18.0);
      shutdown(s);
      return pass ? 0 : 1;
    }
  }

  s.app_running.store(false);
  // Phase 8: Stop recorder and unload runner
  if (s.recorder.active()) s.recorder.stop();
  if (s.runner) s.runner->unload();
  
  s.watchdog.stop();
  s.settings.ui_hud = s.hud_visible;
  if (s.plugin_loaded && s.selected_idx >= 0) {
    s.settings.last_plugin = s.plugin_metas[s.selected_idx].name;
  }
  cfg::save_to_file("config/settings.toml", s.settings);

  shutdown(s);
  return 0;
}