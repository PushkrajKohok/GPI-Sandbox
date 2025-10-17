#include "runner.h"
#include "plugin_runtime.h"

class RunnerInproc : public IPluginRunner {
  PluginRuntime rt_;
  std::string err_;
public:
  RunnerInproc(const GPI_HostApi& api, double deadline_ms) {
    rt_.deadline_ms = deadline_ms; 
    rt_.host_api = api; 
  }
  bool load(const std::string& lib) override {
    if (!rt_.load(lib, rt_.deadline_ms, rt_.host_api)) { 
      err_ = rt_.last_error ? rt_.last_error : "load failed"; 
      return false; 
    }
    return true;
  }
  bool update(const FrameArgs& f) override {
    GPI_FrameContext ctx{ f.dt_sec, f.fb_w, f.fb_h, f.t_ns, f.input_blob, f.input_size, f.input_version };
    if (!rt_.call_update(ctx)) { err_ = "update failed/stalled"; return false; }
    return true;
  }
  bool render() override {
    if (!rt_.call_render()) { err_ = "render failed/stalled"; return false; }
    return true;
  }
  void unload() override { rt_.unload(); }
  const char* last_error() const override { return err_.c_str(); }
};

std::unique_ptr<IPluginRunner> make_runner_inproc(const GPI_HostApi& api, double deadline_ms) {
  return std::unique_ptr<IPluginRunner>(new RunnerInproc(api, deadline_ms));
}
