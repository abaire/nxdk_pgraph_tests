cmake_minimum_required(VERSION 3.21)

include(NV2A_VSH REQUIRED)
include(NV20_CG REQUIRED)
include(XBEUtils REQUIRED)

find_package(NXDK REQUIRED)
find_package(NXDK_SDL2 REQUIRED)
find_package(NXDK_SDL2_Image REQUIRED)
find_package(Threads REQUIRED)

configure_file(configure.h.in configure.h)

include(FetchContent)
FetchContent_Declare(
        nxdkftplib
        GIT_REPOSITORY https://github.com/abaire/nxdk_ftp_client_lib.git
        GIT_TAG a6c469260b5b1dc17bad7ea68f4ed9f7d3940853
)
FetchContent_MakeAvailable(nxdkftplib)

macro(set_opt_compile_and_link_options TARGET_NAME)
    target_compile_options(
            "${TARGET_NAME}"
            PRIVATE
            -O3
            -fno-strict-aliasing
            -Wall
            $<$<COMPILE_LANGUAGE:CXX>:-Wno-builtin-macro-redefined>   # Suppress warning from NXDK undef of __STDC_NO_THREADS__
            -D_USE_MATH_DEFINES
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
                -D_USE_MATH_DEFINES
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
        shaders/attribute_carryover_tests.vsh
        shaders/fixed_function_approximation_shader.vsh
        shaders/fog_infinite_fogc_test.vsh
        shaders/fog_vec4_w.vsh
        shaders/fog_vec4_w_x.vsh
        shaders/fog_vec4_w_y.vsh
        shaders/fog_vec4_w_z_y_x.vsh
        shaders/fog_vec4_x.vsh
        shaders/fog_vec4_xw.vsh
        shaders/fog_vec4_xy.vsh
        shaders/fog_vec4_xyz.vsh
        shaders/fog_vec4_x_y_z_w.vsh
        shaders/fog_vec4_xyzw.vsh
        shaders/fog_vec4_xz.vsh
        shaders/fog_vec4_xzw.vsh
        shaders/fog_vec4_y.vsh
        shaders/fog_vec4_yw.vsh
        shaders/fog_vec4_yz.vsh
        shaders/fog_vec4_yzw.vsh
        shaders/fog_vec4_z.vsh
        shaders/fog_vec4_zw.vsh
        shaders/fog_vec4_unset.vsh
        shaders/passthrough.vsh
        shaders/projection_vertex_shader_colorless_light.vsh
        shaders/projection_vertex_shader_no_lighting.vsh
)

generate_pixelshader_inl_files(
        generated_fp20_pixel_shaders
        SOURCES
        shaders/textured_pixelshader.ps.cg
        shaders/untextured_pixelshader.ps.cg
)

generate_vertexshader_inl_files(
        generated_vp20_vertex_shaders
        SOURCES
        shaders/attribute_explicit_setter_tests.vs.cg
        shaders/mul_col0_by_const0_vertex_shader.vs.cg
        shaders/precalculated_vertex_shader_2c_texcoords.vs.cg
        shaders/precalculated_vertex_shader_4c_texcoords.vs.cg
)

# Sources that should be optimized regardless of standard debug settings.
add_library(
        optimized_sources
        STATIC
        dds_image.cpp
        dds_image.h
        debug_output.cpp
        debug_output.h
        ftp_logger.cpp
        ftp_logger.h
        main.cpp
        menu_item.cpp
        menu_item.h
        models/flat_mesh_grid_model.cpp
        models/flat_mesh_grid_model.h
        models/light_control_test_mesh_cone_model.cpp
        models/light_control_test_mesh_cone_model.h
        models/light_control_test_mesh_cylinder_model.cpp
        models/light_control_test_mesh_cylinder_model.h
        models/light_control_test_mesh_sphere_model.cpp
        models/light_control_test_mesh_sphere_model.h
        models/light_control_test_mesh_suzanne_model.cpp
        models/light_control_test_mesh_suzanne_model.h
        models/light_control_test_mesh_torus_model.cpp
        models/light_control_test_mesh_torus_model.h
        models/model_builder.cpp
        models/model_builder.h
        light.cpp
        light.h
        logger.cpp
        logger.h
        pbkit_ext.cpp
        pbkit_ext.h
        pgraph_diff_token.cpp
        pgraph_diff_token.h
        runtime_config.cpp
        runtime_config.h
        shaders/orthographic_vertex_shader.cpp
        shaders/orthographic_vertex_shader.h
        shaders/perspective_vertex_shader.cpp
        shaders/perspective_vertex_shader.h
        shaders/pixel_shader_program.cpp
        shaders/pixel_shader_program.h
        shaders/passthrough_vertex_shader.cpp
        shaders/passthrough_vertex_shader.h
        shaders/precalculated_vertex_shader.cpp
        shaders/precalculated_vertex_shader.h
        shaders/projection_vertex_shader.cpp
        shaders/projection_vertex_shader.h
        shaders/vertex_shader_program.cpp
        shaders/vertex_shader_program.h
        test_driver.cpp
        test_driver.h
        test_host.cpp
        test_host.h
        texture_format.cpp
        texture_format.h
        texture_generator.cpp
        texture_generator.h
        texture_stage.cpp
        texture_stage.h
        vertex_buffer.cpp
        vertex_buffer.h
)

