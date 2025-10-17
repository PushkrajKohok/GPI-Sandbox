#include "replay.h"
#include <fstream>
#include <cstring>

static void put_u32(std::vector<uint8_t>& b, uint32_t v) {
  for(int i=0;i<4;i++) b.push_back((v>>(i*8))&0xFF);
}
static void put_u64(std::vector<uint8_t>& b, uint64_t v) {
  for(int i=0;i<8;i++) b.push_back((v>>(i*8))&0xFF);
}
static uint32_t rd_u32(const uint8_t* p) {
  return p[0]|(p[1]<<8)|(p[2]<<16)|(p[3]<<24);
}
static uint64_t rd_u64(const uint8_t* p) {
  uint64_t r=0; for(int i=0;i<8;i++) r|=((uint64_t)p[i])<<(8*i); return r;
}

void Recorder::start(const std::string& p) {
  path_=p; buf_.clear(); active_=true; 
  put_u32(buf_, 0x50524C59); // "YLRP"
}
void Recorder::add(const ReplayFrame& f) {
  if(!active_) return;
  put_u64(buf_, f.t_ns);
  put_u32(buf_, (uint32_t)f.fb_w); put_u32(buf_, (uint32_t)f.fb_h);
  union { float f; uint32_t u; } dt{f.dt_sec}; put_u32(buf_, dt.u);
  put_u32(buf_, (uint32_t)f.input.size()); 
  buf_.insert(buf_.end(), f.input.begin(), f.input.end());
}
void Recorder::stop() {
  if(!active_) return;
  std::ofstream o(path_, std::ios::binary|std::ios::trunc);
  if(o) o.write((const char*)buf_.data(), (std::streamsize)buf_.size());
  active_ = false;
}

bool Replayer::load(const std::string& p) {
  std::ifstream i(p, std::ios::binary);
  if(!i) return false; 
  buf_.assign((std::istreambuf_iterator<char>(i)), std::istreambuf_iterator<char>());
  if(buf_.size()<4 || rd_u32(buf_.data())!=0x50524C59) { buf_.clear(); return false; }
  off_ = 4; loaded_ = true; return true;
}
bool Replayer::next(ReplayFrame& out) {
  if(!loaded_) return false;
  if(off_+8+4+4+4 > buf_.size()) return false;
  const uint8_t* p = buf_.data()+off_;
  out.t_ns = rd_u64(p); p+=8;
  out.fb_w = (int32_t)rd_u32(p); p+=4;
  out.fb_h = (int32_t)rd_u32(p); p+=4;
  union { float f; uint32_t u; } dt; dt.u = rd_u32(p); p+=4;
  out.dt_sec = dt.f;
  if(p+4 > buf_.data()+buf_.size()) return false;
  uint32_t n = rd_u32(p); p+=4;
  if(p+n > buf_.data()+buf_.size()) return false;
  out.input.assign(p, p+n);
  off_ = (p - buf_.data()) + n;
  return true;
}
void Replayer::reset() { loaded_=false; buf_.clear(); off_=0; }
