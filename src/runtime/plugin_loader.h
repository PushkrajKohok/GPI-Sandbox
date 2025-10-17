#pragma once
#include <string>
#include <cstdint>

struct LoadedLibrary {
  void* handle = nullptr;
  std::string path;
};

class PluginLoader {
public:
  static LoadedLibrary open(const std::string& path);
  static void close(LoadedLibrary& lib);
  static void* sym(LoadedLibrary lib, const char* symbol);
};
