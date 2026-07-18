# SoftRenderer

An SoftRenderer for experiment &amp; learn.

## Build with CMake

This is a Windows/MSVC project. Configure and build a Win32 Release binary from
PowerShell with:

```powershell
cmake -S . -B out/cmake-win32 -G "Visual Studio 18 2026" -A Win32
cmake --build out/cmake-win32 --config Release --target SoftRenderer
```

The runnable layout is generated under
`out/cmake-win32/runtime/Release/`. Run `SoftRenderer.exe` from that directory;
CMake places the renderer DLL in `renderer/`, shader modules in `shader/`, and
copies the tracked media files into `media/`.

## Tile-binning renderer

The legacy backend remains the default while the tile performance gates are
being validated. Pass `--tile` to use draw-local geometry jobs, deterministic
CSR binning, dynamically scheduled single-owner tiles, local pixel shading,
and the deferred full-GBuffer batch used by the normal-mapped SSAO shader.
Use `--legacy` for an explicit A/B reference.
The measured default tile size is 24×24 and can be overridden at runtime.

Useful automation options:

```text
--tile | --legacy
--workers=N
--tile-size=N
--width=N --height=N
--scene=sponza|model
--headless --frames=N
--no-jitaa --no-ssao --no-ui --relaxed-ssao
--hash-output=results.csv
```

`--relaxed-ssao` is an opt-in tile mode that removes the global barrier between
rasterization and full-GBuffer SSAO shading. A tile publishes its GBuffer after
rasterization, then shades and post-processes immediately; SSAO samples aimed at
unfinished neighboring tiles are skipped. This is memory-safe but intentionally
schedule-dependent and may differ visually from the exact default mode.

The software overlay uses a startup-built ASCII glyph cache and writes directly
to the locked frame buffer, avoiding per-frame D3D `GetDC` synchronization.
Headless mode skips overlay work automatically; `--no-ui` does the same for an
interactive run or profiler A/B capture.

`--hash-output` writes per-frame color, depth, GBuffer, and winner hashes plus
stage timings, tile/bin counts, worker job counts, busy ratio, and load-balance
metrics. Tile size and worker count can also be set with `SR_TILE_SIZE` and
`SR_WORKER_COUNT`.

Run the deterministic layout/binning tests with:

```powershell
ctest --test-dir out/cmake-win32 -C Release --output-on-failure
```

For a one-click build and launch from the repository root, double-click
`buildrun.bat`. From a terminal, use `buildrun.bat --build-only` to build without
launching the application.
