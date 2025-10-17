#pragma once
#include <string>

struct Settings {
  std::string plugins_dir = "./plugins/bin";
  double deadline_ms = 12.0;
  double stall_ms = 150.0;
  std::string last_plugin = "";
  bool ui_hud = true;
  bool isolation = true;
  bool show_store = true;
  std::string last_record = "";
  std::string last_replay = "";
};

namespace cfg {
  bool load_from_file(const std::string& path, Settings& out);
  bool save_to_file(const std::string& path, const Settings& in);
}
