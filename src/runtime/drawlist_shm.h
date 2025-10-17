#pragma once
#include <string>
#include <cstdint>

struct DrawListMap {
  void* base = nullptr;
  uint32_t bytes = 0;
};

bool dl_create_host(DrawListMap& m, const std::string& name, uint32_t bytes);
bool dl_open_child(DrawListMap& m, const std::string& name);
void dl_close(DrawListMap& m);
