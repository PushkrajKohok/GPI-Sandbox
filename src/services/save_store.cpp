#include "save_store.h"
#include <sys/stat.h>
#include <fstream>
#include <filesystem>

#if defined(_WIN32)
  #include <windows.h>
  static void fsync_file(std::ofstream&){}
#else
  #include <unistd.h>
  #include <fcntl.h>
  static void fsync_fd(int fd){ if (fd>=0) ::fsync(fd); }
#endif

namespace fs = std::filesystem;

static fs::path base_dir() { return fs::path("userdata"); }
static fs::path plug_dir(const std::string& p) { return base_dir() / p; }

static bool ensure_dir(const fs::path& p) {
  std::error_code ec; fs::create_directories(p, ec); return !ec;
}

bool save::put(const std::string& plugin, const std::string& key,
               const void* data, int size) {
  if (!data || size<=0) return false;
  auto dir = plug_dir(plugin);
  if (!ensure_dir(dir)) return false;

  fs::path tmp = dir / (key + ".tmp");
  fs::path bin = dir / (key + ".bin");
  fs::path jnl = dir / "journal.log";

  {
    std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write((const char*)data, size);
    f.flush();
#if !defined(_WIN32)
    int fd = ::open(tmp.string().c_str(), O_RDONLY);
    fsync_fd(fd); if (fd>=0) ::close(fd);
#endif
  }
  std::error_code ec;
  fs::rename(tmp, bin, ec);
  if (ec) return false;

  std::ofstream jf(jnl, std::ios::app);
  if (jf) jf << "PUT " << key << " size=" << size << "\n";

  return true;
}

int save::get(const std::string& plugin, const std::string& key,
              void* out, int capacity) {
  auto path = plug_dir(plugin) / (key + ".bin");
  std::ifstream f(path, std::ios::binary);
  if (!f) return -1;
  f.read((char*)out, capacity);
  return (int)f.gcount();
}
