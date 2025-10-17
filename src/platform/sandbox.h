#pragma once
#include <string>
#include <vector>

struct SandboxSpec {
  std::string work_dir;
  bool        deny_network = true;
  size_t      max_mem_mb = 512;
  size_t      max_open_files = 128;
  size_t      cpu_time_sec = 120;
};

namespace sandbox {
bool enter(const SandboxSpec& spec, std::string& err);
void scrub_environment();
bool install_seccomp_default(bool deny_network, std::string& err);
void tighten_dll_search_paths();
}
