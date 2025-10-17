# GPI Sandbox Architecture

## Overview

GPI Sandbox is a modular game platform host that safely loads and executes plugins in isolated child processes. The architecture emphasizes performance, security, and reliability.

## Core Components

### Host Process (`gpi_host`)
- **Main Loop**: SDL2-based event loop with fixed timestep
- **Plugin Management**: Loading, lifecycle, hot-swapping
- **Rendering**: Zero-copy draw list with ImGui backend
- **Services**: Logging, telemetry, save store, replay/record

### Child Process (`gpi_child`)
- **Isolation**: Sandboxed execution environment
- **Communication**: Shared memory transport for commands/responses
- **Plugin Execution**: Actual plugin code runs here
- **Crash Handling**: Automatic crash reporting and recovery

### Plugin ABI
- **C Interface**: Stable ABI for plugin compatibility
- **Capabilities**: Feature detection and negotiation
- **Host API**: Services provided by host to plugins
- **Draw List**: Zero-copy rendering interface

## Data Flow

```
Host Process                    Child Process
┌─────────────┐                ┌─────────────┐
│   Main      │                │   Plugin    │
│   Loop      │                │   Code      │
└─────────────┘                └─────────────┘
       │                              │
       │ Shared Memory                │
       │ ┌─────────────────────────┐  │
       │ │ Command Ring            │  │
       │ │ Response Ring           │  │
       │ │ Draw List Buffer        │  │
       │ └─────────────────────────┘  │
       │                              │
       ▼                              ▼
┌─────────────┐                ┌─────────────┐
│   Render    │                │   Update    │
│   Pipeline  │                │   Render    │
└─────────────┘                └─────────────┘
```

## Security Model

### Process Isolation
- Plugins run in separate child processes
- No direct memory access between host and plugins
- Network access denied by default

### Sandboxing
- **Windows**: Job Objects, restricted tokens
- **macOS**: Seatbelt profiles, rlimits
- **Linux**: seccomp-bpf, rlimits

### Communication
- Shared memory for performance
- Atomic operations for synchronization
- Command/response protocol for safety

## Performance Features

### Zero-Copy Rendering
- Draw list shared between host and child
- No per-frame data copying
- Efficient GPU upload

### Fixed Timestep
- Deterministic simulation
- Replay/record capability
- Headless performance testing

### Telemetry
- Per-call timing metrics
- Frame rate monitoring
- Memory usage tracking

## Quality Assurance

### Visual Testing
- Golden replay system
- SSIM-based image comparison
- Automated regression detection

### Performance Testing
- Headless mode for CI
- Frame time analysis
- Memory leak detection

### Crash Handling
- Automatic crash dumps
- Stall detection and recovery
- Plugin hot-swapping

## Extension Points

### Plugin Store
- Manifest-based plugin discovery
- Signature verification
- Dependency management

### SDK
- Template plugin generation
- CMake integration
- Documentation and examples

## Future Considerations

- Metal/Vulkan rendering backend
- WebAssembly plugin support
- Distributed plugin execution
- Advanced sandboxing (Capsicum, Landlock)
