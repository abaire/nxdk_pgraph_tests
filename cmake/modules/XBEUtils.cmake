# Provides:
#
# add_xbe(
#    target executable_file
#    [XBENAME <xbe_filename>="default.xbe"]
#    [TITLE <xbe title>={target}]
#    RESOURCE_FILES <files>
#    RESOURCE_DIRS <directories>
# )
#
#  Generates an XBE file from the given executable_file. The given resources will be attached such that add_xiso will
#  include them in the root of the generated iso.
#
# add_xiso(target xbe_target [XISO <filename>=<target>.xiso])
#  Generates an xiso image for the given XBE.

include(CMakeParseArguments)

set(CXBE_TOOL_PATH "${NXDK_DIR}/tools/cxbe/cxbe")
set(EXTRACT_XISO_TOOL_PATH "${NXDK_DIR}/tools/extract-xiso/build/extract-xiso")

# Makes each path in the given list into an absolute path.
function(_make_abs_paths list_name)
    foreach (src "${${list_name}}")
        get_filename_component(abs "${src}" ABSOLUTE)
        list(APPEND ret "${abs}")
    endforeach ()
    set(${list_name} ${ret} PARENT_SCOPE)
endfunction()

# split_debug(executable_file debug_file_variable)
#
# Splits debugging information from the given `executable_file`, generating a companion file ending in ".debug.exe"
# Sets `debug_file_variable` in the parent scope to the generated file.
function(split_debug)
    if (${ARGC} LESS 1)
        message(FATAL_ERROR "Missing required 'executable_file' parameter.")
    elseif (${ARGC} LESS 2)
        message(FATAL_ERROR "Missing required 'debug_file_variable_name' parameter.")
    endif ()

    set(exe_file "${ARGV0}")
    set(debug_file_variable "${ARGV1}")
    get_filename_component(exe_dirname "${exe_file}" DIRECTORY)
    get_filename_component(exe_basename "${exe_file}" NAME_WE)
    set(output "${exe_dirname}/${exe_basename}.debug.exe")

    add_custom_command(
            OUTPUT "${output}"
            COMMAND "${CMAKE_COMMAND}" -E copy "${exe_file}" "${output}"
            COMMAND "${CMAKE_OBJCOPY}" --strip-debug "${exe_file}"
            COMMAND "${CMAKE_OBJCOPY}" --add-gnu-debuglink="${output}" "${exe_file}"
            DEPENDS "${exe_file}"
    )
    set("${debug_file_variable}" "${output}" PARENT_SCOPE)
endfunction()

