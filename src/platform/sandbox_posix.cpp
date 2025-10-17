#include "sandbox.h"
#ifndef _WIN32
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

void sandbox::tighten_dll_search_paths() {
  unsetenv("LD_PRELOAD");
  unsetenv("LD_LIBRARY_PATH");
  unsetenv("DYLD_INSERT_LIBRARIES");
  unsetenv("DYLD_FALLBACK_LIBRARY_PATH");
}

void sandbox::scrub_environment() {
  setenv("PATH", "/usr/bin:/bin", 1);
  const char* kill[] = {"LD_PRELOAD","LD_LIBRARY_PATH","DYLD_INSERT_LIBRARIES","PYTHONPATH","GEM_PATH",
                        "RUSTFLAGS","LDFLAGS","CPPFLAGS","CFLAGS","CXXFLAGS"};
  for (auto k: kill) unsetenv(k);
}

static bool set_rlimit(int res, rlim_t soft, rlim_t hard) {
  struct rlimit rl{soft, hard};
  return setrlimit(res, &rl) == 0;
}

bool sandbox::enter(const SandboxSpec& spec, std::string& err) {
  umask(0077);
  if (chdir(spec.work_dir.c_str()) != 0) {
    err = std::string("chdir failed: ") + strerror(errno);
    return false;
  }
  set_rlimit(RLIMIT_NOFILE, spec.max_open_files, spec.max_open_files);
#ifdef RLIMIT_AS
  set_rlimit(RLIMIT_AS, spec.max_mem_mb*1024*1024, spec.max_mem_mb*1024*1024);
#endif
  set_rlimit(RLIMIT_CPU, spec.cpu_time_sec, spec.cpu_time_sec);
#ifdef RLIMIT_CORE
  set_rlimit(RLIMIT_CORE, 0, 0);
#endif
  setgroups(0, nullptr);
  if (geteuid() == 0) {
    setgid(65534); setuid(65534);
  }
  return true;
}
#endif
