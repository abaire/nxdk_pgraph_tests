# Provides `generate_nv2a_vshinc_files`, a function to compile nv2a vertex
# shaders written in nv2a assembly into `.vshinc` source files that may be
# included and uploaded using pbkit.

include(CMakeParseArguments)

# pip3 install nv2a-vsh
# https://pypi.org/project/nv2a-vsh/
# https://github.com/abaire/nv2a_vsh_asm
set(NV2AVSH nv2avsh)

function(generate_nv2a_vshinc_files)

    cmake_parse_arguments(
            PARSE_ARGV
            1
            "NV2A_VSH"
            ""
            ""
            "SOURCES"
    )

    set(target "${ARGV0}")

    set(generated_sources)
    set(generated_source_dirs)

    foreach (src ${NV2A_VSH_SOURCES})
        get_filename_component(abs_src "${src}" REALPATH)
        get_filename_component(src_dirname "${src}" DIRECTORY)
        get_filename_component(src_basename "${src}" NAME_WE)
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${src_dirname}")
        set(output "${CMAKE_CURRENT_BINARY_DIR}/${src_dirname}/${src_basename}.vshinc")

        add_custom_command(
                OUTPUT "${output}"
                COMMAND "${NV2AVSH}" "${abs_src}" "${output}"
                DEPENDS "${abs_src}"
        )

        list(APPEND generated_sources "${output}")
        list(APPEND generated_source_dirs "${CMAKE_CURRENT_BINARY_DIR}/${src_dirname}")
    endforeach ()

    add_custom_target(
            "${target}_gen"
            DEPENDS
            ${generated_sources}
    )

    list(REMOVE_DUPLICATES generated_source_dirs)
    add_library(
            "${target}"
            INTERFACE
            EXCLUDE_FROM_ALL
            ${generated_sources}
    )
    target_include_directories(
            "${target}"
            INTERFACE
            ${generated_source_dirs}
    )
    add_dependencies(
            "${target}"
            "${target}_gen"
    )
endfunction()
