# GPI SDK v1.0.0

This is the GPI Sandbox SDK for creating plugins.

## Quick Start

1. Copy the template:
   ```bash
   cp -r template my_plugin
   cd my_plugin
   ```

2. Edit `plugin_example.cpp` to implement your plugin

3. Build:
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build .
   ```

4. Copy the built plugin to your host's plugins directory

## Documentation

- [Quickstart Guide](docs/quickstart.md)
- [Architecture](docs/architecture.md)
- [ABI Reference](docs/abi_reference.md)

## Examples

See the `template/` directory for a minimal example.

## Support

- GitHub Issues: https://github.com/your-org/gpi-sandbox/issues
- Documentation: https://github.com/your-org/gpi-sandbox/wiki
