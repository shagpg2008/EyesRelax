include(GNUInstallDirs)

# Resolve resources relative to the executable so --install --prefix works
# without rebuilding, including when packaging with DESTDIR.
file(RELATIVE_PATH EYERELAX_RESOURCE_DIR_FROM_BIN
    "${CMAKE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_FULL_DATADIR}/eyesrelax")
target_compile_definitions(EyesRelax PRIVATE
    EYERELAX_RESOURCE_DIR_FROM_BIN="${EYERELAX_RESOURCE_DIR_FROM_BIN}")

install(TARGETS EyesRelax RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(DIRECTORY "${PROJECT_SOURCE_DIR}/locales"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/eyesrelax")
install(DIRECTORY "${PROJECT_SOURCE_DIR}/assets/icons/runtime/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/eyesrelax/icons"
    FILES_MATCHING PATTERN "*.png")
install(FILES "${PROJECT_SOURCE_DIR}/assets/icons/runtime/eye-open.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps" RENAME eyesrelax.png)

# Generate Exec and Icon at install time, after --prefix has been applied.
configure_file("${PROJECT_SOURCE_DIR}/cmake/InstallLinuxDesktop.cmake.in"
    "${PROJECT_BINARY_DIR}/InstallLinuxDesktop.cmake" @ONLY)
install(SCRIPT "${PROJECT_BINARY_DIR}/InstallLinuxDesktop.cmake")
