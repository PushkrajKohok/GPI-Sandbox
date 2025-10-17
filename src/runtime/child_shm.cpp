#include "child_shm.h"
#include <cstring>
#include <random>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
// eventfd not available on macOS, use pipe instead
#endif

namespace shm {

#ifdef _WIN32
bool create_host(Block& out, const std::string& name, uint32_t cmd_bytes, uint32_t rsp_bytes) {
  std::string shm_name = "gpi_" + name;
  HANDLE h = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 
                                sizeof(Ctrl) + cmd_bytes + rsp_bytes, shm_name.c_str());
  if (!h) return false;
  
  void* base = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (!base) { CloseHandle(h); return false; }
  
  Ctrl* ctrl = (Ctrl*)base;
  ctrl->magic = MAGIC;
  ctrl->version = 1;
  ctrl->cmd_off = sizeof(Ctrl);
  ctrl->rsp_off = sizeof(Ctrl) + cmd_bytes;
  ctrl->ev_host_wake = (uint32_t)(intptr_t)CreateEventA(NULL, FALSE, FALSE, (shm_name + "_host").c_str());
  ctrl->ev_child_wake = (uint32_t)(intptr_t)CreateEventA(NULL, FALSE, FALSE, (shm_name + "_child").c_str());
  
  Ring* cmd = (Ring*)((char*)base + ctrl->cmd_off);
  cmd->cap = cmd_bytes - sizeof(Ring);
  cmd->head.store(0);
  cmd->tail.store(0);
  
  Ring* rsp = (Ring*)((char*)base + ctrl->rsp_off);
  rsp->cap = rsp_bytes - sizeof(Ring);
  rsp->head.store(0);
  rsp->tail.store(0);
  
  out.ctrl = ctrl;
  out.cmd = cmd;
  out.rsp = rsp;
  out.base = base;
  out.size = sizeof(Ctrl) + cmd_bytes + rsp_bytes;
  return true;
}

bool open_child(Block& out, const std::string& name) {
  std::string shm_name = "gpi_" + name;
  HANDLE h = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shm_name.c_str());
  if (!h) return false;
  
  void* base = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (!base) { CloseHandle(h); return false; }
  
  Ctrl* ctrl = (Ctrl*)base;
  if (ctrl->magic != MAGIC) { UnmapViewOfFile(base); CloseHandle(h); return false; }
  
  out.ctrl = ctrl;
  out.cmd = (Ring*)((char*)base + ctrl->cmd_off);
  out.rsp = (Ring*)((char*)base + ctrl->rsp_off);
  out.base = base;
  return true;
}

void destroy_host(Block& b) {
  if (b.ctrl) {
    CloseHandle((HANDLE)(intptr_t)b.ctrl->ev_host_wake);
    CloseHandle((HANDLE)(intptr_t)b.ctrl->ev_child_wake);
  }
  if (b.base) UnmapViewOfFile(b.base);
}

void close_child(Block& b) {
  if (b.base) UnmapViewOfFile(b.base);
}

void signal(uint32_t handle) {
  SetEvent((HANDLE)(intptr_t)handle);
}

void wait(uint32_t handle, uint32_t timeout_ms) {
  WaitForSingleObject((HANDLE)(intptr_t)handle, timeout_ms);
}
#else
bool create_host(Block& out, const std::string& name, uint32_t cmd_bytes, uint32_t rsp_bytes) {
  std::string shm_name = "/gpi_" + name;
  int fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
  if (fd < 0) return false;
  
  size_t size = sizeof(Ctrl) + cmd_bytes + rsp_bytes;
  if (ftruncate(fd, size) < 0) { close(fd); return false; }
  
  void* base = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);
  if (base == MAP_FAILED) return false;
  
  Ctrl* ctrl = (Ctrl*)base;
  ctrl->magic = MAGIC;
  ctrl->version = 1;
  ctrl->cmd_off = sizeof(Ctrl);
  ctrl->rsp_off = sizeof(Ctrl) + cmd_bytes;
  int pipes[2];
  pipe(pipes);
  ctrl->ev_host_wake = pipes[0];
  ctrl->ev_child_wake = pipes[1];
  
  Ring* cmd = (Ring*)((char*)base + ctrl->cmd_off);
  cmd->cap = cmd_bytes - sizeof(Ring);
  cmd->head.store(0);
  cmd->tail.store(0);
  
  Ring* rsp = (Ring*)((char*)base + ctrl->rsp_off);
  rsp->cap = rsp_bytes - sizeof(Ring);
  rsp->head.store(0);
  rsp->tail.store(0);
  
  out.ctrl = ctrl;
  out.cmd = cmd;
  out.rsp = rsp;
  out.base = base;
  out.size = size;
  return true;
}

