#pragma once
#include <atomic>
#include <thread>
#include <functional>
#include <chrono>

class Watchdog {
public:
  using Clock = std::chrono::steady_clock;
  using Ms = std::chrono::milliseconds;

  bool start(std::atomic<bool>* running_flag,
             std::atomic<bool>* in_call_flag,
             std::atomic<Clock::time_point>* call_start_tp,
             double stall_ms,
             std::function<void()> on_stall);
  void stop();

private:
  std::thread th_;
  std::atomic<bool> alive_{false};
};
