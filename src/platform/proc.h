#pragma once
#include <string>
#include <vector>

struct ChildProc {
  void* handle=nullptr;
  int   in_fd=-1;
  int   out_fd=-1;
};

namespace proc {
  bool spawn_child(const std::string& exe, const std::vector<std::string>& args, ChildProc& out);
  void kill_child(ChildProc& p);
  bool write_line(const ChildProc& p, const std::string& line);
  bool read_line(const ChildProc& p, std::string& line);
}
