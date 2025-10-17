#include "l10n.h"
#include <fstream>
#include <sstream>

namespace l10n {

L10n g_l10n;

L10n::L10n(const std::string& lang) : lang_(lang) {
  // Default English strings
  strings_["app.title"] = "GPI Sandbox";
  strings_["ui.hud"] = "HUD";
  strings_["ui.store"] = "Plugin Store";
  strings_["ui.settings"] = "Settings";
  strings_["plugin.load"] = "Load Plugin";
  strings_["plugin.unload"] = "Unload Plugin";
  strings_["plugin.reload"] = "Reload Plugin";
  strings_["error.load_failed"] = "Failed to load plugin";
  strings_["error.crash"] = "Plugin crashed";
}

const char* L10n::get(const std::string& key) const {
  auto it = strings_.find(key);
  if (it != strings_.end()) {
    return it->second.c_str();
  }
  return key.c_str();  // Fallback to key itself
}

bool L10n::load(const std::string& path) {
  std::ifstream f(path);
  if (!f.is_open()) return false;
  
  std::string line;
  while (std::getline(f, line)) {
    if (line.empty() || line[0] == '#') continue;
    
    size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    
    std::string key = line.substr(0, eq);
    std::string value = line.substr(eq + 1);
    
    // Trim whitespace
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);
    
    strings_[key] = value;
  }
  
  return true;
}

} // namespace l10n

