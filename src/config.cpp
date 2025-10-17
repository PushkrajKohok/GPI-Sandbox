#include "config.h"
#include <fstream>
#include <sstream>
#include <algorithm>

static std::string trim(std::string s) {
  auto is_space = [](unsigned char c){ return std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&](char c){ return !is_space(c);} ));
  s.erase(std::find_if(s.rbegin(), s.rend(), [&](char c){ return !is_space(c);} ).base(), s.end());
  return s;
}
static std::string lower(std::string s){ std::transform(s.begin(), s.end(), s.begin(), ::tolower); return s; }

bool cfg::load_from_file(const std::string& path, Settings& out) {
  std::ifstream f(path);
  if (!f) return false;
  std::string line, section;
  while (std::getline(f, line)) {
    auto pos_hash = line.find('#'); if (pos_hash != std::string::npos) line = line.substr(0, pos_hash);
    line = trim(line);
    if (line.empty()) continue;
    if (line.front()=='[' && line.back()==']') { section = lower(line.substr(1, line.size()-2)); continue; }
    auto eq = line.find('='); if (eq == std::string::npos) continue;
    auto key = trim(line.substr(0, eq));
    auto val = trim(line.substr(eq+1));
    if (!val.empty() && val.front()=='"' && val.back()=='"') val = val.substr(1, val.size()-2);

    if (section == "runtime") {
      if (key == "plugins_dir") out.plugins_dir = val;
      else if (key == "deadline_ms") out.deadline_ms = std::stod(val);
      else if (key == "stall_ms") out.stall_ms = std::stod(val);
      else if (key == "last_plugin") out.last_plugin = val;
      else if (key == "isolation") out.isolation = (lower(val)=="true" || val=="1");
    } else if (section == "ui") {
      if (key == "hud") out.ui_hud = (lower(val)=="true" || val=="1");
      else if (key == "show_store") out.show_store = (lower(val)=="true" || val=="1");
    } else if (section == "replay") {
      if (key == "last_record") out.last_record = val;
      else if (key == "last_replay") out.last_replay = val;
    }
  }
  return true;
}

bool cfg::save_to_file(const std::string& path, const Settings& in) {
  std::ofstream f(path, std::ios::trunc);
  if (!f) return false;
  f << "[runtime]\n";
  f << "plugins_dir = \"" << in.plugins_dir << "\"\n";
  f << "deadline_ms = " << in.deadline_ms << "\n";
  f << "stall_ms    = " << in.stall_ms << "\n";
  f << "last_plugin = \"" << in.last_plugin << "\"\n";
  f << "isolation   = " << (in.isolation ? "true" : "false") << "\n\n";
  f << "[ui]\n";
  f << "hud = " << (in.ui_hud ? "true" : "false") << "\n";
  f << "show_store = " << (in.show_store ? "true" : "false") << "\n\n";
  f << "[replay]\n";
  f << "last_record = \"" << in.last_record << "\"\n";
  f << "last_replay = \"" << in.last_replay << "\"\n";
  return true;
}
