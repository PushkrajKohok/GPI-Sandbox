#pragma once
#include <string>
#include <vector>

namespace fsu {
std::string join(const std::string& a, const std::string& b);
bool is_regular_file(const std::string& path);
std::vector<std::string> list_shared_libs(const std::string& dir);
}
