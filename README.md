# Eyes Relax

A small, native break reminder for Windows, Linux and macOS. The legacy Windows
build targets Windows XP SP3 (32-bit) using wxWidgets 3.2 and w64devkit x86.

## Dependencies

- wxWidgets 3.2 source in `third_party/wxWidgets`
- CMake 3.16 or newer
- A C++17 compiler
- w64devkit x86 for the Windows XP build

## Build on Windows 11

The recommended entry point builds either architecture (or both), accepts custom
w64devkit locations, and copies release files into `dist/`:

```powershell
.\scripts\build-windows.ps1
.\scripts\build-windows.ps1 -Architecture x86 -W64DevkitX86 "D:\tools\w64devkit"
.\scripts\build-windows.ps1 -Architecture x64 -W64DevkitX64 "D:\tools\w64devkit-x64"
```

The same arguments can be passed through `scripts\build-windows.cmd`.

Modern static x64 build with `D:\tools\w64devkit-x64`:

```powershell
cmake -S . -B build-win64 -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/w64devkit-x64.cmake
cmake --build build-win64 --parallel
```

Windows XP build from a w64devkit x86 shell:

```sh
cmake -S . -B build-xp -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/w64devkit-x86-xp.cmake
cmake --build build-xp --parallel
```

Or use the checked-in presets:

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64

cmake --preset windows-xp-x86
cmake --build --preset windows-xp-x86
```

Both Windows presets statically link wxWidgets and the GCC runtimes. Release
executables are stripped by the linker and compressed with UPX. Disable UPX for
debugging or antivirus comparison with `-DEYERELAX_USE_UPX=OFF`.

The settings UI uses native wxWidgets controls and follows the operating system
theme. The distraction-free break overlay intentionally uses a black background.

The XP option rejects a 64-bit compiler. The resulting executable should be
checked with `objdump -p build-xp/EyesRelax.exe` and tested on an XP SP3 VM.

## Build on Linux and macOS

On Linux, install a C++ compiler, CMake, GTK 3 development packages and the
usual wxWidgets build dependencies, then run:

```sh
./scripts/build-linux.sh
```

On macOS, install Xcode command-line tools and CMake, then run:

```sh
./scripts/build-macos.sh
```

Set `JOBS` to override the detected parallel job count. Linux output is placed
in `dist/linux`; the macOS application bundle is placed in `dist/macos`.

## Start at sign-in

The Settings window can register Eyes Relax to start for the current user.
Windows uses the per-user `Run` registry key, Linux uses an XDG autostart
desktop file, and macOS uses a per-user LaunchAgent. Disabling the option removes
the corresponding entry. No administrator privileges are required.

## Tray and break controls

Double-click the tray icon to disable or re-enable all scheduled breaks. The
Settings window shows a live countdown to the next break. It also controls
whether the full-screen break overlay offers a Cancel button.

## Locales

Locale files are UTF-8 flat JSON objects in `locales/`. English is the final
fallback. A locale can be selected in Settings; `system` follows the OS locale.
