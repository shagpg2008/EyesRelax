cmake_minimum_required(VERSION 3.16)

project(EyesRelax VERSION 0.1.0 LANGUAGES CXX)

option(EYERELAX_TARGET_XP "Build a Windows XP SP3 compatible x86 binary" OFF)
option(EYERELAX_USE_UPX "Compress Windows release binaries with UPX" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(wxBUILD_SHARED OFF CACHE BOOL "" FORCE)
set(wxBUILD_MONOLITHIC OFF CACHE BOOL "" FORCE)
set(wxBUILD_SAMPLES OFF CACHE STRING "" FORCE)
set(wxBUILD_TESTS OFF CACHE STRING "" FORCE)
set(wxBUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(wxBUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(wxBUILD_PRECOMP OFF CACHE STRING "" FORCE)
set(wxBUILD_INSTALL OFF CACHE BOOL "" FORCE)
set(wxUSE_WEBVIEW OFF CACHE BOOL "" FORCE)
set(wxUSE_MEDIACTRL OFF CACHE BOOL "" FORCE)
set(wxUSE_RIBBON OFF CACHE BOOL "" FORCE)
set(wxUSE_RICHTEXT OFF CACHE BOOL "" FORCE)
set(wxUSE_STC OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/wxWidgets EXCLUDE_FROM_ALL)

add_executable(EyesRelax WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/App.cpp
    src/BreakScheduler.cpp
    src/BreakWindow.cpp
    src/Localization.cpp
    src/Icons.cpp
    src/IdleTime.cpp
    src/Settings.cpp
    src/SettingsFrame.cpp
    src/TrayIcon.cpp
    src/UpcomingBreakWindow.cpp
)

if(WIN32)
    enable_language(RC)
    target_sources(EyesRelax PRIVATE resources/windows/EyesRelax.rc)
endif()

target_include_directories(EyesRelax PRIVATE include)
target_link_libraries(EyesRelax PRIVATE wx::core wx::base)

if(APPLE)
    target_link_libraries(EyesRelax PRIVATE "-framework ApplicationServices")
elseif(UNIX)
    find_package(X11 QUIET)
    if(X11_FOUND AND X11_Xss_FOUND)
        target_compile_definitions(EyesRelax PRIVATE EYERELAX_HAVE_XSS=1)
        target_link_libraries(EyesRelax PRIVATE X11::X11 X11::Xss)
    endif()
endif()

target_compile_definitions(EyesRelax PRIVATE
    EYERELAX_VERSION="${PROJECT_VERSION}"
)

if(WIN32)
    target_compile_definitions(EyesRelax PRIVATE UNICODE _UNICODE)
endif()

if(WIN32 AND MINGW)
    target_compile_options(EyesRelax PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(EyesRelax PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
        -Wl,--gc-sections
        $<$<CONFIG:Release>:-s>
    )
endif()

if(EYERELAX_TARGET_XP)
    if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(FATAL_ERROR "EYERELAX_TARGET_XP requires a 32-bit Windows toolchain")
    endif()
    target_compile_definitions(EyesRelax PRIVATE
        EYERELAX_LEGACY_WINDOWS=1
        WINVER=0x0501
        _WIN32_WINNT=0x0501
        NTDDI_VERSION=0x05010300
    )
    if(MINGW)
        target_link_options(EyesRelax PRIVATE
            -Wl,--major-subsystem-version,5
            -Wl,--minor-subsystem-version,1
        )
    endif()
endif()

if(WIN32 AND EYERELAX_USE_UPX)
    get_filename_component(EYERELAX_COMPILER_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    find_program(EYERELAX_UPX_EXECUTABLE NAMES upx
        HINTS "${W64DEVKIT_ROOT}/bin" "${EYERELAX_COMPILER_BIN}"
        NO_DEFAULT_PATH
    )
    if(NOT EYERELAX_UPX_EXECUTABLE)
        message(FATAL_ERROR "EYERELAX_USE_UPX is ON but upx was not found in the selected toolchain")
    endif()
    add_custom_command(TARGET EyesRelax POST_BUILD
        COMMAND "${EYERELAX_UPX_EXECUTABLE}" --best --no-progress "$<TARGET_FILE:EyesRelax>"
        COMMENT "Compressing EyesRelax with UPX"
        VERBATIM
    )
endif()

add_custom_command(TARGET EyesRelax POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/locales"
            "$<TARGET_FILE_DIR:EyesRelax>/locales"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets/icons/runtime"
            "$<TARGET_FILE_DIR:EyesRelax>/icons"
)
