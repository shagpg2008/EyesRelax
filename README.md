# Eyes Relax

A small, native break reminder for Windows, Linux and macOS. The legacy Windows
build targets Windows XP SP3 (32-bit) using wxWidgets 3.2 and w64devkit x86.

## Dependencies

- One wxWidgets 3.2 source archive named `wxWidgets*.zip` in `third_party/`
- CMake 3.16 or newer
- A C++17 compiler
- w64devkit x86 for the Windows XP build

Do not unpack wxWidgets manually and do not place its ZIP archive in the project
root. The expected layout is, for example:

```text
third_party/
└── wxWidgets-3.2.11.zip
```

During CMake configuration, `cmake/PrepareWxWidgets.cmake` finds the single
matching archive and extracts it to the ignored `third_party/wxWidgets/`
directory. Both the flat official Windows ZIP and an archive containing one
top-level source directory are supported. Extraction is skipped on later builds
when a valid source tree already exists. A process lock makes simultaneous x86
and x64 configuration safe, and an interrupted partial extraction is replaced
automatically on the next run.

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

All Windows entry points automatically prepare and compile the bundled
wxWidgets source. Both Windows presets statically link wxWidgets and the GCC runtimes. Release
executables are stripped by the linker and compressed with UPX. Disable UPX for
debugging or antivirus comparison with `-DEYERELAX_USE_UPX=OFF`.

The settings UI uses native wxWidgets controls and follows the operating system
theme. The distraction-free break overlay intentionally uses a black background.

## Scheduled shutdown window

The settings window can enable an independent daily shutdown time window with
second-level start and end times. An end time earlier than the start time means
the window crosses midnight; equal start and end times mean an empty window.
The interval includes its start and excludes its end. Enabling this option also
enables launch at sign-in, and the schedule is checked immediately after the
application starts. Pausing Eyes Relax from the tray pauses both break reminders
and scheduled shutdowns.

This is a user-session feature: the computer is shut down after the user signs
in and Eyes Relax starts, not at the firmware/login screen. Windows uses the
native shutdown API. Linux uses `systemctl poweroff`, and macOS uses System
Events; those systems may require the user or administrator to grant the
corresponding shutdown permission.

The XP option rejects a 64-bit compiler. The resulting executable should be
checked with `objdump -p build-xp/EyesRelax.exe` and tested on an XP SP3 VM.

## Build on Linux and macOS

On Linux, install a C++ compiler, CMake, GTK 3 development packages,
XScreenSaver development files, Ayatana AppIndicator development files and the
usual wxWidgets build dependencies. On Ubuntu/Debian:

```sh
sudo apt install build-essential cmake pkg-config libgtk-3-dev \
  libxss-dev libayatana-appindicator3-dev
```

Then run:

```sh
sh scripts/build-linux.sh
```

For a system-wide Linux installation, use CMake's install rules after building:

```sh
sudo cmake --install build/linux --prefix /usr/local
```

This installs `bin/EyesRelax`, resources in `share/eyesrelax`, the menu launcher
in `share/applications/eyesrelax.desktop`, and its icon in
`share/pixmaps/eyesrelax.png`, all under the selected prefix. Search for
**EyesRelax** in the application menu. No separate `logo.png` is required.
The portable `dist/linux` directory continues to work without installation.
If you configured a different build directory (such as `build/linux-release`),
pass that directory to `cmake --install` instead. `--install` does not build;
rebuild before installing updates. Packaging tools can also use `DESTDIR`.

On macOS, install Apple Command Line Tools (`xcode-select --install`) and CMake,
then run:

```sh
sh scripts/build-macos.sh
```

The macOS script uses Apple Clang and the SDK selected by `xcrun`, with Unix
Makefiles; the full Xcode application is not required. It builds in
`build/macos-release` to avoid the old Xcode generator cache, and packages
`dist/macos/EyesRelax.app` with language files and icons in `Contents/Resources`.
The application icon is generated from the existing eye artwork using macOS
`sips` and `iconutil` and embedded as `EyesRelax.icns` for Finder and the Dock.
The default deployment target is macOS 11.0, and the default architecture is
the build machine's native architecture. To request a universal build:

```sh
sh scripts/build-macos.sh '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'
```

For `No CMAKE_CXX_COMPILER could be found`, verify `xcrun --find clang++` and
`xcrun --sdk macosx --show-sdk-path`. If tools are installed but not selected,
use `sudo xcode-select --switch /Library/Developer/CommandLineTools` for the
standalone tools, or select your full Xcode installation. Re-run the script
instead of reusing an old build directory copied from another platform.
The script prints the exact compiler, SDK, and build paths before configuring.
To isolate an existing cache without deleting it, select a new directory:

```sh
EYERELAX_MACOS_BUILD_DIR="$PWD/build/macos-clean" sh scripts/build-macos.sh
```

If configuration still fails, include these printed paths and the compiler
detection details from `CMakeFiles/CMakeConfigureLog.yaml` (or `CMakeError.log`
with older CMake versions) inside that build directory.

Set `JOBS` to override the detected parallel job count. Linux output is placed
in `dist/linux`; the macOS application bundle is placed in `dist/macos`.
Both scripts automatically extract and compile the ZIP from `third_party/`.
wxWidgets itself is linked statically on Windows, Linux and macOS; operating
system frameworks and Linux system libraries remain normal platform dependencies.

The Linux build disables PIE so desktop file managers recognize `EyesRelax` as
an executable instead of a shared-library file. The build script also restores
mode `755` after copying it into `dist/linux`. Following the iTool packaging
approach, it also creates an absolute-path desktop launcher and verifies the ELF
type and absence of shared wxWidgets libraries. The preferred graphical entry is:

```text
dist/linux/EyesRelax.desktop
```

Double-click this launcher. Some file managers require selecting **Allow
Launching** or **Trust and Launch** once. A direct terminal launch remains
available:

```sh
./dist/linux/EyesRelax
```

If the binary was transferred through a filesystem or ZIP tool that discarded
Unix permissions, restore them before launching:

```sh
chmod +x dist/linux/EyesRelax
./dist/linux/EyesRelax
```

Use `file dist/linux/EyesRelax` to verify that it reports an ELF executable.
If the shell reports a missing shared object instead, install the GTK 3 and
XScreenSaver runtime packages for the distribution.

On Linux the Settings window is shown on startup. This is intentional because
GNOME and some other desktops may hide wxWidgets' legacy tray icon; without a
visible initial window, a successfully running tray-only process can appear to
have done nothing.

The Linux tray uses Ayatana AppIndicator (or the compatible legacy
`appindicator3-0.1` development package) instead of wxGTK's obsolete
`GtkStatusIcon`. This makes it visible through GNOME's AppIndicator extension
on both X11 and Wayland. AppIndicator reserves a normal click for opening its
menu, so Linux toggles reminders with the Pause/Resume menu item; tray-icon
double-click toggling remains available on Windows and macOS.

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
