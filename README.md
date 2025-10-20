# GPI Sandbox — A Modular Game Platform Host in Modern C++

A **native desktop app** (Mac/Windows/Linux) that runs a **plugin-based mini-app/game platform**. The host discovers, loads, and hot-swaps C++ game plugins at runtime while maintaining **smooth 60 FPS**, **crash isolation**, and **telemetry**. It demonstrates **interface design, real-time rendering, multithreading, and shipping discipline**—the same concerns you'd face in an in-car UI.

## 🎮 Current Status: WORKING!

- **Phase 17 Complete** - The GPI Sandbox is now fully functional with:
- **Working plugin system** - Pong and Snake games load and run successfully
- **Plugin Store UI** - Browse and load plugins with one click
- **Performance monitoring** - Real-time FPS and frame time metrics
- **Hot-swapping** - Switch between games without restarting
- **Crash isolation** - Plugins run safely in-process (isolation mode available)
- **Complete documentation** - README, architecture docs, and SDK
- **CI/CD ready** - GitHub Actions, packaging scripts, and release automation

**🎯 Ready to play!** Launch the app and enjoy the games!

### Quick Start (30 seconds)
```bash
# Build and run
cmake --preset release
cmake --build --preset release
./out/release/gpi_host

# In the app:
# 1. Click "Pong" or "Snake" in the Plugin Store
# 2. Click "Load Plugin" button
# 3. Play! (Pong: arrow keys, Snake: WASD/arrows)
```

---

