#include "drawlist_shm.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <random>

#ifdef _WIN32
#include <windows.h>
#endif

bool dl_create_host(DrawListMap& m, const std::string& name, uint32_t bytes) {
#ifdef _WIN32
  HANDLE hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, bytes, name.c_str());
  if (!hMap) return false;
  m.base = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, bytes);
  if (!m.base) { CloseHandle(hMap); return false; }
  m.bytes = bytes;
  return true;
#else
  int fd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
  if (fd == -1) return false;
  ftruncate(fd, bytes);
  m.base = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);
  if (m.base == MAP_FAILED) return false;
  m.bytes = bytes;
  return true;
#endif
}

bool dl_open_child(DrawListMap& m, const std::string& name) {
#ifdef _WIN32
  HANDLE hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());
  if (!hMap) return false;
  m.base = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (!m.base) { CloseHandle(hMap); return false; }
  m.bytes = 0; // Size not needed for child
  return true;
#else
  int fd = shm_open(name.c_str(), O_RDWR, 0666);
  if (fd == -1) return false;
  struct stat st;
  fstat(fd, &st);
  m.bytes = st.st_size;
  m.base = mmap(nullptr, m.bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);
  if (m.base == MAP_FAILED) return false;
  return true;
#endif
}

void dl_close(DrawListMap& m) {
  if (m.base) {
#ifdef _WIN32
    UnmapViewOfFile(m.base);
#else
    munmap(m.base, m.bytes);
#endif
    m.base = nullptr;
  }
}
