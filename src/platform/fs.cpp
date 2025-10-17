#include "fs.h"
#include <sys/stat.h>
#include <string>
#include <vector>

#if defined(_WIN32)
  #include <windows.h>
#else
  #include <dirent.h>
#endif

namespace fsu {
std::string join(const std::string& a, const std::string& b) {
  if (a.empty()) return b;
  if (a.back() == '/' || a.back() == '\\') return a + b;
#if defined(_WIN32)
  return a + "\\" + b;
#else
  return a + "/" + b;
#endif
}

bool is_regular_file(const std::string& path) {
  struct stat st{};
  if (stat(path.c_str(), &st) != 0) return false;
  return (st.st_mode & S_IFREG) != 0;
}

static bool has_shared_ext(const std::string& name) {
#if defined(_WIN32)
  return name.size() > 4 && name.substr(name.size()-4) == ".dll";
#elif defined(__APPLE__)
  return name.size() > 6 && name.substr(name.size()-6) == ".dylib";
#else
  return name.size() > 3 && name.substr(name.size()-3) == ".so";
#endif
}

std::vector<std::string> list_shared_libs(const std::string& dir) {
  std::vector<std::string> out;
#if defined(_WIN32)
  std::string pattern = join(dir, "*.*");
  WIN32_FIND_DATAA fd; HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
  if (h == INVALID_HANDLE_VALUE) return out;
  do {
    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      std::string name(fd.cFileName);
      if (has_shared_ext(name)) out.push_back(join(dir, name));
    }
  } while (FindNextFileA(h, &fd));
  FindClose(h);
#else
  DIR* d = opendir(dir.c_str());
  if (!d) return out;
  while (auto* de = readdir(d)) {
    std::string name = de->d_name;
    if (name == "." || name == "..") continue;
    if (has_shared_ext(name)) {
      std::string p = join(dir, name);
      if (is_regular_file(p)) out.push_back(p);
    }
  }
  closedir(d);
#endif
  return out;
}
}
