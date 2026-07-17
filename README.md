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

For a one-click build and launch from the repository root, double-click
`buildrun.bat`. From a terminal, use `buildrun.bat --build-only` to build without
launching the application.
