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

include(NV2A_VSH REQUIRED)
include(NV20_CG REQUIRED)
include(XBEUtils REQUIRED)

find_package(NXDK REQUIRED)
find_package(NXDK_SDL2 REQUIRED)
find_package(NXDK_SDL2_Image REQUIRED)
find_package(Threads REQUIRED)

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

configure_file(src/configure.h.in src/configure.h)

macro(set_opt_compile_and_link_options TARGET_NAME)
    target_compile_options(
            "${TARGET_NAME}"
            PRIVATE
            -O3
            -fno-strict-aliasing
            -Wall
            $<$<COMPILE_LANGUAGE:CXX>:-Wno-builtin-macro-redefined>   # Suppress warning from NXDK undef of __STDC_NO_THREADS__
    )
    target_link_options(
            "${TARGET_NAME}"
            PRIVATE
            "/debug:none"
    )
endmacro()

macro(set_compile_and_link_options TARGET_NAME)
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        target_compile_options(
                "${TARGET_NAME}"
                PRIVATE
                -g
                -gdwarf-4
                -O0
                -Wall
                -fstandalone-debug
                $<$<COMPILE_LANGUAGE:CXX>:-Wno-builtin-macro-redefined>   # Suppress warning from NXDK undef of __STDC_NO_THREADS__
        )
        target_link_options(
                "${TARGET_NAME}"
                PRIVATE
                "/debug:full"
        )
    else ()
        set_opt_compile_and_link_options("${TARGET_NAME}")
    endif ()
endmacro()

# Vertex shaders.
generate_nv2a_vshinc_files(
        generated_nv2a_vertex_shaders
        SOURCES
        src/shaders/fog_infinite_fogc_test.vsh
        src/shaders/fog_vec4_w.vsh
        src/shaders/fog_vec4_w_x.vsh
        src/shaders/fog_vec4_w_y.vsh
        src/shaders/fog_vec4_w_z_y_x.vsh
        src/shaders/fog_vec4_x.vsh
        src/shaders/fog_vec4_xw.vsh
        src/shaders/fog_vec4_xy.vsh
        src/shaders/fog_vec4_xyz.vsh
        src/shaders/fog_vec4_x_y_z_w.vsh
        src/shaders/fog_vec4_xyzw.vsh
        src/shaders/fog_vec4_xz.vsh
        src/shaders/fog_vec4_xzw.vsh
        src/shaders/fog_vec4_y.vsh
        src/shaders/fog_vec4_yw.vsh
        src/shaders/fog_vec4_yz.vsh
        src/shaders/fog_vec4_yzw.vsh
        src/shaders/fog_vec4_z.vsh
        src/shaders/fog_vec4_zw.vsh
        src/shaders/fog_vec4_unset.vsh
        src/shaders/passthrough.vsh
)

generate_pixelshader_inl_files(
        generated_fp20_pixel_shaders
        SOURCES
        src/shaders/textured_pixelshader.ps.cg
        src/shaders/untextured_pixelshader.ps.cg
)

generate_vertexshader_inl_files(
        generated_vp20_vertex_shaders
        SOURCES
        src/shaders/attribute_carryover_test.vs.cg
        src/shaders/attribute_explicit_setter_tests.vs.cg
        src/shaders/mul_col0_by_const0_vertex_shader.vs.cg
        src/shaders/precalculated_vertex_shader_2c_texcoords.vs.cg
        src/shaders/precalculated_vertex_shader_4c_texcoords.vs.cg
        src/shaders/projection_vertex_shader.vs.cg
        src/shaders/projection_vertex_shader_no_lighting.vs.cg
        src/shaders/projection_vertex_shader_no_lighting_4c_texcoords.vs.cg
)

# Sources that should be optimized regardless of standard debug settings.
add_library(
        optimized_sources
        STATIC
        src/dds_image.cpp
        src/dds_image.h
        src/debug_output.cpp
        src/debug_output.h
        src/main.cpp
        src/menu_item.cpp
        src/menu_item.h
        src/logger.cpp
        src/logger.h
        src/pbkit_ext.cpp
        src/pbkit_ext.h
        src/pgraph_diff_token.cpp
        src/pgraph_diff_token.h
        src/runtime_config.cpp
        src/runtime_config.h
        src/shaders/orthographic_vertex_shader.cpp
        src/shaders/orthographic_vertex_shader.h
        src/shaders/perspective_vertex_shader.cpp
        src/shaders/perspective_vertex_shader.h
        src/shaders/pixel_shader_program.cpp
        src/shaders/pixel_shader_program.h
        src/shaders/passthrough_vertex_shader.cpp
        src/shaders/passthrough_vertex_shader.h
        src/shaders/precalculated_vertex_shader.cpp
        src/shaders/precalculated_vertex_shader.h
        src/shaders/projection_vertex_shader.cpp
        src/shaders/projection_vertex_shader.h
        src/shaders/vertex_shader_program.cpp
        src/shaders/vertex_shader_program.h
        src/test_driver.cpp
        src/test_driver.h
        src/test_host.cpp
        src/test_host.h
        src/texture_format.cpp
        src/texture_format.h
        src/texture_generator.cpp
        src/texture_generator.h
        src/texture_stage.cpp
        src/texture_stage.h
        src/vertex_buffer.cpp
        src/vertex_buffer.h
)

