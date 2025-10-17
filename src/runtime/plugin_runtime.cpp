#include "plugin_runtime.h"
#include <chrono>
#include <cstdio>

static bool resolve_syms(LoadedLibrary lib, PluginFns& f) {
  f.init     = (PluginFns::init_t)    PluginLoader::sym(lib, "gpi_init");
  f.caps     = (PluginFns::caps_t)    PluginLoader::sym(lib, "gpi_query_capabilities");
  f.update   = (PluginFns::update_t)  PluginLoader::sym(lib, "gpi_update");
  f.render   = (PluginFns::render_t)  PluginLoader::sym(lib, "gpi_render");
  f.suspend  = (PluginFns::suspend_t) PluginLoader::sym(lib, "gpi_suspend");
  f.resume   = (PluginFns::resume_t)  PluginLoader::sym(lib, "gpi_resume");
  f.shutdown = (PluginFns::shutdown_t)PluginLoader::sym(lib, "gpi_shutdown");
  f.debug_crash = (PluginFns::debug_crash_t) PluginLoader::sym(lib, "gpi_debug_crash");
  return f.init && f.update && f.render;
}

bool PluginRuntime::load(const std::string& fullpath, double dl_ms, const GPI_HostApi& api) {
  unload();
  lib = PluginLoader::open(fullpath);
  if (!lib.handle) { last_error = "dlopen/LoadLibrary failed"; return false; }
  if (!resolve_syms(lib, fns)) { last_error = "required symbols missing"; unload(); return false; }
  path = fullpath; deadline_ms = dl_ms; host_api = api;

  if (!call_init()) { unload(); return false; }
  if (fns.caps) caps = fns.caps(); else caps.caps = GPI_CAP_NONE;
  loaded = true;
  return true;
}

void PluginRuntime::unload() {
  if (loaded && fns.shutdown) { fns.shutdown(); }
  loaded = false; path.clear();
  fns = PluginFns{};
  if (lib.handle) PluginLoader::close(lib);
  lib = LoadedLibrary{};
}

static bool timed_call(double deadline_ms, double& out_ms, auto&& fn) {
  using clock = std::chrono::high_resolution_clock;
  auto t0 = clock::now();
  const bool ok = fn();
  auto t1 = clock::now();
  std::chrono::duration<double, std::milli> ms = t1 - t0;
  out_ms = ms.count();
  return ok && (ms.count() <= deadline_ms);
}

bool PluginRuntime::call_init() {
  last_error = nullptr;
  auto thunk = [&]() {
    GPI_Result r = fns.init(&ver, &host_api);
    if (r != GPI_OK) { last_error = "gpi_init failed"; return false; }
    return true;
  };
  return timed_call(deadline_ms, last_call_ms, thunk);
}

bool PluginRuntime::call_update(const GPI_FrameContext& ctx) {
  last_error = nullptr;
  auto thunk = [&]() {
    GPI_Result r = fns.update(&ctx);
    if (r != GPI_OK) { last_error = "gpi_update failed"; return false; }
    return true;
  };
  return timed_call(deadline_ms, last_call_ms, thunk);
}

bool PluginRuntime::call_render() {
  last_error = nullptr;
  auto thunk = [&]() {
    GPI_Result r = fns.render();
    if (r != GPI_OK) { last_error = "gpi_render failed"; return false; }
    return true;
  };
  return timed_call(deadline_ms, last_call_ms, thunk);
}

void PluginRuntime::call_suspend() { if (fns.suspend) fns.suspend(); }
void PluginRuntime::call_resume()  { if (fns.resume)  fns.resume();  }
void PluginRuntime::call_shutdown(){ if (fns.shutdown)fns.shutdown();}
