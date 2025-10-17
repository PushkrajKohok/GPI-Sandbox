#pragma once
#include <mutex>
#include <unordered_map>
#include <string>
#include <vector>
#include <chrono>

class Telemetry {
public:
  void mark(const std::string& key, double value);
  void flush_csv(const std::string& path);
  void flush_json(const std::string& path);
private:
  std::mutex m_;
  std::unordered_map<std::string, std::vector<std::pair<double,double>>> series_;
  std::chrono::steady_clock::time_point t0_ = std::chrono::steady_clock::now();
};
