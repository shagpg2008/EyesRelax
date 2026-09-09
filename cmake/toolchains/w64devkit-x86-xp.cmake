set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(W64DEVKIT_ROOT "D:/tools/w64devkit" CACHE PATH "w64devkit x86 root")
set(CMAKE_C_COMPILER "${W64DEVKIT_ROOT}/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "${W64DEVKIT_ROOT}/bin/g++.exe")
set(CMAKE_RC_COMPILER "${W64DEVKIT_ROOT}/bin/windres.exe")
set(CMAKE_MAKE_PROGRAM "${W64DEVKIT_ROOT}/bin/make.exe" CACHE FILEPATH "" FORCE)

# Force GCC to use the matching assembler/linker even if another MinGW is in PATH.
set(CMAKE_C_FLAGS_INIT "-B${W64DEVKIT_ROOT}/bin/")
set(CMAKE_CXX_FLAGS_INIT "-B${W64DEVKIT_ROOT}/bin/")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-B${W64DEVKIT_ROOT}/bin/")

set(EYERELAX_TARGET_XP ON CACHE BOOL "" FORCE)

