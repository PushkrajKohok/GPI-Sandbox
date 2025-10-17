#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <sys/stat.h>
#include "../include/gpi/gpi_plugin.h"
#include "../src/runtime/plugin_loader.h"
#include "../src/runtime/child_shm.h"
#include "crash_report.h"
#include "../src/platform/sandbox.h"

static LoadedLibrary lib{};
static GPI_HostApi host{}; 
static GPI_VersionInfo ver{ GPI_ABI_VERSION, 0 };
static GPI_InputV1 inbuf{};
static GPI_FrameContext ctx{};
static struct {
  decltype(&gpi_init)    init=nullptr;
  decltype(&gpi_update)  update=nullptr;
  decltype(&gpi_render)  render=nullptr;
  decltype(&gpi_shutdown)shutdown=nullptr;
} P;

static std::vector<uint8_t> from_b64(const std::string& s) {
  std::vector<uint8_t> out;
  const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int val = 0, bits = 0;
  for (char c : s) {
    if (c == '=') break;
    const char* p = strchr(chars, c);
    if (!p) continue;
    val = (val << 6) | (p - chars);
    bits += 6;
    if (bits >= 8) {
      out.push_back((val >> (bits - 8)) & 0xFF);
      bits -= 8;
    }
  }
  return out;
}

static void send_rsp(shm::Block& shm, uint8_t phase, float ms) {
  std::vector<uint8_t> rsp = {3, phase}; // RspOk
  rsp.insert(rsp.end(), (uint8_t*)&ms, (uint8_t*)&ms + 4);
  shm::write_msg(shm.rsp, rsp.data(), rsp.size());
  shm::signal(shm.ctrl->ev_host_wake);
}

int main(int argc, char** argv) {
  std::string shm_name, lib_path, workdir = "userdata/child";
  for (int i = 1; i < argc; i += 2) {
    if (i + 1 >= argc) break;
    std::string arg = argv[i];
    if (arg == "--shm") shm_name = argv[i + 1];
    else if (arg == "--lib") lib_path = argv[i + 1];
    else if (arg == "--work") workdir = argv[i + 1];
  }
  
  if (shm_name.empty() || lib_path.empty()) return 1;
  
  // Phase 11: Security hardening
  std::string dumps = workdir + "/crashdumps";
  mkdir(dumps.c_str(), 0755);
  install_crash_handler(dumps);
  
  sandbox::scrub_environment();
  sandbox::tighten_dll_search_paths();
  SandboxSpec spec; spec.work_dir = workdir; spec.deny_network = true;
  std::string serr;
  if (!sandbox::enter(spec, serr)) {
    std::fprintf(stderr, "sandbox enter failed: %s\n", serr.c_str()); return 2;
  }
  sandbox::install_seccomp_default(true, serr);
  
  shm::Block shm;
  if (!shm::open_child(shm, shm_name)) return 1;
  
  // Load plugin
  lib = PluginLoader::open(lib_path);
  if (!lib.handle) return 1;
  P.init = (decltype(P.init)) PluginLoader::sym(lib, "gpi_init");
  P.update = (decltype(P.update))PluginLoader::sym(lib, "gpi_update");
  P.render = (decltype(P.render))PluginLoader::sym(lib, "gpi_render");
  P.shutdown = (decltype(P.shutdown))PluginLoader::sym(lib,"gpi_shutdown");
  if (!P.init || !P.update || !P.render) return 1;
  if (P.init(&ver, &host) != GPI_OK) return 1;
  
  // Send init response
  send_rsp(shm, 0, 0.0f);
  
  // Main loop
  while (true) {
    shm::wait(shm.ctrl->ev_child_wake, 1000);
    std::vector<uint8_t> cmd;
    if (!shm::read_msg(shm.cmd, cmd)) continue;
    if (cmd.empty()) continue;
    
    auto start = std::chrono::high_resolution_clock::now();
    uint8_t type = cmd[0];
    
    if (type == 1) { // CmdUpdate
      if (cmd.size() < 25) continue;
      float dt = *(float*)&cmd[1];
      int w = *(int32_t*)&cmd[5];
      int h = *(int32_t*)&cmd[9];
      uint64_t t = *(uint64_t*)&cmd[13];
      uint32_t input_n = *(uint32_t*)&cmd[21];
      
      if (cmd.size() < 25 + input_n) continue;
      if (input_n >= sizeof(GPI_InputV1)) {
        std::memcpy(&inbuf, &cmd[25], sizeof(GPI_InputV1));
        ctx = GPI_FrameContext{dt, w, h, t, &inbuf, (uint32_t)sizeof(GPI_InputV1), GPI_INPUT_VERSION};
        if (P.update(&ctx) == GPI_OK) {
          auto end = std::chrono::high_resolution_clock::now();
          float ms = std::chrono::duration<float, std::milli>(end - start).count();
          send_rsp(shm, 1, ms);
        }
      }
    } else if (type == 2) { // CmdRender
      if (P.render() == GPI_OK) {
        auto end = std::chrono::high_resolution_clock::now();
        float ms = std::chrono::duration<float, std::milli>(end - start).count();
        send_rsp(shm, 2, ms);
      }
    }
  }
  
  if (P.shutdown) P.shutdown();
  PluginLoader::close(lib);
  shm::close_child(shm);
  return 0;
}
