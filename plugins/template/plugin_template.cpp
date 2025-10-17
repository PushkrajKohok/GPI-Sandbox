#include <cstdio>
#include <cmath>
extern "C" {
#include "gpi/gpi_plugin.h"
}

static const GPI_HostApi* G = nullptr;

extern "C" GPI_Result gpi_init(const GPI_VersionInfo* ver, const GPI_HostApi* api) {
  if (!ver || !api) return GPI_ERR_BAD_ARGUMENT;
  if (ver->abi_version != GPI_ABI_VERSION) return GPI_ERR_BAD_VERSION;
  G = api;
  if (G && G->log_info) G->log_info("plugin_template: init OK");
  return GPI_OK;
}

extern "C" GPI_Capabilities gpi_query_capabilities(void) {
  GPI_Capabilities c{};
  c.caps = (GPI_CAP_PAUSE_RESUME);
  return c;
}

extern "C" GPI_Result gpi_update(const GPI_FrameContext* ctx) {
  if (!ctx) return GPI_ERR_BAD_ARGUMENT;

  if (ctx->input_version == GPI_INPUT_VERSION && ctx->input_blob && ctx->input_size >= (int)sizeof(GPI_InputV1)) {
    const GPI_InputV1* in = (const GPI_InputV1*)ctx->input_blob;
    if (in->key_escape) {
      if (G && G->log_info) G->log_info("plugin_template: ESC pressed (from plugin)");
    }
  }

  if (G && G->telemetry_mark) {
    double phase = std::sin((double)(ctx->time_ns % 1'000'000'000ull) / 1.0e9 * 6.283185307179586);
    G->telemetry_mark("template.phase", phase);
  }
  return GPI_OK;
}

extern "C" GPI_Result gpi_render(void) {
  return GPI_OK;
}

extern "C" void gpi_suspend(void) {
  if (G && G->log_info) G->log_info("plugin_template: suspend");
}
extern "C" void gpi_resume(void) {
  if (G && G->log_info) G->log_info("plugin_template: resume");
}
extern "C" void gpi_shutdown(void) {
  if (G && G->log_info) G->log_info("plugin_template: shutdown");
  G = nullptr;
}

extern "C" void gpi_debug_crash(void) {
  volatile int* p = nullptr; *p = 42; // deliberate crash
}
