#!/usr/bin/env sh
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
jobs=${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '4')}

cmake -S "$project_root" -B "$project_root/build/linux" \
  -DCMAKE_BUILD_TYPE=Release -DEYERELAX_USE_UPX=OFF -DwxBUILD_SHARED=OFF
cmake --build "$project_root/build/linux" --parallel "$jobs"

mkdir -p "$project_root/dist/linux"
cp "$project_root/build/linux/EyesRelax" "$project_root/dist/linux/EyesRelax"
chmod 755 "$project_root/dist/linux/EyesRelax"
cp -R "$project_root/locales" "$project_root/dist/linux/"
mkdir -p "$project_root/dist/linux/icons"
cp "$project_root/assets/icons/runtime/"*.png "$project_root/dist/linux/icons/"

executable="$project_root/dist/linux/EyesRelax"
launcher="$project_root/dist/linux/EyesRelax.desktop"
icon="$project_root/dist/linux/icons/eye-open.png"
cmake \
  -DEYERELAX_PROJECT_ROOT="$project_root" \
  -DEYERELAX_DESKTOP_EXECUTABLE="$executable" \
  -DEYERELAX_DESKTOP_ICON="$icon" \
  -DEYERELAX_DESKTOP_OUTPUT="$launcher" \
  -P "$project_root/cmake/GenerateLinuxDesktop.cmake"
chmod 755 "$launcher"

if command -v readelf >/dev/null 2>&1; then
  if ! readelf -h "$executable" | grep -q 'Type:.*EXEC'; then
    echo "Error: EyesRelax is not an ET_EXEC executable." >&2
    exit 1
  fi
fi

if command -v ldd >/dev/null 2>&1; then
  ldd_output=$(ldd "$executable")
  if printf '%s\n' "$ldd_output" | grep -E 'libwx[^ ]*\.so'; then
    echo "Error: EyesRelax still depends on a shared wxWidgets library." >&2
    exit 1
  fi
fi

echo "Built executable: $executable"
echo "Desktop launcher: $launcher"
