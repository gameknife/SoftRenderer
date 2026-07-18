# SoftRenderer

An experimental software renderer for learning and research.

## Environment

SoftRenderer uses [MiniFB](https://github.com/emoon/minifb) for its window,
framebuffer presentation, keyboard, and mouse support. DirectX and DirectInput
are no longer required. The macOS build has been verified on Apple Silicon.
Windows and Linux/Wayland are configured through the same CMake project.

All platforms require CMake 3.20 or newer. On macOS, install the Xcode Command
Line Tools. On Windows, use Visual Studio with the Desktop development with C++ workload.
On Linux/Wayland, MiniFB additionally needs a C/C++ toolchain, `pkg-config`, and
the Wayland client, cursor, and xkbcommon development packages. On Debian/Ubuntu:

```sh
sudo apt install build-essential cmake pkg-config libwayland-dev libxkbcommon-dev
```

## Quick start

Run `buildrun.sh` from the repository root:

```sh
./buildrun.sh
```

On macOS the script configures `out/macos`; on Linux it configures
`out/linux-wayland` with MiniFB's Wayland backend. It then builds SoftRenderer
and launches `bin/SoftRenderer`.

On Windows, run `buildrun.bat` from a Visual Studio-capable command prompt.

To build without launching the application:

```sh
./buildrun.sh --build-only
```

## Manual CMake build

Configure and build a macOS Release target with:

```sh
cmake -S . -B out/macos -DCMAKE_BUILD_TYPE=Release
cmake --build out/macos --parallel
```

For Linux/Wayland:

```sh
cmake -S . -B out/linux-wayland -DCMAKE_BUILD_TYPE=Release -DSR_LINUX_WAYLAND=ON
cmake --build out/linux-wayland --parallel
```

Set `SR_LINUX_WAYLAND=OFF` to use MiniFB's X11/OpenGL backend instead. On
Windows, `buildrun.bat --build-only` configures and builds the x64 Release target.

The runtime locates its assets relative to the executable, so it can be started
from any working directory:

```sh
./bin/SoftRenderer
```

## Output layout

Build outputs are written directly to the repository's `bin/` directory:

```text
bin/
├── SoftRenderer
├── renderer/SrSwRendererCpu.dylib  # .dll on Windows, .so on Linux
├── shader/BasicShaders.swsl
├── shader/AdvanceShaders.swsl
└── media/                  # tracked runtime assets
```

The build does not copy runtime assets. The application uses the existing
tracked `bin/media/` directory.

## Optimization comparison

The following screenshots show the Sponza scene before and after the renderer
optimization. The optimized view includes the tile-binning statistics together
with the CPU model and worker information.

| Before | After |
| --- | --- |
| ![Before optimization](doc/sr_dbc6527.webp) | ![After optimization](doc/sr_c87febf.webp) |

## Tests

Build and run the deterministic tile layout/binning tests with:

```sh
cmake --build out/macos --target SoftRendererTests
ctest --test-dir out/macos --output-on-failure
```

## Renderer options

The legacy backend remains available as an A/B reference. Use `--tile` for the
tile-binning renderer and `--legacy` for the legacy renderer. The default tile
size is 24×24 and can be overridden at runtime.

```text
--tile | --legacy
--workers=N
--tile-size=N
--width=N --height=N
--scene=sponza|model
--headless --frames=N
--no-jitaa --no-ssao --relaxed-ssao
--hash-output=results.csv
```

`--relaxed-ssao` removes the global barrier between rasterization and full
G-buffer SSAO shading. It is memory-safe but schedule-dependent and may differ
visually from the exact default mode.

`--hash-output` writes per-frame color, depth, G-buffer, and winner hashes,
stage timings, tile/bin counts, worker statistics, and load-balance metrics.
Tile size and worker count can also be set with `SR_TILE_SIZE` and
`SR_WORKER_COUNT`.
