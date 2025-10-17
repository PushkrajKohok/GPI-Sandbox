#pragma once
#include <mutex>
#include <deque>
#include <string>

enum class LogLvl { Info, Warn, Error };

struct LogMsg { LogLvl lvl; std::string text; };

class LogBus {
public:
  void push(LogLvl lvl, const std::string& s);
  void snapshot(std::deque<LogMsg>& out, int max=500);
private:
  std::mutex m_;
  std::deque<LogMsg> q_;
  int cap_ = 2000;
};
