#!/usr/bin/env sh
set -eu

if [ "$(uname -s)" != Darwin ]; then
  echo "Error: run this script on macOS with Apple Command Line Tools installed." >&2
  exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then
  echo "Error: CMake is not installed or is not on PATH." >&2
  exit 1
fi
if ! cc=$(xcrun --sdk macosx --find clang) ||
   ! cxx=$(xcrun --sdk macosx --find clang++) ||
   ! sdk=$(xcrun --sdk macosx --show-sdk-path); then
  echo "Error: Apple Clang/macOS SDK is unavailable. Run xcode-select --install." >&2
  echo "If tools are already installed, check xcode-select -p and DEVELOPER_DIR." >&2
  exit 1
fi

if [ -z "$cc" ] || [ ! -x "$cc" ] ||
   [ -z "$cxx" ] || [ ! -x "$cxx" ] || [ ! -d "$sdk" ]; then
  printf 'Error: invalid Apple toolchain paths.\nC compiler: %s\nC++ compiler: %s\nSDK: %s\n' \
    "$cc" "$cxx" "$sdk" >&2
  exit 1
fi
if ! "$cxx" --version; then
  echo "Error: the selected Clang C++ compiler cannot run. Check xcode-select -p." >&2
  exit 1
fi

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
jobs=${JOBS:-$(sysctl -n hw.logicalcpu 2>/dev/null || printf '4')}
# Use a new directory to avoid the previous script's cached Xcode generator.
build_dir=${EYERELAX_MACOS_BUILD_DIR:-"$project_root/build/macos-release"}
printf 'CMake: %s\nC compiler: %s\nC++ compiler: %s\nmacOS SDK: %s\nBuild directory: %s\n' \
  "$(command -v cmake)" "$cc" "$cxx" "$sdk" "$build_dir"

if ! cmake -S "$project_root" -B "$build_dir" -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release -DEYERELAX_USE_UPX=OFF -DwxBUILD_SHARED=OFF \
  -DCMAKE_C_COMPILER:FILEPATH="$cc" -DCMAKE_CXX_COMPILER:FILEPATH="$cxx" \
  -DCMAKE_OBJCXX_COMPILER:FILEPATH="$cxx" -DCMAKE_OSX_SYSROOT:PATH="$sdk" "$@"; then
  echo "CMake configuration failed. Please include the toolchain paths printed above." >&2
  printf 'Compiler detection logs (depending on CMake version):\n  %s/CMakeFiles/CMakeConfigureLog.yaml\n  %s/CMakeFiles/CMakeError.log\n' \
    "$build_dir" "$build_dir" >&2
  echo 'To try without old cache, set EYERELAX_MACOS_BUILD_DIR to a new build directory.' >&2
  exit 1
fi
cmake --build "$build_dir" --config Release --parallel "$jobs"

cmake --install "$build_dir" --config Release --component Runtime \
  --prefix "$project_root/dist/macos"

app="$project_root/dist/macos/EyesRelax.app"
test -x "$app/Contents/MacOS/EyesRelax"
test -f "$app/Contents/Resources/locales/en.json"
test -f "$app/Contents/Resources/icons/eye-open.png"
test -f "$app/Contents/Resources/icons/eye-closed.png"
test -s "$app/Contents/Resources/EyesRelax.icns"
plutil -lint "$app/Contents/Info.plist"
echo "Built application: $app"
