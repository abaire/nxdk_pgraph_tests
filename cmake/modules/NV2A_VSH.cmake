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

    set("${target}")
    foreach (src ${NV2A_VSH_SOURCES})
        set(output "${CMAKE_CURRENT_BINARY_DIR}/${src}inc")

        get_filename_component(abs_src "${src}" REALPATH)

        add_custom_command(
                OUTPUT "${output}"
                COMMAND "${NV2AVSH}" "${abs_src}" "${output}"
                DEPENDS "${abs_src}"
        )

        set("${target}" "${${target}};${output}" CACHE INTERNAL "")
    endforeach ()
endfunction()
