cmake_minimum_required(VERSION 3.18)
project(nxdk_pgraph_tests)

set(
        CMAKE_MODULE_PATH
        ${CMAKE_MODULE_PATH}
        "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/"
)

set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(_CMAKE_PROCESSING_LANGUAGE "CXX")

include(CheckSymbolExists)
include(FindPkgConfig)

# Determine if this build is for the Xbox or the host system.
if (CMAKE_TOOLCHAIN_FILE MATCHES "toolchain-nxdk.cmake")
    set(IS_TARGET_BUILD ON)
else ()
    set(IS_TARGET_BUILD OFF)
endif ()


add_subdirectory(third_party)


option(
        NO_OPT
        "Suppress optimization on core files that are typically optimized regardless of release/debug configuration"
        OFF
)

option(
        ENABLE_PROGRESS_LOG
        "Write the name of each test to a log file on the target."
        OFF
)

option(
        ENABLE_INTERACTIVE_CRASH_AVOIDANCE
        "Attempt to detect crashes and prevent crashing tests from running after the next restart."
        OFF
)

option(
        DUMP_CONFIG_FILE
        "Write a default config file and exit instead of running tests."
        OFF
)

option(
        DISABLE_AUTORUN
        "Prevents the tests from being executed without user input."
        OFF
)

option(
        AUTORUN_IMMEDIATELY
        "Remove the delay for input before starting automated testing."
        OFF
)

option(
        ENABLE_SHUTDOWN
        "Cause the program to shut down the xbox on completion instead of rebooting."
        OFF
)

option(
        ENABLE_MULTIFRAME_CPU_BLIT_TEST
        "Enables the MultiframeCPUBlit test within the Antialiasing tests. This test requires evaluation across multiple frames and thus cannot produce a usable golden artifact for automated validation. This should only be enabled for manual testing purposes."
        OFF
)

option(
        ENABLE_PGRAPH_REGION_DIFF
        "Causes a diff of the nv2a PGRAPH registers to be done between the start and end of each test in order to detect state leakage. Output is logged to XBDM and will be written into the progress log if it is enabled."
        OFF
)

set(
        RUNTIME_CONFIG_PATH
        "e:/nxdk_pgraph_tests/nxdk_pgraph_tests_config.json"
        CACHE STRING
        "Absolute XBOX-path to the location of a JSON configuration file to set options and filter the tests that are executed."
)

set(
        DEFAULT_OUTPUT_DIRECTORY_PATH
        "e:/nxdk_pgraph_tests"
        CACHE STRING
        "Absolute XBOX-path to a directory into which test output will be written."
)

option(
        SKIP_TESTS_BY_DEFAULT
        "Causes all tests whose names do not appear in the runtime config to be skipped when automatically executing tests."
        OFF
)

if (IS_TARGET_BUILD)
    add_subdirectory(src)
else ()
    add_subdirectory(tests/host)
endif ()
