#include "proc.h"
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>

namespace proc {
bool spawn_child(const std::string& exe, const std::vector<std::string>& args, ChildProc& out) {
  HANDLE hInRead, hInWrite, hOutRead, hOutWrite;
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
  
  if (!CreatePipe(&hInRead, &hInWrite, &sa, 0) || !CreatePipe(&hOutRead, &hOutWrite, &sa, 0)) {
    return false;
  }
  
  STARTUPINFOA si = {sizeof(STARTUPINFOA)};
  si.hStdInput = hInRead;
  si.hStdOutput = hOutWrite;
  si.hStdError = hOutWrite;
  si.dwFlags = STARTF_USESTDHANDLES;
  
  PROCESS_INFORMATION pi;
  if (!CreateProcessA(exe.c_str(), NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
    CloseHandle(hInRead); CloseHandle(hInWrite);
    CloseHandle(hOutRead); CloseHandle(hOutWrite);
    return false;
  }
  
  CloseHandle(hInRead); CloseHandle(hOutWrite);
  out.handle = pi.hProcess;
  out.in_fd = _open_osfhandle((intptr_t)hInWrite, _O_WRONLY);
  out.out_fd = _open_osfhandle((intptr_t)hOutRead, _O_RDONLY);
  return true;
}

void kill_child(ChildProc& p) {
  if (p.handle) { TerminateProcess((HANDLE)p.handle, 1); CloseHandle((HANDLE)p.handle); }
  if (p.in_fd >= 0) { _close(p.in_fd); }
  if (p.out_fd >= 0) { _close(p.out_fd); }
  p = ChildProc{};
}

bool write_line(const ChildProc& p, const std::string& line) {
  if (p.in_fd < 0) return false;
  return _write(p.in_fd, line.c_str(), (unsigned)line.size()) > 0;
}

bool read_line(const ChildProc& p, std::string& line) {
  if (p.out_fd < 0) return false;
  char buf[4096];
  int n = _read(p.out_fd, buf, sizeof(buf)-1);
  if (n <= 0) return false;
  buf[n] = 0;
  line = buf;
  return true;
}
}
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <signal.h>

namespace proc {
bool spawn_child(const std::string& exe, const std::vector<std::string>& args, ChildProc& out) {
  int in[2], out_pipes[2];
  if (pipe(in) || pipe(out_pipes)) return false;
  
  pid_t pid = fork();
  if (pid == 0) {
    dup2(in[0], STDIN_FILENO);  dup2(out_pipes[1], STDOUT_FILENO);
    close(in[0]); close(in[1]); close(out_pipes[0]); close(out_pipes[1]);
    execl(exe.c_str(), exe.c_str(), nullptr);
    _exit(1);
  }
  if (pid < 0) { close(in[0]); close(in[1]); close(out_pipes[0]); close(out_pipes[1]); return false; }
  
  close(in[0]); close(out_pipes[1]);
  out.handle = (void*)(intptr_t)pid;
  out.in_fd = in[1]; out.out_fd = out_pipes[0];
  return true;
}

void kill_child(ChildProc& p) {
  if (p.handle) { kill((pid_t)(intptr_t)p.handle, SIGTERM); waitpid((pid_t)(intptr_t)p.handle, nullptr, 0); }
  if (p.in_fd >= 0) { close(p.in_fd); }
  if (p.out_fd >= 0) { close(p.out_fd); }
  p = ChildProc{};
}

bool write_line(const ChildProc& p, const std::string& line) {
  if (p.in_fd < 0) return false;
  return write(p.in_fd, line.c_str(), line.size()) > 0;
}

bool read_line(const ChildProc& p, std::string& line) {
  if (p.out_fd < 0) return false;
  char buf[4096];
  int n = read(p.out_fd, buf, sizeof(buf)-1);
  if (n <= 0) return false;
  buf[n] = 0;
  line = buf;
  return true;
}
}
#endif
