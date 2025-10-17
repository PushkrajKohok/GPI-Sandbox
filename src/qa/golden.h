#pragma once
#include <string>
#include <vector>

struct GoldenMeta {
  std::vector<int> check_frames;
  double rmse_tol = 6.0;
  double px_pct_tol = 0.4;
  int w = 1280, h = 720;
  std::string plugin_hint;
};

bool load_golden_meta(const std::string& folder, GoldenMeta& out);
bool verify_golden(const std::string& folder, const std::string& replay_path,
                   const GoldenMeta& meta, const std::string& out_dir,
                   std::string& report_json_path, bool& pass);
