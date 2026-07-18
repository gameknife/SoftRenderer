#!/bin/sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
app_path="$repo_dir/bin/SoftRenderer"

case "$(uname -s)" in
    Darwin)
        build_dir="$repo_dir/out/macos"
        platform=macos
        ;;
    Linux)
        build_dir="$repo_dir/out/linux-wayland"
        platform=linux-wayland
        ;;
    *)
        echo "error: unsupported platform; use buildrun.bat on Windows" >&2
        exit 1
        ;;
esac

build_only=false
if [ "${1:-}" = "--build-only" ]; then
    build_only=true
    shift
fi

# Keep runtime-relative behavior deterministic even when this script is called
# through an absolute path or from Finder/another working directory.
cd "$repo_dir"

if [ "$platform" = linux-wayland ]; then
    cmake -S "$repo_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DSR_LINUX_WAYLAND=ON
else
    cmake -S "$repo_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
fi
cmake --build "$build_dir" --parallel

if [ "$build_only" = true ]; then
    exit 0
fi

if [ ! -x "$app_path" ]; then
    echo "error: build succeeded but executable was not created: $app_path" >&2
    exit 1
fi

echo "Launching $app_path"
exec "$app_path" "$@"
