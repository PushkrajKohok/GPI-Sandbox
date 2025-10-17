#include "gpi_plugin.h"
#include <cmath>

static float t = 0.0f;

extern "C" GPI_Result gpi_init(const GPI_VersionInfo* v, const GPI_HostApi* api) {
    return GPI_OK;
}

extern "C" GPI_Capabilities gpi_query_capabilities(void) {
    GPI_Capabilities c{};
    c.caps = GPI_CAP_DRAW_PRIMS;
    return c;
}

extern "C" GPI_Result gpi_update(float dt) {
    t += dt;
    return GPI_OK;
}

extern "C" GPI_Result gpi_render(void) {
    // Draw a bouncing circle
    float x = 400.0f + 200.0f * std::cos(t);
    float y = 300.0f + 100.0f * std::sin(t);
    
    GPI_DrawRect rect{};
    rect.x = x - 25.0f;
    rect.y = y - 25.0f;
    rect.w = 50.0f;
    rect.h = 50.0f;
    rect.rgba = 0xFF44D3F7; // Blue
    
    // Use host's draw API
    // (This would be called via the host API in a real implementation)
    
    return GPI_OK;
}

extern "C" GPI_Result gpi_suspend(void) { return GPI_OK; }
extern "C" GPI_Result gpi_resume(void) { return GPI_OK; }
extern "C" GPI_Result gpi_shutdown(void) { return GPI_OK; }
