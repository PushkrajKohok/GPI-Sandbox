#include "artifacts.h"
#include <fstream>

bool artifacts::write_frame_csv(const std::string& path, const std::vector<double>& frame_ms) {
  std::ofstream f(path, std::ios::trunc);
  if (!f) return false;
  f << "frame,ms\n";
  for (size_t i=0;i<frame_ms.size();++i) f << i << "," << frame_ms[i] << "\n";
  return true;
}

static std::string json_escape(const std::string& s){
  std::string o; o.reserve(s.size()+8);
  for (char c: s) { if (c=='"'||c=='\\') { o.push_back('\\'); o.push_back(c); } else if (c=='\n'){o+="\\n";} else o.push_back(c);}
  return o;
}

bool artifacts::write_session_json(const std::string& path, const SessionInfo& info, const FrameSummary& sum){
  std::ofstream f(path, std::ios::trunc);
  if (!f) return false;
  f << "{\n";
  f << "  \"app_version\": \"" << json_escape(info.app_version) << "\",\n";
  f << "  \"os\": \"" << json_escape(info.os) << "\",\n";
  f << "  \"plugin\": \"" << json_escape(info.plugin) << "\",\n";
  f << "  \"target_fps\": " << info.target_fps << ",\n";
  f << "  \"avg_ms\": " << sum.avg_ms << ",\n";
  f << "  \"p95_ms\": " << sum.p95_ms << ",\n";
  f << "  \"p99_ms\": " << sum.p99_ms << ",\n";
  f << "  \"dropped_pct\": " << sum.dropped_pct << ",\n";
  f << "  \"total_frames\": " << sum.total_frames << "\n";
  f << "}\n";
  return true;
}
