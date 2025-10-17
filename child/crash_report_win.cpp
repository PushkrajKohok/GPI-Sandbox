#ifdef _WIN32
#include "crash_report.h"
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

static std::string g_dir;
static LONG WINAPI handler(EXCEPTION_POINTERS* info) {
  SYSTEMTIME st; GetSystemTime(&st);
  char name[256];
  std::snprintf(name, sizeof(name), "%s\\crash_%04d%02d%02d_%02d%02d%02d.dmp",
    g_dir.c_str(), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

  HANDLE hFile = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile != INVALID_HANDLE_VALUE) {
    MINIDUMP_EXCEPTION_INFORMATION mei;
    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = info;
    mei.ClientPointers = FALSE;
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithIndirectlyReferencedMemory, &mei, NULL, NULL);
    CloseHandle(hFile);
  }
  return EXCEPTION_EXECUTE_HANDLER;
}

bool install_crash_handler(const std::string& dir) {
  g_dir = dir;
  SetUnhandledExceptionFilter(handler);
  return true;
}
#endif