function(add_xbe)
    cmake_parse_arguments(
            PARSE_ARGV
            2
            "XBE"
            ""
            "XBENAME"
            "RESOURCE_FILES;RESOURCE_DIRS"
    )

    if (${ARGC} LESS 1)
        message(FATAL_ERROR "Missing required 'target' parameter.")
    elseif (${ARGC} LESS 2)
        message(FATAL_ERROR "Missing required 'executable_file' parameter.")
    endif ()

    set(target "${ARGV0}")
    set(exe_file "${ARGV1}")

    # exe_file will be updated to contain the stripped binary and
    # exe_file_DEBUG will be created to track the generated debug artifact.
    split_debug("${exe_file}" exe_file_DEBUG)
    message("DEBUG FILE: ${exe_file} -> ${exe_file_DEBUG}")

    if (NOT XBE_XBENAME)
        set(XBE_XBENAME default.xbe)
    endif ()

    if (NOT XBE_TITLE)
        set(XBE_TITLE "${target}")
    endif ()

    set(
            "${target}_XBE_STAGING_DIR"
            "${CMAKE_CURRENT_BINARY_DIR}/xbe/${target}"
            CACHE INTERNAL
            "Directory into which the raw sources for an xiso have been placed."
    )
    set(XBE_STAGING_DIR "${${target}_XBE_STAGING_DIR}")

    set(
            "${target}_XBE_OUTPUT_PATH"
            "${${target}_XBE_STAGING_DIR}/${XBE_XBENAME}"
            CACHE INTERNAL
            "XBE file that should be added to an xiso."
    )
    set(XBE_OUTPUT_PATH "${${target}_XBE_OUTPUT_PATH}")

    add_custom_command(
            OUTPUT "${XBE_STAGING_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${XBE_STAGING_DIR}"
    )

    file(MAKE_DIRECTORY "${XBE_STAGING_DIR}")

    add_custom_command(
            OUTPUT "${XBE_OUTPUT_PATH}"
            COMMAND "${CXBE_TOOL_PATH}"
            "-TITLE:${XBE_TITLE}"
            "-OUT:${XBE_OUTPUT_PATH}"
            "${exe_file}"
            DEPENDS "${exe_file}" "${exe_file_DEBUG}"
    )

    # Copy resources to the staging directory.
    set(
            "${target}_RESOURCE_FILES_RECEIPT_OUTPUT_PATH"
            CACHE
            INTERNAL
            "Timestamp file indicating that resource files have been copied."
    )
    if (XBE_RESOURCE_FILES)
        set(
                "${target}_RESOURCE_FILES_RECEIPT_OUTPUT_PATH"
                "${CMAKE_CURRENT_BINARY_DIR}/xbe/.${target}_resource_files_time"
                CACHE
                INTERNAL
                "Timestamp file indicating that resource files have been copied."
        )
        set(RESOURCE_FILES_RECEIPT "${${target}_RESOURCE_FILES_RECEIPT_OUTPUT_PATH}")
        _make_abs_paths(XBE_RESOURCE_FILES)
        add_custom_command(
                OUTPUT "${RESOURCE_FILES_RECEIPT}"
                COMMAND
                "${CMAKE_COMMAND}" -E copy_if_different "${XBE_RESOURCE_FILES}" "${XBE_STAGING_DIR}"
                COMMAND
                "${CMAKE_COMMAND}" -E touch "${RESOURCE_FILES_RECEIPT}"
                DEPENDS ${XBE_RESOURCE_FILES}
        )
    endif ()

    set(
            "${target}_RESOURCE_DIRS_RECEIPT_OUTPUT_PATH"
            CACHE
            INTERNAL
            "Timestamp file indicating that resource directories have been copied."
    )
    if (XBE_RESOURCE_DIRS)
        set(
                "${target}_RESOURCE_DIRS_RECEIPT_OUTPUT_PATH"
                "${CMAKE_CURRENT_BINARY_DIR}/xbe/.${target}_resource_dirs_time"
                CACHE INTERNAL
                "Timestamp file indicating that resource directories have been copied."
        )
        set(RESOURCE_DIRS_RECEIPT "${${target}_RESOURCE_DIRS_RECEIPT_OUTPUT_PATH}")
        _make_abs_paths(XBE_RESOURCE_DIRS)
        add_custom_command(
                OUTPUT "${RESOURCE_DIRS_RECEIPT}"
                COMMAND
                "${CMAKE_COMMAND}" -E env pwd
                COMMAND
                "${CMAKE_COMMAND}" -E copy_directory ${XBE_RESOURCE_DIRS} "${XBE_STAGING_DIR}"
                COMMAND
                "${CMAKE_COMMAND}" -E touch "${RESOURCE_DIRS_RECEIPT}"
                DEPENDS ${XBE_RESOURCE_DIRS}
        )
    endif ()
endfunction()

function(add_xiso)
    cmake_parse_arguments(
            PARSE_ARGV
            2
            "XISO"
            ""
            "XISO"
            ""
    )

    if (${ARGC} LESS 1)
        message(FATAL_ERROR "Missing required 'target' parameter.")
    elseif (${ARGC} LESS 2)
        message(FATAL_ERROR "Missing required 'xbe_target' parameter.")
    endif ()
    set(target "${ARGV0}")
    set(xbe_target "${ARGV1}")

    if (NOT XISO_XISO)
        set(XISO_XISO "${target}.iso")
    endif ()

    set(XBE_STAGING_DIR "${${xbe_target}_XBE_STAGING_DIR}")
    set(XBE_OUTPUT_PATH "${${xbe_target}_XBE_OUTPUT_PATH}")
    set(XBE_RESOURCE_FILES_RECEIPT "${${xbe_target}_RESOURCE_FILES_RECEIPT_OUTPUT_PATH}")
    set(XBE_RESOURCE_DIRS_RECEIPT "${${xbe_target}_RESOURCE_DIRS_RECEIPT_OUTPUT_PATH}")
    set(XISO_STAGING_DIR "${CMAKE_CURRENT_BINARY_DIR}/xiso/${target}")
    file(MAKE_DIRECTORY "${XISO_STAGING_DIR}")
    set(XISO_OUTPUT_PATH "${XISO_STAGING_DIR}/${XISO_XISO}")

    add_custom_command(
            OUTPUT "${XISO_OUTPUT_PATH}"
            COMMAND "${EXTRACT_XISO_TOOL_PATH}" -c "${XBE_STAGING_DIR}" "${XISO_OUTPUT_PATH}"
            DEPENDS
            "${XBE_OUTPUT_PATH}"
            "${XBE_RESOURCE_FILES_RECEIPT}"
            "${XBE_RESOURCE_DIRS_RECEIPT}"
    )

    add_custom_target(
            "${target}"
            ALL
            DEPENDS
            "${XISO_OUTPUT_PATH}")
endfunction()
