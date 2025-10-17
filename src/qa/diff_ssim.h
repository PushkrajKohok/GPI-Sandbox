#pragma once
#include "diff.h"
#include <string>
#include <vector>

namespace qa {

// SSIM (Structural Similarity Index) comparison
struct SSIMResult {
  double ssim = 0.0;     // 0..1 (1 = identical)
  double mssim = 0.0;    // Mean SSIM across image
  int w = 0, h = 0;
};

// Compute SSIM between two images
bool diff_ssim(const std::string& expected, const std::string& actual, SSIMResult& out);

// Generate a heatmap showing differences
struct Heatmap {
  std::vector<unsigned char> rgba_data;  // RGBA8, size = w*h*4
  int w = 0, h = 0;
};

// Generate visual difference heatmap (red = high error)
bool generate_heatmap(const std::string& expected, const std::string& actual, 
                      const std::string& out_path, Heatmap& out);

} // namespace qa

