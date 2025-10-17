#include "telemetry.h"
#include <fstream>

void Telemetry::mark(const std::string& key, double value) {
  auto now = std::chrono::steady_clock::now();
  double t = std::chrono::duration<double>(now - t0_).count();
  std::lock_guard<std::mutex> lk(m_);
  series_[key].push_back({t, value});
}

void Telemetry::flush_csv(const std::string& path) {
  std::lock_guard<std::mutex> lk(m_);
  std::ofstream f(path, std::ios::trunc);
  if (!f) return;
  f << "key,time,value\n";
  for (auto& [k, vec] : series_)
    for (auto& [t, v] : vec) f << k << "," << t << "," << v << "\n";
}

void Telemetry::flush_json(const std::string& path) {
  std::lock_guard<std::mutex> lk(m_);
  std::ofstream f(path, std::ios::trunc);
  if (!f) return;
  f << "{\n";
  bool firstk = true;
  for (auto& [k, vec] : series_) {
    if (!firstk) f << ",\n"; firstk = false;
    f << "  \"" << k << "\": [";
    for (size_t i=0;i<vec.size();++i) {
      auto [t,v]=vec[i];
      f << (i? ", ":" ") << "[" << t << "," << v << "]";
    }
    f << " ]";
  }
  f << "\n}\n";
}
