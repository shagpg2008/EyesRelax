foreach(required IN ITEMS EYERELAX_ICON_SOURCE EYERELAX_ICON_OUTPUT)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "Missing ${required}")
    endif()
endforeach()
find_program(EYERELAX_SIPS sips)
find_program(EYERELAX_ICONUTIL iconutil)
if(NOT EYERELAX_SIPS OR NOT EYERELAX_ICONUTIL)
    message(FATAL_ERROR "Generating the application icon requires macOS sips and iconutil")
endif()

get_filename_component(output_dir "${EYERELAX_ICON_OUTPUT}" DIRECTORY)
set(iconset "${output_dir}/EyesRelax.iconset")
file(MAKE_DIRECTORY "${iconset}")
foreach(size IN ITEMS 16 32 128 256 512)
    foreach(scale IN ITEMS 1 2)
        math(EXPR pixels "${size} * ${scale}")
        set(suffix "")
        if(scale EQUAL 2)
            set(suffix "@2x")
        endif()
        execute_process(
            COMMAND "${EYERELAX_SIPS}" -z "${pixels}" "${pixels}"
                "${EYERELAX_ICON_SOURCE}"
                --out "${iconset}/icon_${size}x${size}${suffix}.png"
            RESULT_VARIABLE result OUTPUT_QUIET ERROR_VARIABLE error
        )
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Failed to resize the macOS icon: ${error}")
        endif()
    endforeach()
endforeach()
execute_process(
    COMMAND "${EYERELAX_ICONUTIL}" --convert icns --output "${EYERELAX_ICON_OUTPUT}" "${iconset}"
    RESULT_VARIABLE result ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to generate the macOS icon: ${error}")
endif()
