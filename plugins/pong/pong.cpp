#include <cmath>
#include <cstring>
#include <cstdio>
#include "gpi/gpi_plugin.h"

static const GPI_HostApi* G = nullptr;
static float px=40, py=260, bx=320, by=200, vx=220, vy=180;
static int fbw=1280, fbh=720;
static GPI_DrawListV1* DL=nullptr; static uint32_t DL_bytes=0;
static GPI_DrawListV15* DL15=nullptr; static uint32_t DL15_bytes=0;
static float font_ascent=0, font_descent=0, font_gap=0, font_ref=0;

// Simple text helper
static void add_text(GPI_DrawListV15* dl, float x, float y, float size, unsigned color, uint32_t align, const char* s) {
  if (!s || dl->text_count >= dl->max_text) return;
  size_t len = strlen(s);
  if (dl->utf8_size + len > dl->utf8_capacity) return;
  
  uint32_t off = dl->utf8_size;
  memcpy((char*)((uint8_t*)dl + sizeof(GPI_DrawListV15) + dl->max_quads*sizeof(GPI_QuadV1) + dl->max_text*sizeof(GPI_TextRunV15)) + off, s, len);
  dl->utf8_size += (uint32_t)len;
  
  auto* runs = (GPI_TextRunV15*)((uint8_t*)dl + sizeof(GPI_DrawListV15) + dl->max_quads*sizeof(GPI_QuadV1));
  runs[dl->text_count++] = GPI_TextRunV15{ x, y, size, color, align, off, (uint32_t)len };
}

extern "C" GPI_Result gpi_init(const GPI_VersionInfo* v, const GPI_HostApi* api) {
  if (!v || v->abi_version != GPI_ABI_VERSION) return GPI_ERR_BAD_VERSION;
  G = api; if (G->log_info) G->log_info("pong: init");
  if (G->get_drawlist_v1){ G->get_drawlist_v1(&DL, &DL_bytes); }
  if (G->get_drawlist_v15){ G->get_drawlist_v15(&DL15, &DL15_bytes); }
  if (G->get_font_metrics_v15){ G->get_font_metrics_v15(&font_ascent,&font_descent,&font_gap,&font_ref); }
  return GPI_OK;
}
extern "C" GPI_Capabilities gpi_query_capabilities(void) {
  GPI_Capabilities c{}; c.caps = GPI_CAP_DRAW_PRIMS | GPI_CAP_PAUSE_RESUME | GPI_CAP_DRAWLIST_V1 | GPI_CAP_DRAWLIST_V15; return c;
}
extern "C" GPI_Result gpi_update(const GPI_FrameContext* ctx) {
  fbw = ctx->fb_width; fbh = ctx->fb_height;
  float dt = ctx->dt_sec;
  if (ctx->input_version==GPI_INPUT_VERSION && ctx->input_blob) {
    const GPI_InputV1* in=(const GPI_InputV1*)ctx->input_blob;
    py = (float)in->mouse_y - 40.0f;
    if (py < 0) py = 0; if (py > fbh-80) py = (float)(fbh-80);
  }
  bx += vx*dt; by += vy*dt;
  if (by<0){ by=0; vy=-vy; } if (by>fbh-20){ by=(float)(fbh-20); vy=-vy; }
  if (bx<px+12 && by+20>py && by<py+80){ bx=px+12; vx=std::abs(vx); if(G->telemetry_mark) G->telemetry_mark("pong.hit",1); }
  if (bx<0 || bx>fbw-20){ bx=fbw*0.5f; by=fbh*0.5f; vx=-vx; }
  return GPI_OK;
}
extern "C" GPI_Result gpi_render(void) {
  if (DL15 && DL15->magic==GPI_DL15_MAGIC) {
    // Reset counts
    DL15->quad_count = 0; DL15->text_count = 0; DL15->utf8_size = 0;
    
    // Quads
    auto* quads = (GPI_QuadV1*)((uint8_t*)DL15 + sizeof(GPI_DrawListV15));
    if (DL15->quad_count < DL15->max_quads) quads[DL15->quad_count++] = GPI_QuadV1{ px, py, 12, 80, 0xFF44D3F7u };
    if (DL15->quad_count < DL15->max_quads) quads[DL15->quad_count++] = GPI_QuadV1{ (float)(fbw-24), by-40, 12, 80, 0xFF86F76Bu };
    if (DL15->quad_count < DL15->max_quads) quads[DL15->quad_count++] = GPI_QuadV1{ bx, by, 20, 20, 0xFFF7C244u };
    
    // Text
    add_text(DL15, fbw*0.5f, 22.0f, 20.0f, 0xFFEFEFEFu, GPI_TALIGN_CENTER, "GPI Pong");
    char score[32]; std::snprintf(score, sizeof(score), "Score: %d", (int)(by) % 20);
    add_text(DL15, 16.0f, 18.0f, 18.0f, 0xFFB0F7B0u, GPI_TALIGN_LEFT, score);
  } else if (DL && DL->magic==GPI_DL_MAGIC) {
    uint32_t cap = DL->max_quads;
    DL->quad_count = 0;
    auto pushq = [&](float x,float y,float w,float h,unsigned c){
      if (DL->quad_count < cap) DL->quads[DL->quad_count++] = GPI_QuadV1{ x,y,w,h,c };
    };
    pushq(px, py, 12, 80, 0xFF44D3F7u);
    pushq((float)(fbw-24), by-40, 12, 80, 0xFF86F76Bu);
    pushq(bx, by, 20, 20, 0xFFF7C244u);
  } else if (G && G->draw_rects) {
    GPI_DrawRect r[3];
    r[0] = { px, py, 12, 80, 0xFF44D3F7u };
    r[1] = { (float)(fbw-24), by-40, 12, 80, 0xFF86F76Bu };
    r[2] = { bx, by, 20, 20, 0xFFF7C244u };
    G->draw_rects(r, 3);
  }
  return GPI_OK;
}
extern "C" void gpi_suspend(void) {}
extern "C" void gpi_resume(void)  {}
extern "C" void gpi_shutdown(void){ if(G&&G->log_info) G->log_info("pong: bye"); }
