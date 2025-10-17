#pragma once
#include <cstdint>
#include <string>
#include <atomic>
#include <vector>

namespace shm {
constexpr uint32_t MAGIC = 0x47504953; // "SPIG"
enum Phase : uint8_t { PH_UPDATE=1, PH_RENDER=2 };

struct alignas(64) Ring {
  uint32_t cap;
  std::atomic<uint32_t> head;
  std::atomic<uint32_t> tail;
  uint8_t data[1]; // Use fixed size for compatibility
};

struct Ctrl {
  uint32_t magic;
  uint32_t version;
  uint32_t cmd_off;
  uint32_t rsp_off;
  uint32_t ev_host_wake;
  uint32_t ev_child_wake;
};

struct Block {
  Ctrl* ctrl = nullptr;
  Ring* cmd = nullptr;
  Ring* rsp = nullptr;
  void* base = nullptr;
  size_t size = 0;
};

bool create_host(Block& out, const std::string& name, uint32_t cmd_bytes=1<<20, uint32_t rsp_bytes=1<<16);
bool open_child(Block& out, const std::string& name);
void destroy_host(Block& b);
void close_child(Block& b);

bool ring_push(Ring* r, const void* blob, uint32_t n);
bool ring_pop(Ring* r, void* blob, uint32_t n);

bool write_msg(Ring* r, const void* data, uint32_t n);
bool read_msg(Ring* r, std::vector<uint8_t>& out);

void signal(uint32_t handle);
void wait(uint32_t handle, uint32_t timeout_ms=1000);
}