## Table of Contents
- [Goals & Non-Goals](#goals--non-goals)
- [Feature Set](#feature-set)
- [Architecture Overview](#architecture-overview)
- [Plugin ABI & Lifecycle](#plugin-abi--lifecycle)
- [Threading & Timing Model](#threading--timing-model)
- [Performance Targets & Telemetry](#performance-targets--telemetry)
- [Fault Tolerance & Safety](#fault-tolerance--safety)
- [Project Layout](#project-layout)
- [Build & Run](#build--run)
- [Configuration](#configuration)
- [Demo Script (2 minutes)](#demo-script-2-minutes)
- [Testing, CI & Quality Gates](#testing-ci--quality-gates)
- [Conventions](#conventions)
- [Roadmap](#roadmap)
- [License](#license)

---

## Goals & Non-Goals

**Goals**
- Clean, stable **C ABI** for loading **C++ plugins** across compilers/OSes.
- **Hot-load/unload** plugins without restarting the host.
- **Frame-paced render loop** with a strict **16.6 ms** budget.
- **Graceful crash handling**: a misbehaving plugin cannot freeze the host.
- **Telemetry & UX polish**: on-screen Perf HUD, artifacts saved to disk.
- “Download → run in 30s” developer experience (DX).

**Non-Goals**
- AAA graphics or complex physics.
- Networked multiplayer.
- Scripting languages (Lua, JS) in v1—focus is C++ plugins.

---

## Feature Set

- **Launcher UI**
  - Discovers plugins in a folder.
  - Shows tiles (icon, name, version, last played).
  - One-click **Launch**, **Suspend/Resume**, **Unload**.
- **Runtime**
  - **Hot-load** shared libs (`.dll`/`.so`/`.dylib`) via a C ABI.
  - **Frame pacing**: fixed or hybrid timestep; FPS governor.
  - **Input unification** (keyboard/mouse/controller) passed to plugins.
  - **Save data service**: per-plugin key/value blob storage.
- **Resilience**
  - **Watchdog** to detect plugin stalls/crashes.
  - **Isolation mode** (optional): run plugin loop in a separate process; shared memory ring buffer for frames/events.
  - Automatic **state teardown** on failure; UI remains responsive.
- **Telemetry**
  - On-screen **Perf HUD**: FPS, avg/p95/p99 frame time, CPU%, memory, input-to-pixel latency estimate.
  - **Artifacts**: JSON report + PNG histograms saved per session.
- **Tooling**
  - **Plugin Wizard**: generate a new plugin skeleton from a template.
  - **Latency probe** tool to measure end-to-end delay.
  - **Trace logger** (CSV/JSON) for frame times and events.

---

## Architecture Overview

```
             ┌──────────────────────── Host App ────────────────────────┐
             │ UI (Launcher, HUD)   Core (Loop, Timing)   Telemetry     │
Input ───►   │  ┌─────────────┐     ┌──────────────────┐   ┌──────────┐ │
Events       │  │ View/Layout │◄──► │ Runtime/Plugins  │◄──│ Metrics  │ │
             │  └─────────────┘     └──────────────────┘   └──────────┘ │
             │         ▲                       ▲               ▲        │
             └─────────┼───────────────────────┼───────────────┼────────┘
                       │ OS/Windowing          │ Loader/API    │ Storage
                       ▼                       ▼               ▼
                  SDL2/Qt/ImGui           dlopen/LoadLibrary  Save Service

                                ┌──── Plugin .so/.dll ────┐
                                │ init/update/render/...  │
                                │ uses host callbacks     │
                                └──────────────────────────┘
```

**Host layers**
- **Core Loop**: time step, input pump, render scheduling.
- **Runtime**: plugin discovery, loading, lifecycle, watchdog, isolation.
- **Services**: logging, metrics, save store, config.
- **UI**: launcher, plugin tiles, perf HUD, dialogs/toasts.

---

## Plugin ABI & Lifecycle

### Minimal C ABI (header excerpt)
```c
// gpi_plugin.h  (C-compatible header)
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float dt_sec;               // delta time
  int   width, height;        // framebuffer size
  const unsigned char* input; // opaque input snapshot from host
} GPI_FrameContext;

typedef struct {
  // Host function pointers available to plugins:
  void (*log_info)(const char* msg);
  void (*save_put)(const char* key, const void* data, int size);
  int  (*save_get)(const char* key, void* out, int capacity);
  void (*telemetry_mark)(const char* key, double value);
} GPI_HostApi;

// Required plugin exports:
__attribute__((visibility("default")))
int gpi_init(const GPI_HostApi* api);

__attribute__((visibility("default")))
int gpi_update(const GPI_FrameContext* ctx);

__attribute__((visibility("default")))
int gpi_render();  // draw with host-provided GPU context (ImDrawList/Qt painter)

__attribute__((visibility("default")))
void gpi_suspend();   // app goes to background

__attribute__((visibility("default")))
void gpi_resume();    // app returns to foreground

__attribute__((visibility("default")))
void gpi_shutdown();

#ifdef __cplusplus
}
#endif
```

### Lifecycle
1. **Load** shared library → resolve required symbols.
2. **`gpi_init`** with `GPI_HostApi` (logging, save, telemetry).
3. Host starts frame loop: **`gpi_update` → `gpi_render`** per frame.
4. On backgrounding: **`gpi_suspend`**. On return: **`gpi_resume`**.
5. On unload or exit: **`gpi_shutdown`** then library is closed.

**Versioning**
- `GPI_ABI_VERSION` integer checked at load; minor feature flags via `gpi_query_capabilities()` (optional).

---

## Threading & Timing Model

- **Main thread**: window events, input snapshotting, **render submission**.
- **Worker (optional)**: physics/AI/audio for the active plugin.
- **Watchdog thread**: monitors plugin callbacks (deadline and stall detection).
- **Frame pacing**
  - Target **60 FPS** (16.6 ms budget).
  - Hybrid step: fixed physics step + interpolated render, or pure variable `dt` (configurable).
- **Input-to-pixel latency**
  - Timestamp inputs on capture; propagate through `GPI_FrameContext`.
  - HUD estimates median & p99 latency.

---

## Performance Targets & Telemetry

**Targets (default build on a 4-core laptop)**
- **FPS ≥ 60**, **p99 frame time ≤ 18 ms** with sample plugins.
- **Input-to-pixel median < 40 ms**.
- **Host survives plugin crash/unload** with no UI freeze.

**Telemetry**
- On-screen **Perf HUD** (toggle with `F10`):
  - FPS, avg/p95/p99 frame time
  - CPU %, mem usage
  - Dropped frame %
  - Input→pixel latency median/p99
- **Artifacts** saved on exit or via `F9`:
  - `artifacts/session-YYYYMMDD-HHMMSS.json` (stats, build info, OS)
  - `artifacts/frame_hist.png` (histogram)
  - `artifacts/latency_hist.png`

---

## Fault Tolerance & Safety

- **Crash containment**
  - Host wraps plugin calls with guard rails; if a call faults or exceeds a **deadline** (config), host **unloads** the plugin and shows a non-modal toast.
- **Isolation mode (optional)**
  - Run plugin in a **child process**.
  - Exchange frame commands via **shared memory ring buffer** + eventfd/condvar.
  - If child dies, host cleans up and stays alive.
- **Resource safety**
  - RAII wrappers for GPU/context handles.
  - ASan/UBSan presets; hardening flags in Release (`-fno-exceptions` for plugin boundary if desired).

---

## Project Layout

```
gpi-sandbox/
├─ cmake/                 # toolchains, modules
├─ external/              # third-party (SDL2/Qt, imgui, fmt, spdlog, gtest)
├─ host/
│  ├─ core/               # loop, timing, input, renderer glue
│  ├─ runtime/            # loader, watchdog, isolation bridge
│  ├─ services/           # logging, telemetry, save store, config
│  ├─ ui/                 # launcher, tiles, dialogs, perf HUD
│  └─ main.cpp
├─ plugins/
│  ├─ pong/               # sample plugin A
│  ├─ snake/              # sample plugin B
│  └─ template/           # cookiecutter/skeleton
├─ tools/
│  ├─ plugin_wizard/      # new plugin generator
│  ├─ latency_probe/
│  └─ trace_to_png/
├─ artifacts/             # perf outputs (gitignored)
├─ config/
│  ├─ settings.toml       # frame budget, isolation, HUD, folders
│  └─ keymap.toml
├─ tests/                 # unit & integration tests
├─ CMakePresets.json
├─ README.md
└─ LICENSE
```

---

## Build & Run

**Requirements**
- C++20, CMake ≥ 3.22, Ninja/Make, one of:
  - **SDL2 + Dear ImGui** (default), or
  - **Qt 6** (set `-DGPI_USE_QT=ON`)
- Windows: MSVC 2022+; macOS: Clang 15+; Linux: gcc 12+ or clang 15+

**Quick Start (Release)**
```bash
git clone https://github.com/yourname/gpi-sandbox
cd gpi-sandbox
cmake --preset release
cmake --build --preset release
./out/release/gpi_host
```

**Windows (MSVC)**
```powershell
cmake --preset msvc-release
cmake --build --preset msvc-release
.\out\msvc-release\gpi_host.exe
```

**Run Options**
```
gpi_host --plugins ./plugins/bin --fps 60 --isolation=off --hud=on
```

---

## Configuration

`config/settings.toml`
```toml
[runtime]
plugins_dir = "./plugins/bin"
isolation   = false          # true = run plugin in child process
deadline_ms = 12             # per-callback soft deadline

[render]
target_fps     = 60
fixed_timestep = true
vsync          = true

[hud]
enabled = true
opacity = 0.85
show_latency = true

[save]
dir = "./userdata"

[input]
gamepad_deadzone = 0.15
```

`config/keymap.toml`
```toml
[global]
toggle_hud = "F10"
save_artifacts = "F9"
quit = "Esc"
```

---

## Demo Script (2 minutes)

1. **Launch** host → tiles for **Pong** and **Snake**, HUD visible.
2. Click **Pong** → instant start; move paddle; point out **FPS/p99**.
3. Open menu **“Simulate Crash”** → toast “Plugin safely unloaded,” UI still responsive.
4. Launch **Snake** without restarting → show **hot-swap**.
5. Press **F9** to save artifacts → show JSON/PNG in `./artifacts`.

---

## Testing, CI & Quality Gates

- **Unit tests** (GoogleTest): loader, save store, ring buffer, time stepper.
- **Integration tests**: headless frame loop renders N frames with a null plugin; assert timing bounds.
- **Sanitizers**: `asan`, `ubsan` presets; must pass locally and in CI.
- **Static analysis**: clang-tidy with a curated checks list.
- **CI (GitHub Actions)**:
  - Matrix: Win + macOS + Linux (Release + ASan).
  - Artifacts: zipped binaries + a sample **perf report**.
- **Code coverage**: optional in Debug builds (lcov/llvm-cov).

---

## Conventions

- **C ABI at boundary**, C++ internally (RAII, `std::unique_ptr`, `std::span`, `std::chrono`).
- **Error handling**: `expected<T,E>` style returns internally; never throw across plugin boundary.
- **Logging**: `spdlog` with rotating sinks; plugin logs are tagged.
- **Formatting**: clang-format, `.editorconfig`.
- **Docs**: Doxygen comments for ABI headers.

---

## Roadmap

- **v1.1**: Deterministic replay (record input, re-render identical frames).
- **v1.2**: GPU instancing helper for plugin render calls.
- **v1.3**: Sandboxed isolation (child process) default on Windows/macOS/Linux.
- **v1.4**: In-host plugin store (download/update via manifest).

---

## License

MIT (host). Example plugins are MIT. Third-party libraries retain their original licenses.

---

### Appendix: Hello Plugin (C++) — Skeleton

```cpp
// build as C++ but export C symbols
#include "gpi_plugin.h"
#include <cmath>
static const GPI_HostApi* G = nullptr;

extern "C" int gpi_init(const GPI_HostApi* api) {
  G = api; G->log_info("hello: init");
  return 0;
}

extern "C" int gpi_update(const GPI_FrameContext* ctx) {
  // read input from ctx->input (opaque snapshot), update state
  G->telemetry_mark("hello.phase", std::sin(ctx->dt_sec));
  return 0; // 0 = OK
}

extern "C" int gpi_render() {
  // draw using host-provided draw list / Qt painter (passed via global)
  return 0;
}

extern "C" void gpi_suspend() {}
extern "C" void gpi_resume() {}
extern "C" void gpi_shutdown() { G->log_info("hello: bye"); }
```
