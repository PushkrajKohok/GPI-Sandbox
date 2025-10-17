#pragma once
#include <string>

struct DiffResult {
  double rmse = 0.0;
  double pct_over_threshold = 0.0; // %
  int    w = 0, h = 0;
};

bool diff_png_vs_png(const std::string& expected, const std::string& actual,
                     int abs_threshold, DiffResult& out);
