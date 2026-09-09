#!/usr/bin/env sh
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
jobs=${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '4')}

cmake -S "$project_root" -B "$project_root/build/linux" \
  -DCMAKE_BUILD_TYPE=Release -DEYERELAX_USE_UPX=OFF
cmake --build "$project_root/build/linux" --parallel "$jobs"

mkdir -p "$project_root/dist/linux"
cp "$project_root/build/linux/EyesRelax" "$project_root/dist/linux/EyesRelax"
cp -R "$project_root/locales" "$project_root/dist/linux/"
mkdir -p "$project_root/dist/linux/icons"
cp "$project_root/assets/icons/runtime/"*.png "$project_root/dist/linux/icons/"
