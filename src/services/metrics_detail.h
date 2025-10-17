#pragma once
#include <vector>
#include <algorithm>
#include <numeric>

struct CallStats { 
  double avg=0, p95=0, p99=0, last=0; 
};

class CallHistogram {
public:
  explicit CallHistogram(size_t cap=512) : cap_(cap) { v_.reserve(cap); }
  void push(double ms) { 
    if(v_.size()==cap_) v_.erase(v_.begin()); 
    v_.push_back(ms); 
    last_ = ms; 
  }
  CallStats stats() const {
    CallStats s{};
    if (v_.empty()) return s;
    std::vector<double> t(v_);
    std::sort(t.begin(), t.end());
    auto n=t.size();
    s.avg = std::accumulate(t.begin(), t.end(), 0.0) / (double)n;
    s.p95 = t[(size_t)(0.95*(n-1))];
    s.p99 = t[(size_t)(0.99*(n-1))];
    s.last = last_;
    return s;
  }
  const std::vector<double>& samples() const { return v_; }
private:
  size_t cap_;
  std::vector<double> v_;
  double last_=0;
};