bool open_child(Block& out, const std::string& name) {
  std::string shm_name = "/gpi_" + name;
  int fd = shm_open(shm_name.c_str(), O_RDWR, 0666);
  if (fd < 0) return false;
  
  void* base = mmap(nullptr, sizeof(Ctrl), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);
  if (base == MAP_FAILED) return false;
  
  Ctrl* ctrl = (Ctrl*)base;
  if (ctrl->magic != MAGIC) { munmap(base, sizeof(Ctrl)); return false; }
  
  size_t size = sizeof(Ctrl) + (ctrl->rsp_off - ctrl->cmd_off) + 1024;
  munmap(base, sizeof(Ctrl));
  base = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (base == MAP_FAILED) return false;
  
  out.ctrl = ctrl;
  out.cmd = (Ring*)((char*)base + ctrl->cmd_off);
  out.rsp = (Ring*)((char*)base + ctrl->rsp_off);
  out.base = base;
  return true;
}

void destroy_host(Block& b) {
  if (b.ctrl) {
    close(b.ctrl->ev_host_wake);
    close(b.ctrl->ev_child_wake);
  }
  if (b.base) munmap(b.base, b.size);
}

void close_child(Block& b) {
  if (b.base) munmap(b.base, b.size);
}

void signal(uint32_t handle) {
  char val = 1;
  write(handle, &val, 1);
}

void wait(uint32_t handle, uint32_t timeout_ms) {
  char val;
  read(handle, &val, 1);
}
#endif

bool ring_push(Ring* r, const void* blob, uint32_t n) {
  uint32_t head = r->head.load(std::memory_order_relaxed);
  uint32_t tail = r->tail.load(std::memory_order_acquire);
  uint32_t avail = r->cap - (head - tail);
  if (avail < n + 4) return false;
  
  uint8_t* data = r->data + (head & (r->cap - 1));
  *(uint32_t*)data = n;
  memcpy(data + 4, blob, n);
  r->head.store(head + n + 4, std::memory_order_release);
  return true;
}

bool ring_pop(Ring* r, void* blob, uint32_t n) {
  uint32_t tail = r->tail.load(std::memory_order_relaxed);
  uint32_t head = r->head.load(std::memory_order_acquire);
  if (head == tail) return false;
  
  uint8_t* data = r->data + (tail & (r->cap - 1));
  uint32_t len = *(uint32_t*)data;
  if (len > n) return false;
  
  memcpy(blob, data + 4, len);
  r->tail.store(tail + len + 4, std::memory_order_release);
  return true;
}

bool write_msg(Ring* r, const void* data, uint32_t n) {
  return ring_push(r, data, n);
}

bool read_msg(Ring* r, std::vector<uint8_t>& out) {
  uint32_t tail = r->tail.load(std::memory_order_relaxed);
  uint32_t head = r->head.load(std::memory_order_acquire);
  if (head == tail) return false;
  
  uint8_t* data = r->data + (tail & (r->cap - 1));
  uint32_t len = *(uint32_t*)data;
  out.resize(len);
  memcpy(out.data(), data + 4, len);
  r->tail.store(tail + len + 4, std::memory_order_release);
  return true;
}
}
