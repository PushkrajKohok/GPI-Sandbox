#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include "../../include/gpi/gpi_plugin.h"

struct FrameArgs {
  float dt_sec;
  int fb_w, fb_h;
  uint64_t t_ns;
  const void* input_blob;
  uint32_t input_size;
  uint32_t input_version;
};

class IPluginRunner {
public:
  virtual ~IPluginRunner() = default;
  virtual bool load(const std::string& lib_path) = 0;
  virtual bool update(const FrameArgs& f) = 0;
  virtual bool render() = 0;
  virtual void unload() = 0;
  virtual const char* last_error() const = 0;
};

std::unique_ptr<IPluginRunner> make_runner_inproc(const GPI_HostApi& api, double deadline_ms);
std::unique_ptr<IPluginRunner> make_runner_child();
