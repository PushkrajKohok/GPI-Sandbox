#include "plugin_manifest.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace store {

// Simple JSON parser for manifest (minimal, no external deps)
static bool parse_manifest_json(const std::string& content, PluginManifest& out) {
  // Very basic parsing - in production, use a real JSON library
  auto get_str = [&](const std::string& key) -> std::string {
    std::string search = "\"" + key + "\":";
    size_t pos = content.find(search);
    if (pos == std::string::npos) return "";
    pos += search.length();
    while (pos < content.length() && (content[pos] == ' ' || content[pos] == '\"')) ++pos;
    size_t end = content.find('\"', pos);
    if (end == std::string::npos) return "";
    return content.substr(pos, end - pos);
  };
  
  out.id = get_str("id");
  out.name = get_str("name");
  out.version = get_str("version");
  out.author = get_str("author");
  out.description = get_str("description");
  out.lib_path = get_str("lib_path");
  out.icon_path = get_str("icon_path");
  out.signature = get_str("signature");
  out.signer = get_str("signer");
  out.min_host_version = get_str("min_host_version");
  
  return !out.id.empty() && !out.name.empty() && !out.version.empty();
}

bool load_manifest(const std::string& path, PluginManifest& out) {
  std::ifstream f(path);
  if (!f.is_open()) return false;
  
  std::stringstream buf;
  buf << f.rdbuf();
  std::string content = buf.str();
  
  return parse_manifest_json(content, out);
}

bool verify_signature(const PluginManifest& manifest, const std::string& trust_file) {
  // Stub: In production, verify SHA256 signature against public key
  // For now, just check if signature is present and trust file exists
  if (manifest.signature.empty()) return false;
  
  std::ifstream trust(trust_file);
  if (!trust.is_open()) return true; // No trust file = skip verification
  
  // TODO: Actual crypto verification using OpenSSL or similar
  return true;
}

PluginStore::PluginStore(const std::string& store_dir) : store_dir_(store_dir) {
  fs::create_directories(store_dir_);
}

void PluginStore::scan() {
  entries_.clear();
  
  if (!fs::exists(store_dir_)) return;
  
  for (const auto& entry : fs::directory_iterator(store_dir_)) {
    if (!entry.is_directory()) continue;
    
    std::string manifest_path = entry.path().string() + "/plugin.manifest.json";
    if (!fs::exists(manifest_path)) continue;
    
    PluginManifest manifest;
    if (!load_manifest(manifest_path, manifest)) continue;
    
    StoreEntry se;
    se.manifest = manifest;
    se.install_path = entry.path().string();
    se.installed = true;
    se.enabled = true; // TODO: Read from state file
    
    entries_.push_back(se);
  }
}

bool PluginStore::install(const std::string& manifest_path) {
  PluginManifest manifest;
  if (!load_manifest(manifest_path, manifest)) return false;
  
  // Check if already installed
  for (const auto& e : entries_) {
    if (e.manifest.id == manifest.id) {
      return false; // Already installed
    }
  }
  
  // Create install directory
  std::string install_dir = store_dir_ + "/" + manifest.id;
  if (!fs::create_directories(install_dir)) return false;
  
  // Copy manifest
  fs::path src_dir = fs::path(manifest_path).parent_path();
  fs::copy(manifest_path, install_dir + "/plugin.manifest.json");
  
  // Copy library file
  if (!manifest.lib_path.empty()) {
    std::string src_lib = src_dir.string() + "/" + manifest.lib_path;
    if (fs::exists(src_lib)) {
      fs::copy(src_lib, install_dir + "/" + fs::path(manifest.lib_path).filename().string());
    }
  }
  
  // Rescan
  scan();
  return true;
}

bool PluginStore::uninstall(const std::string& id) {
  for (auto it = entries_.begin(); it != entries_.end(); ++it) {
    if (it->manifest.id == id) {
      // Remove directory
      fs::remove_all(it->install_path);
      entries_.erase(it);
      return true;
    }
  }
  return false;
}

void PluginStore::set_enabled(const std::string& id, bool enabled) {
  for (auto& e : entries_) {
    if (e.manifest.id == id) {
      e.enabled = enabled;
      // TODO: Persist to state file
      break;
    }
  }
}

bool PluginStore::upgrade(const std::string& id, const std::string& new_manifest_path) {
  // Backup current version
  for (auto& e : entries_) {
    if (e.manifest.id == id) {
      std::string backup_dir = e.install_path + ".backup";
      fs::remove_all(backup_dir);
      fs::copy(e.install_path, backup_dir, fs::copy_options::recursive);
      
      // Uninstall current
      uninstall(id);
      
      // Install new
      return install(new_manifest_path);
    }
  }
  return false;
}

bool PluginStore::rollback(const std::string& id) {
  std::string install_dir = store_dir_ + "/" + id;
  std::string backup_dir = install_dir + ".backup";
  
  if (!fs::exists(backup_dir)) return false;
  
  // Remove current
  fs::remove_all(install_dir);
  
  // Restore backup
  fs::rename(backup_dir, install_dir);
  
  // Rescan
  scan();
  return true;
}

} // namespace store

