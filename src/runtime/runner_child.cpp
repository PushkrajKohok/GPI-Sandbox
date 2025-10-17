#include "runner.h"
#include "child_shm.h"
#include "../platform/proc.h"
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <random>
#include <sys/stat.h>

static std::string b64(const void* data, uint32_t n) {
  const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  const uint8_t* p = (const uint8_t*)data;
  for (uint32_t i = 0; i < n; i += 3) {
    uint32_t v = (p[i] << 16) | ((i+1<n) ? (p[i+1] << 8) : 0) | ((i+2<n) ? p[i+2] : 0);
    for (int j = 0; j < 4; j++) {
      if (i*4/3 + j < (n*4+2)/3) out += chars[(v >> (18-j*6)) & 63];
      else out += '=';
    }
  }
  return out;
}

class RunnerChild : public IPluginRunner {
  shm::Block shm_;
  ChildProc child_;
  std::string err_;
  double last_call_ms_ = 0.0;
public:
  bool load(const std::string& lib) override {
    std::random_device rd;
    std::string shm_name = std::to_string(rd());
    if (!shm::create_host(shm_, shm_name)) { err_="shm create failed"; return false; }
    
    // Phase 11: Create plugin work directory
    std::string workdir = "userdata/" + std::to_string(rd());
    mkdir(workdir.c_str(), 0755);
    
    std::vector<std::string> args = {"--shm", shm_name, "--lib", lib, "--work", workdir};
    if (!proc::spawn_child("gpi_child", args, child_)) { err_="spawn failed"; return false; }
    
    // Wait for child init
    std::vector<uint8_t> msg;
    if (!shm::read_msg(shm_.rsp, msg)) { err_="no init response"; return false; }
    if (msg.size() < 2 || msg[0] != 3) { err_="init failed"; return false; }
    return true;
  }
  bool update(const FrameArgs& f) override {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<uint8_t> cmd;
    cmd.push_back(1); // CmdUpdate
    cmd.insert(cmd.end(), (uint8_t*)&f.dt_sec, (uint8_t*)&f.dt_sec + 4);
    cmd.insert(cmd.end(), (uint8_t*)&f.fb_w, (uint8_t*)&f.fb_w + 4);
    cmd.insert(cmd.end(), (uint8_t*)&f.fb_h, (uint8_t*)&f.fb_h + 4);
    cmd.insert(cmd.end(), (uint8_t*)&f.t_ns, (uint8_t*)&f.t_ns + 8);
    uint32_t input_n = f.input_size;
    cmd.insert(cmd.end(), (uint8_t*)&input_n, (uint8_t*)&input_n + 4);
    cmd.insert(cmd.end(), (uint8_t*)f.input_blob, (uint8_t*)f.input_blob + f.input_size);
    
    if (!shm::write_msg(shm_.cmd, cmd.data(), cmd.size())) { err_="cmd write failed"; return false; }
    shm::signal(shm_.ctrl->ev_child_wake);
    
    std::vector<uint8_t> rsp;
    if (!shm::read_msg(shm_.rsp, rsp)) { err_="no response"; return false; }
    if (rsp.size() < 6 || rsp[0] != 3) { err_="update failed"; return false; }
    
    last_call_ms_ = *(float*)&rsp[2];
    return true;
  }
  bool render() override {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<uint8_t> cmd = {2}; // CmdRender
    if (!shm::write_msg(shm_.cmd, cmd.data(), cmd.size())) { err_="cmd write failed"; return false; }
    shm::signal(shm_.ctrl->ev_child_wake);
    
    std::vector<uint8_t> rsp;
    if (!shm::read_msg(shm_.rsp, rsp)) { err_="no response"; return false; }
    if (rsp.size() < 6 || rsp[0] != 3) { err_="render failed"; return false; }
    
    last_call_ms_ = *(float*)&rsp[2];
    return true;
  }
  void unload() override {
    shm::destroy_host(shm_);
    proc::kill_child(child_);
  }
  const char* last_error() const override { return err_.c_str(); }
  double last_call_ms() const { return last_call_ms_; }
};

std::unique_ptr<IPluginRunner> make_runner_child() {
  return std::unique_ptr<IPluginRunner>(new RunnerChild());
}
