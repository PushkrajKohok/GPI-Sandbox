#pragma once
#include <string>
#include <unordered_map>

namespace l10n {

// Localization system
class L10n {
 public:
  explicit L10n(const std::string& lang = "en");
  
  // Get localized string
  const char* get(const std::string& key) const;
  
  // Operator overload for convenience
  const char* operator[](const std::string& key) const { return get(key); }
  
  // Load translations from file
  bool load(const std::string& path);
  
  // Get current language
  const std::string& lang() const { return lang_; }
  
 private:
  std::string lang_;
  std::unordered_map<std::string, std::string> strings_;
};

// Global instance
extern L10n g_l10n;

// Helper macro
#define TR(key) l10n::g_l10n[key]

} // namespace l10n

