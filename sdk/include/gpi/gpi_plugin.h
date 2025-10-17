#ifndef GPI_PLUGIN_H
#define GPI_PLUGIN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPI_ABI_VERSION 0x00010000u

typedef struct {
  uint32_t abi_version;
  uint32_t impl_version;
} GPI_VersionInfo;

typedef enum {
  GPI_OK                = 0,
  GPI_ERR_BAD_VERSION   = -1,
  GPI_ERR_BAD_ARGUMENT  = -2,
  GPI_ERR_INIT_FAILED   = -3,
  GPI_ERR_RUNTIME       = -4
} GPI_Result;

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

#define GPI_INPUT_VERSION 0x00010000u

typedef struct {
  uint32_t input_version;
  uint8_t  key_escape;
  uint8_t  reserved_keys[7];
  int32_t  mouse_x;
  int32_t  mouse_y;
  uint8_t  mouse_left;
  uint8_t  mouse_right;
  uint8_t  mouse_middle;
  uint8_t  _pad0;
  uint8_t  gamepad_connected;
  int8_t   pad_axis_left_x;
  int8_t   pad_axis_left_y;
  uint8_t  pad_button_a;
  uint8_t  _pad1[3];
} GPI_InputV1;

typedef struct {
  float     dt_sec;
  int32_t   fb_width;
  int32_t   fb_height;
  uint64_t  time_ns;
  const void* input_blob;
  uint32_t  input_size;
  uint32_t  input_version;
} GPI_FrameContext;

typedef struct {
  float x, y, w, h;
  unsigned int rgba;
} GPI_DrawRect;

typedef void (*GPI_LogFn)(const char* msg);
typedef void (*GPI_TelemetryFn)(const char* key, double value);
typedef int  (*GPI_SavePutFn)(const char* key, const void* data, int32_t size);
typedef int  (*GPI_SaveGetFn)(const char* key, void* out, int32_t capacity);
typedef void (*GPI_DrawRectFn)(const GPI_DrawRect* r, int count);

/* ---- DrawList V1 (optional) ---- */
typedef struct { float x, y, w, h; unsigned int rgba; } GPI_QuadV1;

#define GPI_DL_MAGIC 0x315f4c44u /* 'DL_1' */
typedef struct {
  uint32_t magic;        /* GPI_DL_MAGIC */
  uint32_t version;      /* 0x00010000 */
  uint32_t max_quads;    /* capacity */
  uint32_t quad_count;   /* written by plugin, read by host */
  GPI_QuadV1 quads[1];   /* fixed size for compatibility */
} GPI_DrawListV1;

/* ---- DrawList V1.5 (text runs) ---- */
typedef enum {
  GPI_TALIGN_LEFT   = 0,
  GPI_TALIGN_CENTER = 1,
  GPI_TALIGN_RIGHT  = 2
} GPI_TextAlign;

typedef struct {
  float x, y;           /* baseline origin in screen px */
  float size_px;        /* font pixel height to scale atlas to */
  unsigned int rgba;    /* text color (0xAABBGGRR) */
  uint32_t align;       /* GPI_TextAlign */
  uint32_t utf8_off;    /* offset into utf8_blob[] */
  uint32_t utf8_len;    /* bytes */
} GPI_TextRunV15;

#define GPI_DL15_MAGIC 0x315f4c44u  /* reuse 'DL_1' magic */
#define GPI_DL15_VERSION 0x00010001u
typedef struct {
  uint32_t magic;         /* GPI_DL15_MAGIC */
  uint32_t version;       /* GPI_DL15_VERSION */
  uint32_t max_quads;     /* capacity */
  uint32_t quad_count;   /* written by plugin */
  uint32_t max_text;     /* capacity (runs) */
  uint32_t text_count;   /* written by plugin */
  uint32_t utf8_capacity; /* bytes available for packed utf-8 */
  uint32_t utf8_size;    /* bytes written by plugin */
} GPI_DrawListV15;

typedef void (*GPI_GetDrawListFn)(GPI_DrawListV1** out_ptr, uint32_t* out_bytes);
typedef void (*GPI_GetDrawListV15Fn)(GPI_DrawListV15** out_ptr, uint32_t* out_bytes);
typedef void (*GPI_GetFontMetricsFn)(float* ascent, float* descent, float* line_gap, float* atlas_px_height);

/* ---- Images V1 (Sprite Atlas) ---- */
typedef uint32_t GPI_ImageHandle;  /* opaque handle to uploaded texture */

/* Upload RGBA8 image data, returns handle (0 = failure) */
typedef GPI_ImageHandle (*GPI_UploadImageFn)(const void* rgba_data, int w, int h);

/* Free an uploaded image */
typedef void (*GPI_FreeImageFn)(GPI_ImageHandle handle);

/* Sprite quad (extension of GPI_QuadV1 with texture) */
typedef struct {
  float x, y, w, h;     /* screen-space dest rect */
  float u0, v0, u1, v1; /* texture UVs (0..1) */
  GPI_ImageHandle tex;  /* texture handle */
  unsigned int rgba;    /* tint color (0xAABBGGRR) */
} GPI_SpriteV1;

typedef struct {
  GPI_LogFn         log_info;
  GPI_LogFn         log_warn;
  GPI_LogFn         log_error;
  GPI_TelemetryFn   telemetry_mark;
  GPI_SavePutFn     save_put;
  GPI_SaveGetFn     save_get;
  GPI_DrawRectFn    draw_rects;      /* older primitive path (still works) */
  GPI_GetDrawListFn get_drawlist_v1; /* new zero-copy path (nullable) */
  GPI_GetDrawListV15Fn get_drawlist_v15; /* text runs path (nullable) */
  GPI_GetFontMetricsFn get_font_metrics_v15; /* font metrics (nullable) */
  GPI_UploadImageFn upload_image;    /* upload texture (nullable) */
  GPI_FreeImageFn   free_image;      /* free texture (nullable) */
} GPI_HostApi;

#if defined(_WIN32) || defined(_WIN64)
  #define GPI_EXPORT extern __declspec(dllexport)
#else
  #define GPI_EXPORT __attribute__((visibility("default"))) extern
#endif

GPI_EXPORT GPI_Result gpi_init(const GPI_VersionInfo* ver, const GPI_HostApi* api);
GPI_EXPORT GPI_Capabilities gpi_query_capabilities(void);
GPI_EXPORT GPI_Result gpi_update(const GPI_FrameContext* ctx);
GPI_EXPORT GPI_Result gpi_render(void);
GPI_EXPORT void gpi_suspend(void);
GPI_EXPORT void gpi_resume(void);
GPI_EXPORT void gpi_shutdown(void);

#ifdef __cplusplus
}
#endif
#endif
