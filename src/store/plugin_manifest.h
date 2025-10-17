#pragma once
#include <string>
#include <vector>

namespace store {

// Plugin manifest structure (loaded from plugin.manifest.json)
struct PluginManifest {
  std::string id;           // unique identifier (reverse-DNS style)
  std::string name;         // display name
  std::string version;      // semantic version (e.g., "1.0.0")
  std::string author;       // author name
  std::string description;  // short description
  std::string lib_path;     // relative path to .so/.dll/.dylib
  std::string icon_path;    // optional: icon file path
  std::vector<std::string> tags;  // categories/tags
  
  // Signing info
  std::string signature;    // hex-encoded SHA256 signature
  std::string signer;       // public key identifier or cert fingerprint
  
  // Requirements
  std::string min_host_version;  // minimum compatible host version
  std::vector<std::string> dependencies;  // other plugin IDs required
};

// Load manifest from JSON file
bool load_manifest(const std::string& path, PluginManifest& out);

// Verify plugin signature against trusted keys
bool verify_signature(const PluginManifest& manifest, const std::string& trust_file);

// Store operations
struct StoreEntry {
  PluginManifest manifest;
  std::string install_path;  // where it's installed
  bool installed = false;
  bool enabled = true;
};

class PluginStore {
 public:
  explicit PluginStore(const std::string& store_dir);
  
  // Scan installed plugins
  void scan();
  
  // Get all entries
  const std::vector<StoreEntry>& entries() const { return entries_; }
  
  // Install a plugin from a manifest file
  bool install(const std::string& manifest_path);
  
  // Uninstall a plugin by ID
  bool uninstall(const std::string& id);
  
  // Enable/disable a plugin
  void set_enabled(const std::string& id, bool enabled);
  
  // Upgrade a plugin to a new version
  bool upgrade(const std::string& id, const std::string& new_manifest_path);
  
  // Rollback to previous version (if backup exists)
  bool rollback(const std::string& id);
  
 private:
  std::string store_dir_;
  std::vector<StoreEntry> entries_;
};

} // namespace store

