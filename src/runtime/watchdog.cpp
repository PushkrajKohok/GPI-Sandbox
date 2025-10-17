#include "watchdog.h"

bool Watchdog::start(std::atomic<bool>* running,
                     std::atomic<bool>* in_call,
                     std::atomic<Clock::time_point>* call_start_tp,
                     double stall_ms,
                     std::function<void()> on_stall) {
  if (alive_.exchange(true)) return false;
  th_ = std::thread([=]() {
    const auto budget = Ms((int)stall_ms);
    while (alive_.load(std::memory_order_relaxed) && running->load(std::memory_order_relaxed)) {
      if (in_call->load(std::memory_order_relaxed)) {
        auto since = Clock::now() - call_start_tp->load(std::memory_order_relaxed);
        if (since > budget) {
          on_stall();
        }
      }
      std::this_thread::sleep_for(Ms(10));
    }
  });
  return true;
}

void Watchdog::stop() {
  if (!alive_.exchange(false)) return;
  if (th_.joinable()) th_.join();
}
