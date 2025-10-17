# GPI Sandbox — Quickstart

Build your first plugin in ~10 minutes.

## Prerequisites

- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.22+
- SDL2 development libraries

## 1. Get the SDK

Download `gpi-sdk.zip` from the [releases page](https://github.com/your-org/gpi-sandbox/releases).

```bash
unzip gpi-sdk.zip
cd gpi-sdk
```

## 2. Create a Plugin

```bash
mkdir my_plugin
cd my_plugin
```

Copy the template files:
```bash
cp -r ../template/* .
```

## 3. Implement Your Plugin

Edit `plugin_example.cpp`:

```cpp
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
```

## 4. Build

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## 5. Run

```bash
# Copy your plugin to the host's plugins directory
cp libmy_plugin.so ../../gpi_host/plugins/

# Run the host
../../gpi_host/gpi_host --plugin my_plugin
```

## Next Steps

- Read the [Architecture Guide](architecture.md)
- Check the [ABI Reference](abi_reference.md)
- Explore the [API Documentation](api/)

## Troubleshooting

**Build fails**: Ensure you have SDL2 development libraries installed.

**Plugin not found**: Check the plugin path and file permissions.

**Crash on load**: Verify your plugin exports all required functions.

## Examples

- **Pong**: Classic paddle game
- **Snake**: Grid-based snake game
- **Particles**: Particle system demo

See the `examples/` directory for more complex plugins.
