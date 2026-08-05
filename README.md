# SoftRenderer

An experimental software renderer for learning and research.

## Environment

SoftRenderer uses [MiniFB](https://github.com/emoon/minifb) for its desktop
window, framebuffer presentation, keyboard, and mouse support. The renderer
and all software shaders are statically linked into the desktop executable;
there are no runtime renderer/shader DLL or `.swsl` plugin files. The UWP/Xbox
build uses the same statically linked renderer core behind a XAML/D3D12 host.
DirectX and DirectInput are not required by the desktop build.

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
└── media/                  # tracked runtime assets
```

The build does not copy runtime assets. The application uses the existing
tracked `bin/media/` directory.

## Xbox / UWP build

The UWP target is a single `SoftRendererUwp.exe` AppX binary: the CPU
rasterizer, core, shaders, UWP input adapter, and D3D12 frame presenter are all
compiled together. It does not load desktop renderer or shader plugins.

Requirements: Visual Studio with the UWP workload, a Windows SDK containing
`fxc.exe`, CMake, and the `Microsoft.Windows.CppWinRT` NuGet restore package.

```powershell
cmake --preset xbox-release
cmake --build --preset xbox-release --parallel 4
```

The MSIX package is produced below:

```text
out/build/xbox-release/AppPackages/SoftRendererUwp/SoftRendererUwp_1.0.0.0_x64_Test/
```

Use `uwp/scripts/Deploy-Xbox.ps1` to sign and upload a built package through
Xbox Device Portal. `windows-release` uses the same UWP binary and package
layout for local Windows testing.

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
