#pragma once
#include <vector>
#include <algorithm>
#include <numeric>
#include <cstdint>

struct FrameStats {
  double fps = 0.0;
  double avg_ms = 0.0;
  double p95_ms = 0.0;
  double p99_ms = 0.0;
  double dropped_pct = 0.0;
};

class FrameHistogram {
public:
  explicit FrameHistogram(std::size_t cap = 600, double budget_ms = 16.6)
    : cap_(cap), budget_ms_(budget_ms) { samples_.reserve(cap_); }

  void push(double ms) {
    if (samples_.size() == cap_) samples_.erase(samples_.begin());
    samples_.push_back(ms);
    total_time_sec_ += ms / 1000.0;
    if (ms > budget_ms_) dropped_++;
  }

  FrameStats stats() const {
    FrameStats s{};
    if (samples_.empty()) return s;

    std::vector<double> tmp(samples_);
    std::sort(tmp.begin(), tmp.end());
    const auto n = tmp.size();

    const auto sum = std::accumulate(tmp.begin(), tmp.end(), 0.0);
    s.avg_ms = sum / static_cast<double>(n);
    s.p95_ms = tmp[static_cast<std::size_t>(0.95 * (n - 1))];
    s.p99_ms = tmp[static_cast<std::size_t>(0.99 * (n - 1))];

    s.fps = (s.avg_ms > 0.0) ? 1000.0 / s.avg_ms : 0.0;

    const double drop = (n > 0) ? (static_cast<double>(dropped_) / static_cast<double>(n)) : 0.0;
    s.dropped_pct = drop * 100.0;
    return s;
  }

  void reset() {
    samples_.clear();
    dropped_ = 0;
    total_time_sec_ = 0.0;
  }

  double budget_ms() const { return budget_ms_; }
  void set_budget_ms(double ms) { budget_ms_ = ms; }
  const std::vector<double>& samples() const { return samples_; }
  FrameStats stats_for_summary() const { return stats(); }

private:
  std::vector<double> samples_;
  std::size_t cap_;
  double budget_ms_;
  std::size_t dropped_ = 0;
  double total_time_sec_ = 0.0;
};
