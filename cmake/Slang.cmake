find_program(GEMSTONE_SLANGC
    NAMES slangc slangc.exe
    HINTS "$ENV{VULKAN_SDK}/Bin"
)

if(NOT GEMSTONE_SLANGC)
    message(FATAL_ERROR "Could not find slangc. Set -DGEMSTONE_SLANGC=...")
endif()

function(add_slang_shader)
    cmake_parse_arguments(PARSE_ARGV 0 ARG
        "ALL"
        "TARGET;NAME;SOURCE;ENTRY;PROFILE;OUTPUT_DIR"
        "INCLUDE_DIRS;DEPENDS;OPTIONS"
    )

    foreach(required TARGET NAME SOURCE ENTRY PROFILE)
        if(NOT ARG_${required})
            message(FATAL_ERROR
                "add_slang_shader requires ${required}"
            )
        endif()
    endforeach()

    if(NOT ARG_OUTPUT_DIR)
        set(ARG_OUTPUT_DIR
            "${CMAKE_CURRENT_BINARY_DIR}/Shaders"
        )
    endif()

    set(output "${ARG_OUTPUT_DIR}/${ARG_NAME}.dxil")

    set(include_arguments)
    foreach(include_dir IN LISTS ARG_INCLUDE_DIRS)
        list(APPEND include_arguments -I "${include_dir}")
    endforeach()

    add_custom_command(
        OUTPUT "${output}"

        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${ARG_OUTPUT_DIR}"

        COMMAND "${GEMSTONE_SLANGC}"
            "${ARG_SOURCE}"
            ${include_arguments}
            -target dxil
            -profile "${ARG_PROFILE}"
            -entry "${ARG_ENTRY}"
            ${ARG_OPTIONS}
            -o "${output}"

        DEPENDS
            "${ARG_SOURCE}"
            ${ARG_DEPENDS}

        VERBATIM
        COMMENT "Compiling ${ARG_NAME}.dxil"
    )

    string(MAKE_C_IDENTIFIER
        "${ARG_TARGET}_${ARG_NAME}"
        shader_target
    )

    add_custom_target("${shader_target}"
        DEPENDS "${output}"
    )

    if(NOT TARGET "${ARG_TARGET}")
        if(ARG_ALL)
            add_custom_target("${ARG_TARGET}" ALL)
        else()
            add_custom_target("${ARG_TARGET}")
        endif()
    endif()

    add_dependencies("${ARG_TARGET}" "${shader_target}")
endfunction()