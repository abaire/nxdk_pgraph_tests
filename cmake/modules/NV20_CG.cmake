# Provides:
#
# `generate_pixelshader_inl_files`, a function to compile [cg](https://en.wikipedia.org/wiki/Cg_(programming_language))
# pixel shaders to .inl files using the FP20COMPILER.
#
# `generate_vertexshader_inl_files`, a function to compile [cg](https://en.wikipedia.org/wiki/Cg_(programming_language))
# vertex shaders to .inl files using the VP20COMPILER.
#
# Each function takes the name of a target followed by the "SOURCES" keyword, followed by any number of source files.
# The target should be added via `target_link_libraries` to any targets that require the generated sources.

include(CMakeParseArguments)

if (CMAKE_HOST_SYSTEM_NAME STREQUAL Darwin)
    set(CG "${NXDK_DIR}/tools/cg/mac/cgc")
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    set(CG "${NXDK_DIR}/tools/cg/win/cgc")
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    set(CG "${NXDK_DIR}/tools/cg/linux/cgc")
else ()
    message("Warning: Unknown host system '${CMAKE_HOST_SYSTEM_NAME}', defaulting to Linux")
    set(CG "${NXDK_DIR}/tools/cg/linux/cgc")
endif ()

set(FP20COMPILER "${NXDK_DIR}/tools/fp20compiler/fp20compiler")
set(VP20COMPILER "${NXDK_DIR}/tools/vp20compiler/vp20compiler")

function(generate_pixelshader_inl_files)
    cmake_parse_arguments(
            PARSE_ARGV
            1
            "FP20"
            ""
            ""
            "SOURCES"
    )

    set(target "${ARGV0}")

    set(generated_sources)
    set(generated_source_dirs)
    foreach (src ${FP20_SOURCES})
        get_filename_component(abs_src "${src}" REALPATH)

        set(intermediate "${CMAKE_CURRENT_BINARY_DIR}/${src}.cgout")
        get_filename_component(src_dirname "${src}" DIRECTORY)
        get_filename_component(src_basename "${src}" NAME_WE)
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${src_dirname}")
        set(output "${CMAKE_CURRENT_BINARY_DIR}/${src_dirname}/${src_basename}.inl")

        add_custom_command(
                OUTPUT "${output}"
                COMMAND "${CG}" -profile fp20 -o "${intermediate}" "${abs_src}"
                COMMAND "${FP20COMPILER}" "${intermediate}" > "${output}"
                DEPENDS "${abs_src}"
                BYPRODUCTS "${intermediate}"
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


function(generate_vertexshader_inl_files)
    cmake_parse_arguments(
            PARSE_ARGV
            1
            "VP20"
            ""
            ""
            "SOURCES"
    )

    set(target "${ARGV0}")

    set(generated_sources)
    set(generated_source_dirs)
    foreach (src ${VP20_SOURCES})
        get_filename_component(abs_src "${src}" REALPATH)

        set(intermediate "${CMAKE_CURRENT_BINARY_DIR}/${src}.cgout")
        get_filename_component(src_dirname "${src}" DIRECTORY)
        get_filename_component(src_basename "${src}" NAME_WE)
        set(output "${CMAKE_CURRENT_BINARY_DIR}/${src_dirname}/${src_basename}.inl")

        add_custom_command(
                OUTPUT "${output}"
                COMMAND "${CG}" -profile vp20 -o "${intermediate}" "${abs_src}"
                COMMAND "${VP20COMPILER}" "${intermediate}" > "${output}"
                DEPENDS "${abs_src}"
                BYPRODUCTS "${intermediate}"
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
