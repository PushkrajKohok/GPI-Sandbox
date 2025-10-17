#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct ReplayFrame {
  uint64_t t_ns;
  int32_t  fb_w, fb_h;
  float    dt_sec;
  std::vector<uint8_t> input;
};

class Recorder {
public:
  void start(const std::string& path);
  void add(const ReplayFrame& f);
  void stop();
  bool active() const { return active_; }
  const std::string& path() const { return path_; }
private:
  bool active_{false};
  std::string path_;
  std::vector<uint8_t> buf_;
};

class Replayer {
public:
  bool load(const std::string& path);
  bool next(ReplayFrame& out);
  void reset();
  bool active() const { return loaded_; }
private:
  bool loaded_{false};
  std::vector<uint8_t> buf_;
  size_t off_{0};
};