if (NOT NO_OPT)
    set_opt_compile_and_link_options(optimized_sources)
else ()
    set_compile_and_link_options(optimized_sources)
endif ()

target_include_directories(
        optimized_sources
        PRIVATE
        src
        third_party
        "${CMAKE_CURRENT_BINARY_DIR}/src"
)

target_link_libraries(
        optimized_sources
        PUBLIC
        generated_fp20_pixel_shaders
        generated_vp20_vertex_shaders
        generated_nv2a_vertex_shaders
        PRIVATE
        fpng
        printf
        swizzle
        tiny-json
        xbox_math3d
        NXDK::NXDK
        NXDK::NXDK_CXX
        NXDK::SDL2
        NXDK::SDL2_Image
)

add_executable(
        nxdk_pgraph_tests
        src/tests/antialiasing_tests.cpp
        src/tests/antialiasing_tests.h
        src/tests/attribute_carryover_tests.cpp
        src/tests/attribute_carryover_tests.h
        src/tests/attribute_explicit_setter_tests.cpp
        src/tests/attribute_explicit_setter_tests.h
        src/tests/attribute_float_tests.cpp
        src/tests/attribute_float_tests.h
        src/tests/blend_tests.cpp
        src/tests/blend_tests.h
        src/tests/clear_tests.cpp
        src/tests/clear_tests.h
        src/tests/color_key_tests.cpp
        src/tests/color_key_tests.h
        src/tests/color_mask_blend_tests.cpp
        src/tests/color_mask_blend_tests.h
        src/tests/color_zeta_disable_tests.cpp
        src/tests/color_zeta_disable_tests.h
        src/tests/color_zeta_overlap_tests.cpp
        src/tests/color_zeta_overlap_tests.h
        src/tests/combiner_tests.cpp
        src/tests/combiner_tests.h
        src/tests/depth_format_fixed_function_tests.cpp
        src/tests/depth_format_fixed_function_tests.h
        src/tests/depth_format_tests.cpp
        src/tests/depth_format_tests.h
        src/tests/dma_corruption_around_surface_tests.cpp
        src/tests/dma_corruption_around_surface_tests.h
        src/tests/edge_flag_tests.cpp
        src/tests/edge_flag_tests.h
        src/tests/fog_tests.cpp
        src/tests/fog_tests.h
        src/tests/front_face_tests.cpp
        src/tests/front_face_tests.h
        src/tests/image_blit_tests.cpp
        src/tests/image_blit_tests.h
        src/tests/inline_array_size_mismatch.cpp
        src/tests/inline_array_size_mismatch.h
        src/tests/lighting_normal_tests.cpp
        src/tests/lighting_normal_tests.h
        src/tests/lighting_two_sided_tests.cpp
        src/tests/lighting_two_sided_tests.h
        src/tests/line_width_tests.cpp
        src/tests/line_width_tests.h
        src/tests/material_alpha_tests.cpp
        src/tests/material_alpha_tests.h
        src/tests/material_color_source_tests.cpp
        src/tests/material_color_source_tests.h
        src/tests/material_color_tests.cpp
        src/tests/material_color_tests.h
        src/tests/null_surface_tests.cpp
        src/tests/null_surface_tests.h
        src/tests/overlapping_draw_modes_tests.cpp
        src/tests/overlapping_draw_modes_tests.h
        src/tests/pvideo_tests.cpp
        src/tests/pvideo_tests.h
        src/tests/set_vertex_data_tests.cpp
        src/tests/set_vertex_data_tests.h
        src/tests/shade_model_tests.cpp
        src/tests/shade_model_tests.h
        src/tests/smoothing_tests.cpp
        src/tests/smoothing_tests.h
        src/tests/stencil_tests.cpp
        src/tests/stencil_tests.h
        src/tests/stipple_tests.cpp
        src/tests/stipple_tests.h
        src/tests/surface_clip_tests.cpp
        src/tests/surface_clip_tests.h
        src/tests/surface_pitch_tests.cpp
        src/tests/surface_pitch_tests.h
        src/tests/test_suite.cpp
        src/tests/test_suite.h
        src/tests/texgen_matrix_tests.cpp
        src/tests/texgen_matrix_tests.h
        src/tests/texgen_tests.cpp
        src/tests/texgen_tests.h
        src/tests/texture_border_tests.cpp
        src/tests/texture_border_tests.h
        src/tests/texture_cpu_update_tests.cpp
        src/tests/texture_cpu_update_tests.h
        src/tests/texture_cubemap_tests.cpp
        src/tests/texture_cubemap_tests.h
        src/tests/texture_format_dxt_tests.cpp
        src/tests/texture_format_dxt_tests.h
        src/tests/texture_format_tests.cpp
        src/tests/texture_format_tests.h
        src/tests/texture_framebuffer_blit_tests.cpp
        src/tests/texture_framebuffer_blit_tests.h
        src/tests/texture_matrix_tests.cpp
        src/tests/texture_matrix_tests.h
        src/tests/texture_render_target_tests.cpp
        src/tests/texture_render_target_tests.h
        src/tests/texture_render_update_in_place_tests.cpp
        src/tests/texture_render_update_in_place_tests.h
        src/tests/texture_shadow_comparator_tests.cpp
        src/tests/texture_shadow_comparator_tests.h
        src/tests/texture_signed_component_tests.cpp
        src/tests/texture_signed_component_tests.h
        src/tests/three_d_primitive_tests.cpp
        src/tests/three_d_primitive_tests.h
        src/tests/two_d_line_tests.cpp
        src/tests/two_d_line_tests.h
        src/tests/vertex_shader_independence_tests.cpp
        src/tests/vertex_shader_independence_tests.h
        src/tests/vertex_shader_rounding_tests.cpp
        src/tests/vertex_shader_rounding_tests.h
        src/tests/vertex_shader_swizzle_tests.cpp
        src/tests/vertex_shader_swizzle_tests.h
        src/tests/viewport_tests.cpp
        src/tests/viewport_tests.h
        src/tests/volume_texture_tests.cpp
        src/tests/volume_texture_tests.h
        src/tests/w_param_tests.cpp
        src/tests/w_param_tests.h
        src/tests/window_clip_tests.cpp
        src/tests/window_clip_tests.h
        src/tests/z_min_max_control_tests.cpp
        src/tests/z_min_max_control_tests.h
        src/tests/zero_stride_tests.cpp
        src/tests/zero_stride_tests.h
)

