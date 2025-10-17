#pragma once
#include <string>
#include <vector>

struct SessionInfo {
  std::string app_version;
  std::string os;
  std::string plugin;
  double target_fps = 60.0;
};

struct FrameSummary {
  double avg_ms=0, p95_ms=0, p99_ms=0;
  double dropped_pct=0;
  int    total_frames=0;
};

namespace artifacts {
  bool write_frame_csv(const std::string& path, const std::vector<double>& frame_ms);
  bool write_session_json(const std::string& path, const SessionInfo& info, const FrameSummary& sum);
}
