#include "sandbox.h"
#ifdef _WIN32
#include <windows.h>
#include <userenv.h>
#include <aclapi.h>
#pragma comment(lib, "userenv.lib")

static bool set_job_limits(HANDLE job, const SandboxSpec& s) {
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION li{};
  li.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_MEMORY | JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
  li.ProcessMemoryLimit = SIZE_T(s.max_mem_mb) * 1024 * 1024;
  li.BasicLimitInformation.ActiveProcessLimit = 1;
  return SetInformationJobObject(job, JobObjectExtendedLimitInformation, &li, sizeof(li)) != 0;
}

void sandbox::tighten_dll_search_paths() {
  SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
  SetDllDirectoryW(L"");
}

void sandbox::scrub_environment() {
  SetEnvironmentVariableA("PATH", "C:\\Windows\\System32");
  const char* kill[] = {"__COMPAT_LAYER","COMSPEC","TMP","TEMP","PATHEXT","PROMPT","PSModulePath",
                        "PYTHONPATH","RUSTFLAGS","LDFLAGS","LD_PRELOAD","LD_LIBRARY_PATH"};
  for (auto k: kill) SetEnvironmentVariableA(k, NULL);
}

bool sandbox::enter(const SandboxSpec& spec, std::string& err) {
  HANDLE job = CreateJobObjectW(nullptr, nullptr);
  if (!job) { err = "CreateJobObject failed"; return false; }
  if (!set_job_limits(job, spec)) { err = "SetInformationJobObject failed"; return false; }
  if (!AssignProcessToJobObject(job, GetCurrentProcess())) { err = "AssignProcessToJobObject failed"; return false; }

  JOBOBJECT_BASIC_UI_RESTRICTIONS ui{};
  ui.UIRestrictionsClass = JOB_OBJECT_UILIMIT_HANDLES | JOB_OBJECT_UILIMIT_DESKTOP;
  SetInformationJobObject(job, JobObjectBasicUIRestrictions, &ui, sizeof(ui));

  if (!SetCurrentDirectoryA(spec.work_dir.c_str())) {
    err = "SetCurrentDirectory failed"; return false;
  }
  return true;
}

bool sandbox::install_seccomp_default(bool, std::string&) { return true; }
#endif