# Pull debug info out of the binary into a host-side linked binary.
split_debug(nxdk_pgraph_tests)

set(EXECUTABLE_BINARY "${CMAKE_CURRENT_BINARY_DIR}/nxdk_pgraph_tests.exe")
set_compile_and_link_options(nxdk_pgraph_tests)
target_include_directories(
        nxdk_pgraph_tests
        PRIVATE
        src
        third_party
        "${CMAKE_CURRENT_BINARY_DIR}/src"
)

target_link_libraries(
        nxdk_pgraph_tests
        fpng
        optimized_sources
        printf
        swizzle
        xbox_math3d
        NXDK::NXDK
        NXDK::NXDK_CXX
        NXDK::SDL2
        NXDK::SDL2_Image
)

add_xbe(xbe_file "${EXECUTABLE_BINARY}" TITLE "PGRAPH Tests" RESOURCE_DIRS resources)
add_xiso(nxdk_pgraph_tests_xiso xbe_file)


# Deployment via xbdm_gdb_bridge - https://github.com/abaire/xbdm_gdb_bridge
#set(XBDM_GDB_BRIDGE $ENV{XBDM_GDB_BRIDGE})
#set(XBOX $ENV{XBOX})
#if (DEFINED ENV{REMOTE_PATH})
#    set(REMOTE_PATH $ENV{REMOTE_PATH})
#else ()
#    set(REMOTE_PATH "e:\\\\cmake_pgraph")
#endif ()
#
#add_custom_target(
#        deploy_with_xbdm
#        COMMAND "${XBDM_GDB_BRIDGE}" "${XBOX}" -- mkdir "${REMOTE_PATH}"
#        COMMAND "${XBDM_GDB_BRIDGE}" "${XBOX}" -- putfile "${XBE_STAGING_DIR}" "${REMOTE_PATH}" -f
#        DEPENDS "${XBE_OUTPUT_PATH}"
#)
