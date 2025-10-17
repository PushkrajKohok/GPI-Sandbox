#pragma once
#include <string>
#include <cstdint>
#include "../platform/fs.h"
#include "plugin_loader.h"

extern "C" {
#include "../../include/gpi/gpi_plugin.h"
}

struct PluginFns {
  using init_t    = GPI_Result (*)(const GPI_VersionInfo*, const GPI_HostApi*);
  using caps_t    = GPI_Capabilities (*)(void);
  using update_t  = GPI_Result (*)(const GPI_FrameContext*);
  using render_t  = GPI_Result (*)(void);
  using suspend_t = void (*)(void);
  using resume_t  = void (*)(void);
  using shutdown_t= void (*)(void);
  using debug_crash_t = void (*)(void);

  init_t    init    = nullptr;
  caps_t    caps    = nullptr;
  update_t  update  = nullptr;
  render_t  render  = nullptr;
  suspend_t suspend = nullptr;
  resume_t  resume  = nullptr;
  shutdown_t shutdown= nullptr;
  debug_crash_t debug_crash = nullptr;
};

struct PluginRuntime {
  std::string path;
  LoadedLibrary lib{};
  PluginFns fns{};
  GPI_Capabilities caps{};
  bool loaded = false;
  double deadline_ms = 12.0;

  GPI_HostApi host_api{};
  GPI_VersionInfo ver{ GPI_ABI_VERSION, 0 };

  bool load(const std::string& fullpath, double deadline_ms, const GPI_HostApi& api);
  void unload();

  bool call_init();
  bool call_update(const GPI_FrameContext& ctx);
  bool call_render();
  void call_suspend();
  void call_resume();
  void call_shutdown();

  double last_call_ms = 0.0;
  const char* last_error = nullptr;
};
