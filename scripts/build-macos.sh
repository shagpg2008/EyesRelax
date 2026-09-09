#!/usr/bin/env sh
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
jobs=${JOBS:-$(sysctl -n hw.logicalcpu 2>/dev/null || printf '4')}

cmake -S "$project_root" -B "$project_root/build/macos" -G Xcode \
  -DCMAKE_BUILD_TYPE=Release -DEYERELAX_USE_UPX=OFF
cmake --build "$project_root/build/macos" --config Release --parallel "$jobs"

mkdir -p "$project_root/dist/macos"
cp -R "$project_root/build/macos/Release/EyesRelax.app" "$project_root/dist/macos/"
