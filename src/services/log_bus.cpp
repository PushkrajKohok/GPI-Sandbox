#include "log_bus.h"

void LogBus::push(LogLvl lvl, const std::string& s) {
  std::lock_guard<std::mutex> lk(m_);
  if ((int)q_.size() >= cap_) q_.pop_front();
  q_.push_back(LogMsg{lvl, s});
}
void LogBus::snapshot(std::deque<LogMsg>& out, int max) {
  std::lock_guard<std::mutex> lk(m_);
  out.clear();
  int n = std::min<int>((int)q_.size(), max);
  auto it = q_.end(); for (int i=0;i<n;++i) --it;
  out.insert(out.end(), it, q_.end());
}
