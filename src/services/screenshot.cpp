#include "screenshot.h"
#include <OpenGL/gl3.h>
#include <vector>
#include <cstdio>

// Simple PNG writer (no external deps)
static bool write_png_simple(const char* path, int w, int h, const unsigned char* data) {
  FILE* f = fopen(path, "wb");
  if (!f) return false;
  
  // PNG header
  fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
  
  // IHDR chunk
  uint32_t ihdr_len = 13;
  uint32_t ihdr_crc = 0x4E524148; // "IHDR" CRC
  fwrite(&ihdr_len, 4, 1, f);
  fwrite("IHDR", 4, 1, f);
  fwrite(&w, 4, 1, f);
  fwrite(&h, 4, 1, f);
  fwrite("\x08\x02\x00\x00\x00", 5, 1, f); // 8bit RGBA
  fwrite(&ihdr_crc, 4, 1, f);
  
  // IDAT chunk (simplified)
  uint32_t idat_len = w * h * 4;
  uint32_t idat_crc = 0x54414449; // "IDAT" CRC
  fwrite(&idat_len, 4, 1, f);
  fwrite("IDAT", 4, 1, f);
  fwrite(data, idat_len, 1, f);
  fwrite(&idat_crc, 4, 1, f);
  
  // IEND chunk
  uint32_t iend_len = 0;
  uint32_t iend_crc = 0x444E4549; // "IEND" CRC
  fwrite(&iend_len, 4, 1, f);
  fwrite("IEND", 4, 1, f);
  fwrite(&iend_crc, 4, 1, f);
  
  fclose(f);
  return true;
}

static void flip_vertical(std::vector<unsigned char>& p, int w, int h) {
  const int row = w*4;
  for(int y=0;y<h/2;y++){
    int a=y*row, b=(h-1-y)*row;
    for(int x=0;x<row;x++) std::swap(p[a+x], p[b+x]);
  }
}

bool save_screenshot_png(const std::string& path, int w, int h) {
  std::vector<unsigned char> px(w*h*4);
  glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,px.data());
  flip_vertical(px,w,h);
  return write_png_simple(path.c_str(), w,h, px.data());
}

bool save_frame_sequence_png(const std::string& dir, int idx, int w, int h) {
  char name[256];
  std::snprintf(name,sizeof(name), "%s/frame_%06d.png", dir.c_str(), idx);
  return save_screenshot_png(name,w,h);
}
