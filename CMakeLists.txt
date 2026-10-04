cmake_minimum_required(VERSION 3.16)

# Set this before compiler detection and before wxWidgets chooses its older
# default. Both Apple Silicon and Intel builds use the same supported baseline.
if(CMAKE_HOST_APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET
   AND NOT DEFINED ENV{MACOSX_DEPLOYMENT_TARGET})
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

project(EyesRelax VERSION 0.1.0 LANGUAGES CXX)

if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 17)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    set(CMAKE_OBJCXX_EXTENSIONS OFF)
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_SOURCE_DIR}/cmake/PrepareWxWidgets.cmake"
    RESULT_VARIABLE EYERELAX_PREPARE_WX_RESULT
)
if(NOT EYERELAX_PREPARE_WX_RESULT EQUAL 0)
    message(FATAL_ERROR "Unable to prepare the bundled wxWidgets source archive")
endif()

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
    src/PowerControl.cpp
    src/Settings.cpp
    src/SettingsFrame.cpp
    src/ShutdownSchedule.cpp
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
    target_sources(EyesRelax PRIVATE src/MacWindowActivation.mm)
    target_link_libraries(EyesRelax PRIVATE "-framework ApplicationServices" "-framework AppKit")
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(EYERELAX_AYATANA QUIET IMPORTED_TARGET
                      ayatana-appindicator3-0.1)
    if(TARGET PkgConfig::EYERELAX_AYATANA)
        target_compile_definitions(EyesRelax PRIVATE
            EYERELAX_USE_APPINDICATOR=1
            EYERELAX_USE_AYATANA_APPINDICATOR=1
        )
        target_link_libraries(EyesRelax PRIVATE PkgConfig::EYERELAX_AYATANA)
    else()
        pkg_check_modules(EYERELAX_APPINDICATOR REQUIRED IMPORTED_TARGET
                          appindicator3-0.1)
        target_compile_definitions(EyesRelax PRIVATE EYERELAX_USE_APPINDICATOR=1)
        target_link_libraries(EyesRelax PRIVATE PkgConfig::EYERELAX_APPINDICATOR)
    endif()

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

# Some Linux file managers classify position-independent executables (ET_DYN)
# as shared libraries instead of launchable applications. EyesRelax does not
# require PIE, so emit a traditional ET_EXEC binary for reliable desktop launch.
if(UNIX AND NOT APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(EyesRelax PRIVATE -fno-pie)
    target_link_options(EyesRelax PRIVATE -no-pie)
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

if(APPLE)
    set_target_properties(EyesRelax PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/macos/Info.plist.in"
        MACOSX_BUNDLE_GUI_IDENTIFIER "net.eyesrelax.app"
        MACOSX_BUNDLE_BUNDLE_NAME "EyesRelax"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "EyesRelax.icns"
    )
    set(EYERELAX_MAC_ICON "${CMAKE_CURRENT_BINARY_DIR}/macos-icons/EyesRelax.icns")
    add_custom_command(OUTPUT "${EYERELAX_MAC_ICON}"
        COMMAND "${CMAKE_COMMAND}"
            "-DEYERELAX_ICON_SOURCE=${CMAKE_SOURCE_DIR}/assets/icons/eye-open-photo-inspired-v2.png"
            "-DEYERELAX_ICON_OUTPUT=${EYERELAX_MAC_ICON}"
            -P "${CMAKE_SOURCE_DIR}/cmake/GenerateMacIcon.cmake"
        DEPENDS "${CMAKE_SOURCE_DIR}/assets/icons/eye-open-photo-inspired-v2.png"
                "${CMAKE_SOURCE_DIR}/cmake/GenerateMacIcon.cmake"
        COMMENT "Generating macOS application icon"
        VERBATIM
    )
    add_custom_target(EyesRelaxMacIcon DEPENDS "${EYERELAX_MAC_ICON}")
    add_dependencies(EyesRelax EyesRelaxMacIcon)
    set_source_files_properties("${EYERELAX_MAC_ICON}"
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(EyesRelax PRIVATE "${EYERELAX_MAC_ICON}")
    file(GLOB EYERELAX_LOCALE_RESOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/locales/*.json")
    file(GLOB EYERELAX_ICON_RESOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/assets/icons/runtime/*.png")
    set_source_files_properties(${EYERELAX_LOCALE_RESOURCES}
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/locales")
    set_source_files_properties(${EYERELAX_ICON_RESOURCES}
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/icons")
    target_sources(EyesRelax PRIVATE ${EYERELAX_LOCALE_RESOURCES} ${EYERELAX_ICON_RESOURCES})
    install(TARGETS EyesRelax BUNDLE DESTINATION . COMPONENT Runtime)
else()
    add_custom_command(TARGET EyesRelax POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/locales"
                "$<TARGET_FILE_DIR:EyesRelax>/locales"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/assets/icons/runtime"
                "$<TARGET_FILE_DIR:EyesRelax>/icons"
    )
endif()

if(UNIX AND NOT APPLE)
    include(cmake/InstallLinux.cmake)
endif()
