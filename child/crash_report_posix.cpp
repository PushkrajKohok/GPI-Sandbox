#ifndef _WIN32
#include "crash_report.h"
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string>

static std::string g_dir;

static void write_line(int fd, const char* s) { write(fd, s, strlen(s)); }

static void on_sig(int sig) {
  time_t t = time(nullptr); struct tm tmv; localtime_r(&t, &tmv);
  char path[256];
  std::snprintf(path, sizeof(path), "%s/crash_%04d%02d%02d_%02d%02d%02d.txt",
                g_dir.c_str(), tmv.tm_year+1900, tmv.tm_mon+1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
  int fd = open(path, O_CREAT|O_WRONLY|O_TRUNC, 0600);
  if (fd >= 0) {
    void* bt[64]; int n = backtrace(bt, 64);
    write_line(fd, "Signal: "); char buf[32]; std::snprintf(buf, sizeof(buf), "%d\n", sig); write_line(fd, buf);
    backtrace_symbols_fd(bt, n, fd);
    close(fd);
  }
  _exit(128 + sig);
}

bool install_crash_handler(const std::string& dir) {
  g_dir = dir;
  signal(SIGSEGV, on_sig); signal(SIGABRT, on_sig); signal(SIGFPE, on_sig); signal(SIGILL, on_sig);
  return true;
}
#endif
