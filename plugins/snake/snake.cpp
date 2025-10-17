#include <deque>
#include <random>
#include <cstring>
#include <cstdio>
#include "gpi/gpi_plugin.h"

static const GPI_HostApi* G=nullptr;
static int fbw=1280, fbh=720, cell=20, cols=64, rows=36;
static std::deque<std::pair<int,int>> snake;
static int dirx=1, diry=0, foodx=20, foody=10;
static double acc=0.0;
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

static void spawn_food() {
  foodx = 2 + (rand() % (cols-4));
  foody = 2 + (rand() % (rows-4));
}
extern "C" GPI_Result gpi_init(const GPI_VersionInfo* v, const GPI_HostApi* api){
  if (!v || v->abi_version != GPI_ABI_VERSION) return GPI_ERR_BAD_VERSION;
  G=api; if (G->get_drawlist_v1){ G->get_drawlist_v1(&DL, &DL_bytes); }
  if (G->get_drawlist_v15){ G->get_drawlist_v15(&DL15, &DL15_bytes); }
  if (G->get_font_metrics_v15){ G->get_font_metrics_v15(&font_ascent,&font_descent,&font_gap,&font_ref); }
  snake.clear(); snake.push_front({5,5}); spawn_food(); return GPI_OK;
}
extern "C" GPI_Capabilities gpi_query_capabilities(void){ GPI_Capabilities c{}; c.caps=GPI_CAP_DRAW_PRIMS | GPI_CAP_DRAWLIST_V1 | GPI_CAP_DRAWLIST_V15; return c; }
extern "C" GPI_Result gpi_update(const GPI_FrameContext* ctx){
  fbw=ctx->fb_width; fbh=ctx->fb_height; cols=fbw/cell; rows=fbh/cell;
  acc += ctx->dt_sec;
  if (ctx->input_version==GPI_INPUT_VERSION && ctx->input_blob){
    const GPI_InputV1* in=(const GPI_InputV1*)ctx->input_blob;
    int cx = in->mouse_x / cell, cy = in->mouse_y / cell;
    auto [hx,hy] = snake.front();
    if (cx>hx) { dirx=1;  diry=0; }
    if (cx<hx) { dirx=-1; diry=0; }
    if (cy>hy) { dirx=0;  diry=1; }
    if (cy<hy) { dirx=0;  diry=-1;}
  }
  if (acc>=0.09) {
    acc=0.0;
    auto head = snake.front();
    head.first += dirx; head.second += diry;
    if (head.first<0) head.first=cols-1; if (head.second<0) head.second=rows-1;
    if (head.first>=cols) head.first=0; if (head.second>=rows) head.second=0;
    for (auto& p: snake) if (p==head) { snake.clear(); snake.push_front({5,5}); spawn_food(); return GPI_OK; }
    snake.push_front(head);
    if (head.first==foodx && head.second==foody) { if(G->telemetry_mark) G->telemetry_mark("snake.eat",1); spawn_food(); }
    else snake.pop_back();
  }
  return GPI_OK;
}
extern "C" GPI_Result gpi_render(void){
  if (DL15 && DL15->magic==GPI_DL15_MAGIC) {
    // Reset counts
    DL15->quad_count = 0; DL15->text_count = 0; DL15->utf8_size = 0;
    
    // Quads
    auto* quads = (GPI_QuadV1*)((uint8_t*)DL15 + sizeof(GPI_DrawListV15));
    for (auto& p: snake) {
      if (DL15->quad_count < DL15->max_quads) 
        quads[DL15->quad_count++] = GPI_QuadV1{ (float)(p.first*cell), (float)(p.second*cell), (float)cell-1, (float)cell-1, 0xFF7CF77Cu };
    }
    if (DL15->quad_count < DL15->max_quads) 
      quads[DL15->quad_count++] = GPI_QuadV1{ (float)(foodx*cell), (float)(foody*cell), (float)cell-1, (float)cell-1, 0xFFF77C7Cu };
    
    // Text
    add_text(DL15, fbw*0.5f, 22.0f, 20.0f, 0xFFEFEFEFu, GPI_TALIGN_CENTER, "SNAKE");
    char length[32]; std::snprintf(length, sizeof(length), "Length: %zu", snake.size());
    add_text(DL15, 16.0f, 18.0f, 18.0f, 0xFFB0F7B0u, GPI_TALIGN_LEFT, length);
  } else if (DL && DL->magic==GPI_DL_MAGIC) {
    uint32_t cap = DL->max_quads;
    DL->quad_count = 0;
    auto pushq = [&](int cx,int cy,unsigned c){
      if (DL->quad_count < cap) DL->quads[DL->quad_count++] = GPI_QuadV1{ (float)(cx*cell), (float)(cy*cell), (float)cell-1, (float)cell-1, c };
    };
    for (auto& p: snake) pushq(p.first, p.second, 0xFF7CF77Cu);
    pushq(foodx, foody, 0xFFF77C7Cu);
  } else if (G && G->draw_rects) {
    const int maxr = (int)snake.size()+1;
    static GPI_DrawRect rects[2048];
    int n=0;
    auto add = [&](int cx,int cy,unsigned c){
      if (n<2048) rects[n++] = GPI_DrawRect{ (float)(cx*cell), (float)(cy*cell), (float)cell-1, (float)cell-1, c };
    };
    for (auto& p: snake) add(p.first, p.second, 0xFF7CF77Cu);
    add(foodx, foody, 0xFFF77C7Cu);
    G->draw_rects(rects, n);
  }
  return GPI_OK;
}
extern "C" void gpi_suspend(void) {}
extern "C" void gpi_resume(void) {}
extern "C" void gpi_shutdown(void) {}