if (NOT NO_OPT)
    set_opt_compile_and_link_options(optimized_sources)
else ()
    set_compile_and_link_options(optimized_sources)
endif ()

target_include_directories(
        optimized_sources
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/third_party"
        "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(
        optimized_sources
        PUBLIC
        generated_fp20_pixel_shaders
        generated_vp20_vertex_shaders
        generated_nv2a_vertex_shaders
        nxdk_ftp_client_lib::client
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
        tests/antialiasing_tests.cpp
        tests/antialiasing_tests.h
        tests/alpha_func_tests.cpp
        tests/alpha_func_tests.h
        tests/attribute_carryover_tests.cpp
        tests/attribute_carryover_tests.h
        tests/attribute_explicit_setter_tests.cpp
        tests/attribute_explicit_setter_tests.h
        tests/attribute_float_tests.cpp
        tests/attribute_float_tests.h
        tests/blend_tests.cpp
        tests/blend_tests.h
        tests/clear_tests.cpp
        tests/clear_tests.h
        tests/color_key_tests.cpp
        tests/color_key_tests.h
        tests/color_mask_blend_tests.cpp
        tests/color_mask_blend_tests.h
        tests/color_zeta_disable_tests.cpp
        tests/color_zeta_disable_tests.h
        tests/color_zeta_overlap_tests.cpp
        tests/color_zeta_overlap_tests.h
        tests/combiner_tests.cpp
        tests/combiner_tests.h
        tests/depth_format_fixed_function_tests.cpp
        tests/depth_format_fixed_function_tests.h
        tests/depth_format_tests.cpp
        tests/depth_format_tests.h
        tests/dma_corruption_around_surface_tests.cpp
        tests/dma_corruption_around_surface_tests.h
        tests/edge_flag_tests.cpp
        tests/edge_flag_tests.h
        tests/fog_tests.cpp
        tests/fog_tests.h
        tests/front_face_tests.cpp
        tests/front_face_tests.h
        tests/image_blit_tests.cpp
        tests/image_blit_tests.h
        tests/inline_array_size_mismatch.cpp
        tests/inline_array_size_mismatch.h
        tests/lighting_control_tests.cpp
        tests/lighting_control_tests.h
        tests/lighting_normal_tests.cpp
        tests/lighting_normal_tests.h
        tests/lighting_range_tests.cpp
        tests/lighting_range_tests.h
        tests/lighting_spotlight_tests.cpp
        tests/lighting_spotlight_tests.h
        tests/lighting_two_sided_tests.cpp
        tests/lighting_two_sided_tests.h
        tests/line_width_tests.cpp
        tests/line_width_tests.h
        tests/material_alpha_tests.cpp
        tests/material_alpha_tests.h
        tests/material_color_source_tests.cpp
        tests/material_color_source_tests.h
        tests/material_color_tests.cpp
        tests/material_color_tests.h
        tests/null_surface_tests.cpp
        tests/null_surface_tests.h
        tests/overlapping_draw_modes_tests.cpp
        tests/overlapping_draw_modes_tests.h
        tests/point_params_tests.cpp
        tests/point_params_tests.h
        tests/point_size_tests.cpp
        tests/point_size_tests.h
        tests/pvideo_tests.cpp
        tests/pvideo_tests.h
        tests/set_vertex_data_tests.cpp
        tests/set_vertex_data_tests.h
        tests/shade_model_tests.cpp
        tests/shade_model_tests.h
        tests/smoothing_tests.cpp
        tests/smoothing_tests.h
        tests/specular_back_tests.cpp
        tests/specular_back_tests.h
        tests/specular_tests.cpp
        tests/specular_tests.h
        tests/stencil_tests.cpp
        tests/stencil_tests.h
        tests/stencil_func_tests.cpp
        tests/stencil_func_tests.h
        tests/stipple_tests.cpp
        tests/stipple_tests.h
        tests/swath_width_tests.cpp
        tests/swath_width_tests.h
        tests/surface_clip_tests.cpp
        tests/surface_clip_tests.h
        tests/surface_pitch_tests.cpp
        tests/surface_pitch_tests.h
        tests/test_suite.cpp
        tests/test_suite.h
        tests/texgen_matrix_tests.cpp
        tests/texgen_matrix_tests.h
        tests/texgen_tests.cpp
        tests/texgen_tests.h
        tests/texture_border_tests.cpp
        tests/texture_border_tests.h
        tests/texture_cpu_update_tests.cpp
        tests/texture_cpu_update_tests.h
        tests/texture_cubemap_tests.cpp
        tests/texture_cubemap_tests.h
        tests/texture_format_dxt_tests.cpp
        tests/texture_format_dxt_tests.h
        tests/texture_format_tests.cpp
        tests/texture_format_tests.h
        tests/texture_framebuffer_blit_tests.cpp
        tests/texture_framebuffer_blit_tests.h
        tests/texture_matrix_tests.cpp
        tests/texture_matrix_tests.h
        tests/texture_perspective_enable_tests.cpp
        tests/texture_perspective_enable_tests.h
        tests/texture_render_target_tests.cpp
        tests/texture_render_target_tests.h
        tests/texture_render_update_in_place_tests.cpp
        tests/texture_render_update_in_place_tests.h
        tests/texture_shadow_comparator_tests.cpp
        tests/texture_shadow_comparator_tests.h
        tests/texture_signed_component_tests.cpp
        tests/texture_signed_component_tests.h
        tests/three_d_primitive_tests.cpp
        tests/three_d_primitive_tests.h
        tests/two_d_line_tests.cpp
        tests/two_d_line_tests.h
        tests/vertex_shader_independence_tests.cpp
        tests/vertex_shader_independence_tests.h
        tests/vertex_shader_rounding_tests.cpp
        tests/vertex_shader_rounding_tests.h
        tests/vertex_shader_swizzle_tests.cpp
        tests/vertex_shader_swizzle_tests.h
        tests/viewport_tests.cpp
        tests/viewport_tests.h
        tests/volume_texture_tests.cpp
        tests/volume_texture_tests.h
        tests/w_param_tests.cpp
        tests/w_param_tests.h
        tests/window_clip_tests.cpp
        tests/window_clip_tests.h
        tests/z_min_max_control_tests.cpp
        tests/z_min_max_control_tests.h
        tests/zero_stride_tests.cpp
        tests/zero_stride_tests.h
)

# Pull debug info out of the binary into a host-side linked binary.
split_debug(nxdk_pgraph_tests)

set(EXECUTABLE_BINARY "${CMAKE_CURRENT_BINARY_DIR}/nxdk_pgraph_tests.exe")
set_compile_and_link_options(nxdk_pgraph_tests)
target_include_directories(
        nxdk_pgraph_tests
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/third_party"
        "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(
        nxdk_pgraph_tests
        PRIVATE
        fpng
        optimized_sources
        printf
        swizzle
        xbox_math3d
        NXDK::NXDK
        NXDK::NXDK_CXX
        NXDK::SDL2
        NXDK::SDL2_Image
        nxdk_ftp_client_lib::client
)

add_xbe(
        xbe_file "${EXECUTABLE_BINARY}"
        TITLE "PGRAPH Tests"
        RESOURCE_ROOTS
        "${CMAKE_SOURCE_DIR}/resources"
        RESOURCE_DIRS
        "${CMAKE_SOURCE_DIR}/resources"
)
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
#        COMMAND "${XBDM_GDB_BRIDGE}" "${XBOX}" -- %syncdir "${XBE_STAGING_DIR}" "${REMOTE_PATH}" -d
#        DEPENDS "${XBE_OUTPUT_PATH}"
#)
