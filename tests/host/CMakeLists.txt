set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/host-toolchain.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(_CMAKE_PROCESSING_LANGUAGE "CXX")

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/b514bdc898e2951020cbdca1304b75f5950d1f59.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

configure_file("${CMAKE_SOURCE_DIR}/src/configure.h.in" configure.h)

macro(set_common_target_options TARGET_NAME)
    target_include_directories(
            "${TARGET_NAME}"
            PRIVATE
            .
            stubs
            "${CMAKE_SOURCE_DIR}/src"
            "${CMAKE_SOURCE_DIR}/third_party"
            "${CMAKE_CURRENT_BINARY_DIR}"
    )
endmacro()


#
# RuntimeConfig tests
#
add_library(
        runtime_config
        "${CMAKE_SOURCE_DIR}/src/debug_output.cpp"
        "${CMAKE_SOURCE_DIR}/src/debug_output.h"
        "${CMAKE_SOURCE_DIR}/src/nxdk_ext.h"
        "${CMAKE_SOURCE_DIR}/src/runtime_config.cpp"
        "${CMAKE_SOURCE_DIR}/src/runtime_config.h"
        "${CMAKE_SOURCE_DIR}/src/test_host.h"
        "${CMAKE_SOURCE_DIR}/src/tests/test_suite.h"
        "${CMAKE_SOURCE_DIR}/src/texture_format.cpp"
        "${CMAKE_SOURCE_DIR}/src/texture_format.h"
)
set_common_target_options(runtime_config)
target_link_libraries(
        runtime_config
        PRIVATE
        printf
        xbox_math3d
        tiny-json
)

add_executable(
        test_runtime_config
        test_runtime_config.cpp
)
set_common_target_options(test_runtime_config)
target_link_libraries(test_runtime_config
        runtime_config
        xbox_math3d
        GTest::gmock_main
)

gtest_discover_tests(test_runtime_config)
