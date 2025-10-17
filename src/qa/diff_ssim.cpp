#include "diff_ssim.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../../thirdparty/stb_image.h"
// Note: stb_image_write already defined in screenshot.cpp
#include "../../thirdparty/stb_image_write.h"
#include <cmath>
#include <algorithm>

namespace qa {

// Simple SSIM implementation (single-channel luminance)
static double compute_ssim_window(const unsigned char* img1, const unsigned char* img2, 
                                   int w, int h, int x, int y, int window_size) {
  const double C1 = 6.5025, C2 = 58.5225;  // Constants for stability
  
  double mean1 = 0, mean2 = 0, var1 = 0, var2 = 0, covar = 0;
  int count = 0;
  
  for (int dy = 0; dy < window_size; ++dy) {
    for (int dx = 0; dx < window_size; ++dx) {
      int px = x + dx, py = y + dy;
      if (px >= w || py >= h) continue;
      
      int idx = (py * w + px) * 4;  // RGBA
      double L1 = img1[idx] * 0.3 + img1[idx+1] * 0.59 + img1[idx+2] * 0.11;  // Luminance
      double L2 = img2[idx] * 0.3 + img2[idx+1] * 0.59 + img2[idx+2] * 0.11;
      
      mean1 += L1;
      mean2 += L2;
      ++count;
    }
  }
  
  if (count == 0) return 1.0;
  
  mean1 /= count;
  mean2 /= count;
  
  for (int dy = 0; dy < window_size; ++dy) {
    for (int dx = 0; dx < window_size; ++dx) {
      int px = x + dx, py = y + dy;
      if (px >= w || py >= h) continue;
      
      int idx = (py * w + px) * 4;
      double L1 = img1[idx] * 0.3 + img1[idx+1] * 0.59 + img1[idx+2] * 0.11;
      double L2 = img2[idx] * 0.3 + img2[idx+1] * 0.59 + img2[idx+2] * 0.11;
      
      var1 += (L1 - mean1) * (L1 - mean1);
      var2 += (L2 - mean2) * (L2 - mean2);
      covar += (L1 - mean1) * (L2 - mean2);
    }
  }
  
  var1 /= count;
  var2 /= count;
  covar /= count;
  
  double ssim = ((2 * mean1 * mean2 + C1) * (2 * covar + C2)) /
                ((mean1*mean1 + mean2*mean2 + C1) * (var1 + var2 + C2));
  
  return ssim;
}

bool diff_ssim(const std::string& expected, const std::string& actual, SSIMResult& out) {
  int w1, h1, c1, w2, h2, c2;
  unsigned char* img1 = stbi_load(expected.c_str(), &w1, &h1, &c1, 4);
  unsigned char* img2 = stbi_load(actual.c_str(), &w2, &h2, &c2, 4);
  
  if (!img1 || !img2 || w1 != w2 || h1 != h2) {
    if (img1) stbi_image_free(img1);
    if (img2) stbi_image_free(img2);
    return false;
  }
  
  out.w = w1;
  out.h = h1;
  
  const int window_size = 8;
  double total_ssim = 0.0;
  int window_count = 0;
  
  for (int y = 0; y < h1; y += window_size) {
    for (int x = 0; x < w1; x += window_size) {
      double ssim = compute_ssim_window(img1, img2, w1, h1, x, y, window_size);
      total_ssim += ssim;
      ++window_count;
    }
  }
  
  out.mssim = window_count > 0 ? total_ssim / window_count : 0.0;
  out.ssim = out.mssim;  // For now, same as MSSIM
  
  stbi_image_free(img1);
  stbi_image_free(img2);
  return true;
}

bool generate_heatmap(const std::string& expected, const std::string& actual,
                      const std::string& out_path, Heatmap& out) {
  int w1, h1, c1, w2, h2, c2;
  unsigned char* img1 = stbi_load(expected.c_str(), &w1, &h1, &c1, 4);
  unsigned char* img2 = stbi_load(actual.c_str(), &w2, &h2, &c2, 4);
  
  if (!img1 || !img2 || w1 != w2 || h1 != h2) {
    if (img1) stbi_image_free(img1);
    if (img2) stbi_image_free(img2);
    return false;
  }
  
  out.w = w1;
  out.h = h1;
  out.rgba_data.resize(w1 * h1 * 4);
  
  for (int i = 0; i < w1 * h1; ++i) {
    int r1 = img1[i*4+0], g1 = img1[i*4+1], b1 = img1[i*4+2];
    int r2 = img2[i*4+0], g2 = img2[i*4+1], b2 = img2[i*4+2];
    
    int dr = std::abs(r1 - r2);
    int dg = std::abs(g1 - g2);
    int db = std::abs(b1 - b2);
    int diff = (dr + dg + db) / 3;
    
    // Map diff to heatmap color (black -> yellow -> red)
    unsigned char r = std::min(255, diff * 2);
    unsigned char g = std::min(255, std::max(0, 255 - diff));
    unsigned char b = 0;
    
    out.rgba_data[i*4+0] = r;
    out.rgba_data[i*4+1] = g;
    out.rgba_data[i*4+2] = b;
    out.rgba_data[i*4+3] = 255;
  }
  
  // Write PNG
  stbi_write_png(out_path.c_str(), w1, h1, 4, out.rgba_data.data(), w1 * 4);
  
  stbi_image_free(img1);
  stbi_image_free(img2);
  return true;
}

} // namespace qa

