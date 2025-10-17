#include "plugin_loader.h"
#include <cstdio>

#if defined(_WIN32) || defined(_WIN64)
  #include <windows.h>
  static std::string last_error() {
    DWORD err = GetLastError(); char* msg = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, err, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR)&msg, 0, NULL);
    std::string s = msg ? msg : ""; if (msg) LocalFree(msg); return s;
  }
  LoadedLibrary PluginLoader::open(const std::string& path) {
    HMODULE h = LoadLibraryA(path.c_str());
    if (!h) std::fprintf(stderr, "LoadLibrary failed: %s\n", last_error().c_str());
    return { (void*)h, path };
  }
  void PluginLoader::close(LoadedLibrary& lib) {
    if (lib.handle) { FreeLibrary((HMODULE)lib.handle); lib.handle = nullptr; }
  }
  void* PluginLoader::sym(LoadedLibrary lib, const char* symbol) {
    if (!lib.handle) return nullptr;
    return (void*)GetProcAddress((HMODULE)lib.handle, symbol);
  }
#else
  #include <dlfcn.h>
  LoadedLibrary PluginLoader::open(const std::string& path) {
    void* h = dlopen(path.c_str(), RTLD_NOW);
    if (!h) std::fprintf(stderr, "dlopen failed: %s\n", dlerror());
    return { h, path };
  }
  void PluginLoader::close(LoadedLibrary& lib) {
    if (lib.handle) { dlclose(lib.handle); lib.handle = nullptr; }
  }
  void* PluginLoader::sym(LoadedLibrary lib, const char* symbol) {
    if (!lib.handle) return nullptr;
    return dlsym(lib.handle, symbol);
  }
#endif
