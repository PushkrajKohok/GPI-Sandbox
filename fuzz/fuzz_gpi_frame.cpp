#include <cstdint>
#include <cstddef>
#include <cstring>
extern "C" {
#include "gpi/gpi_plugin.h"
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < sizeof(GPI_InputV1)) return 0;
  GPI_InputV1 in{}; std::memcpy(&in, data, sizeof(GPI_InputV1));
  GPI_FrameContext ctx{};
  ctx.dt_sec = 0.016f; ctx.fb_width=1280; ctx.fb_height=720; ctx.time_ns=123456789;
  ctx.input_blob = &in; ctx.input_size = sizeof(in); ctx.input_version = GPI_INPUT_VERSION;
  return 0;
}
