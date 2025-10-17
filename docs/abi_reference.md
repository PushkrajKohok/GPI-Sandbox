# GPI Plugin ABI Reference

## Overview

The GPI Plugin ABI is a C-compatible interface that allows plugins to communicate with the host. It provides a stable, versioned API for plugin development.

## Version Information

```c
typedef struct {
  uint32_t abi_version;  // 0x00010000 for v1.0
  uint32_t host_version; // Host's version
} GPI_VersionInfo;
```

## Plugin Entry Points

### Initialization
```c
GPI_Result gpi_init(const GPI_VersionInfo* ver, const GPI_HostApi* api);
```
Called once when the plugin is loaded. Returns `GPI_OK` on success.

### Capabilities
```c
GPI_Capabilities gpi_query_capabilities(void);
```
Returns the plugin's capabilities. Must be implemented.

### Update Loop
```c
GPI_Result gpi_update(const GPI_FrameContext* ctx);
```
Called every frame with timing and input data.

### Rendering
```c
GPI_Result gpi_render(void);
```
Called every frame for rendering. Use draw list for zero-copy rendering.

### Lifecycle
```c
void gpi_suspend(void);   // Plugin paused
void gpi_resume(void);    // Plugin resumed
void gpi_shutdown(void);  // Plugin unloaded
```

## Host API

### Logging
```c
typedef void (*GPI_LogFn)(const char* msg);
GPI_LogFn log_info;
GPI_LogFn log_warn;
GPI_LogFn log_error;
GPI_LogFn log_debug;
```

### Telemetry
```c
typedef void (*GPI_TelemetryFn)(const char* key, double value);
GPI_TelemetryFn telemetry_mark;
```

### Save Store
```c
typedef bool (*GPI_SavePutFn)(const char* key, const void* data, uint32_t size);
typedef bool (*GPI_SaveGetFn)(const char* key, void* data, uint32_t* size);
GPI_SavePutFn save_put;
GPI_SaveGetFn save_get;
```

### Rendering APIs

#### Legacy Draw Primitives
```c
typedef struct {
  float x, y, w, h;
  unsigned int rgba;
} GPI_DrawRect;

typedef void (*GPI_DrawRectFn)(const GPI_DrawRect* rects, uint32_t count);
GPI_DrawRectFn draw_rects;
```

#### Zero-Copy Draw List v1
```c
typedef struct {
  float x, y, w, h;
  unsigned int rgba;
} GPI_QuadV1;

typedef struct {
  uint32_t magic;         // GPI_DL_MAGIC
  uint32_t version;       // GPI_DL_VERSION
  uint32_t max_quads;     // capacity
  uint32_t quad_count;    // written by plugin
  GPI_QuadV1 quads[1];    // variable length
} GPI_DrawListV1;

typedef void (*GPI_GetDrawListFn)(GPI_DrawListV1** out_ptr, uint32_t* out_bytes);
GPI_GetDrawListFn get_drawlist_v1;
```

#### Zero-Copy Draw List v1.5 (Text Support)
```c
typedef enum {
  GPI_TALIGN_LEFT   = 0,
  GPI_TALIGN_CENTER = 1,
  GPI_TALIGN_RIGHT  = 2
} GPI_TextAlign;

typedef struct {
  float x, y;           // baseline origin
  float size_px;        // font height
  unsigned int rgba;     // text color
  uint32_t align;       // GPI_TextAlign
  uint32_t utf8_off;    // offset into utf8_blob
  uint32_t utf8_len;    // bytes
} GPI_TextRunV15;

typedef struct {
  uint32_t magic;         // GPI_DL15_MAGIC
  uint32_t version;       // GPI_DL15_VERSION
  uint32_t max_quads;     // capacity
  uint32_t quad_count;    // written by plugin
  uint32_t max_text;      // capacity (runs)
  uint32_t text_count;    // written by plugin
  uint32_t utf8_capacity; // bytes available
  uint32_t utf8_size;     // bytes written
} GPI_DrawListV15;

typedef void (*GPI_GetDrawListV15Fn)(GPI_DrawListV15** out_ptr, uint32_t* out_bytes);
typedef void (*GPI_GetFontMetricsFn)(float* ascent, float* descent, float* line_gap, float* atlas_px_height);
GPI_GetDrawListV15Fn get_drawlist_v15;
GPI_GetFontMetricsFn get_font_metrics_v15;
```

#### Image Support (v1)
```c
typedef uint32_t GPI_ImageHandle;

typedef struct {
  float x, y, w, h;     // screen-space dest rect
  float u0, v0, u1, v1; // texture UVs (0..1)
  GPI_ImageHandle tex;  // texture handle
  unsigned int rgba;    // tint color
} GPI_SpriteV1;

typedef GPI_ImageHandle (*GPI_UploadImageFn)(const void* rgba_data, int w, int h);
typedef void (*GPI_FreeImageFn)(GPI_ImageHandle handle);
GPI_UploadImageFn upload_image;
GPI_FreeImageFn free_image;
```

## Capabilities

```c
typedef enum {
  GPI_CAP_NONE          = 0,
  GPI_CAP_PAUSE_RESUME  = 1 << 0,
  GPI_CAP_GAMEPAD       = 1 << 1,
  GPI_CAP_DRAW_PRIMS    = 1 << 2,
  GPI_CAP_DRAWLIST_V1   = 1 << 3,
  GPI_CAP_DRAWLIST_V15  = 1 << 4,
  GPI_CAP_IMAGES_V1     = 1 << 5
} GPI_CapabilityFlags;

typedef struct {
  uint32_t caps;
} GPI_Capabilities;
```

## Input Handling

```c
typedef struct {
  uint32_t input_version;
  float dt;                    // delta time
  int32_t w, h;               // window size
  uint64_t frame_time;         // microseconds
  uint32_t input_n;           // input data size
  // Input data follows...
} GPI_FrameContext;
```

## Error Codes

```c
typedef enum {
  GPI_OK            = 0,
  GPI_ERR_BAD_ARGUMENT = -1,
  GPI_ERR_INIT_FAILED  = -2,
  GPI_ERR_RUNTIME     = -3
} GPI_Result;
```

## Best Practices

### Performance
- Use draw list v1.5 for zero-copy rendering
- Minimize allocations in update/render loops
- Use telemetry to profile performance

### Safety
- Check API function pointers before use
- Validate input data
- Handle errors gracefully

### Compatibility
- Query capabilities before using features
- Support multiple ABI versions
- Test with different host versions

## Example Plugin

```c
#include "gpi_plugin.h"

static GPI_DrawListV15* dl = nullptr;
static uint32_t dl_bytes = 0;

extern "C" GPI_Result gpi_init(const GPI_VersionInfo* v, const GPI_HostApi* api) {
    if (api->get_drawlist_v15) {
        api->get_drawlist_v15(&dl, &dl_bytes);
    }
    return GPI_OK;
}

extern "C" GPI_Capabilities gpi_query_capabilities(void) {
    GPI_Capabilities c{};
    c.caps = GPI_CAP_DRAWLIST_V15;
    return c;
}

extern "C" GPI_Result gpi_render(void) {
    if (dl && dl->magic == GPI_DL15_MAGIC) {
        // Reset counts
        dl->quad_count = 0;
        dl->text_count = 0;
        dl->utf8_size = 0;
        
        // Add quads and text runs...
    }
    return GPI_OK;
}
```
