#pragma once
#include <string>
#include <vector>

struct PluginMeta {
  std::string path;
  std::string name;
  std::string version;
  std::string summary;
  std::string icon;
};

std::vector<PluginMeta> scan_with_metadata(const std::string& dir);
bool draw_store_panel(std::vector<PluginMeta>& metas, int& selected_index, bool& load_clicked);
