#pragma once
#include <string>

namespace save {
bool put(const std::string& plugin, const std::string& key,
         const void* data, int size);
int  get(const std::string& plugin, const std::string& key,
         void* out, int capacity);
}
