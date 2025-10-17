#pragma once
#include <string>
#include <deque>
#include <chrono>

struct Toast {
  std::string text;
  float seconds = 2.0f;
  std::chrono::steady_clock::time_point start;
  enum Type { Info, Warn, Error } type = Info;
};

class Toasts {
public:
  void info(const std::string& s, float sec=2.0f);
  void warn(const std::string& s, float sec=3.0f);
  void error(const std::string& s, float sec=4.0f);
  void render();
private:
  std::deque<Toast> q_;
};
