#pragma once
#include <string>
bool save_screenshot_png(const std::string& path, int w, int h);
bool save_frame_sequence_png(const std::string& dir, int frame_idx, int w, int h);
