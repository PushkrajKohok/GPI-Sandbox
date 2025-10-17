#pragma once
extern "C" { 
#include "../../include/gpi/gpi_plugin.h" 
}
struct HostFont;
void render_drawlist_v1(const GPI_DrawListV1* dl);
void render_drawlist_v15(const GPI_DrawListV15* dl, const HostFont& font);
