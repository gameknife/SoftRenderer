@echo off
setlocal EnableExtensions EnableDelayedExpansion

cd /d "%~dp0"
set "ROOT=%CD%"
set "BUILD_DIR=%ROOT%\out\cmake-x64"
set "RUNTIME_DIR=%ROOT%\bin"

where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake was not found in PATH.
    echo         Install CMake and make sure cmake.exe is available from a terminal.
    exit /b 1
)

rem Pick the newest Visual Studio generator reported by this CMake.
set "GENERATOR="
for /f "tokens=*" %%G in ('cmake --help ^| findstr /r /c:"Visual Studio [0-9][0-9]* [0-9][0-9]*"') do (
    if not defined GENERATOR (
        for /f "tokens=2-5" %%A in ("%%G") do set "GENERATOR=%%A %%B %%C %%D"
    )
)

if not defined GENERATOR (
    echo [ERROR] No Visual Studio CMake generator was found.
    echo         Install Visual Studio with the Desktop development with C++ workload.
    exit /b 1
)

echo Using generator: !GENERATOR!

rem Recreate only our generated build directory if it was made by another generator.
if exist "%BUILD_DIR%\CMakeCache.txt" (
    set "CACHED_GENERATOR="
    for /f "tokens=1,* delims==" %%A in ('findstr /b /c:"CMAKE_GENERATOR:INTERNAL=" "%BUILD_DIR%\CMakeCache.txt"') do set "CACHED_GENERATOR=%%B"
    if defined CACHED_GENERATOR if /i not "!CACHED_GENERATOR!"=="!GENERATOR!" (
        echo Existing build directory uses !CACHED_GENERATOR!; recreating it.
        rmdir /s /q "%BUILD_DIR%"
    )
)

echo Configuring x64 Release build...
cmake -S "%ROOT%" -B "%BUILD_DIR%" -G "!GENERATOR!" -A x64
if errorlevel 1 goto :build_failed

echo Building SoftRenderer...
cmake --build "%BUILD_DIR%" --config Release --target SoftRenderer --parallel
if errorlevel 1 goto :build_failed

if /i "%~1"=="--build-only" (
    echo Build completed. Skipping launch because --build-only was specified.
    exit /b 0
)

if not exist "%RUNTIME_DIR%\SoftRenderer.exe" (
    echo [ERROR] Build succeeded but SoftRenderer.exe was not found:
    echo         %RUNTIME_DIR%\SoftRenderer.exe
    exit /b 1
)

echo Launching SoftRenderer...
pushd "%RUNTIME_DIR%"
SoftRenderer.exe
set "APP_EXIT=!ERRORLEVEL!"
popd

echo SoftRenderer exited with code !APP_EXIT!.
exit /b !APP_EXIT!

:build_failed
echo [ERROR] CMake configure or build failed.
exit /b 1
