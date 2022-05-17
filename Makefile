XBE_TITLE = nxdk_pgraph_tests
GEN_XISO = $(XBE_TITLE).iso
NXDK_DIR ?= $(CURDIR)/third_party/nxdk
NXDK_SDL = y
NXDK_CXX = y

RESOURCEDIR = $(CURDIR)/resources
SRCDIR = $(CURDIR)/src
THIRDPARTYDIR = $(CURDIR)/third_party

SRCS = \
	$(SRCDIR)/debug_output.cpp \
	$(SRCDIR)/logger.cpp \
	$(SRCDIR)/main.cpp \
	$(SRCDIR)/math3d.c \
	$(SRCDIR)/pbkit_ext.cpp \
	$(SRCDIR)/menu_item.cpp \
	$(SRCDIR)/shaders/orthographic_vertex_shader.cpp \
	$(SRCDIR)/shaders/perspective_vertex_shader.cpp \
	$(SRCDIR)/shaders/pixel_shader_program.cpp \
	$(SRCDIR)/shaders/precalculated_vertex_shader.cpp \
	$(SRCDIR)/shaders/projection_vertex_shader.cpp \
	$(SRCDIR)/shaders/vertex_shader_program.cpp \
	$(SRCDIR)/test_driver.cpp \
	$(SRCDIR)/test_host.cpp \
	$(SRCDIR)/tests/attribute_carryover_tests.cpp \
	$(SRCDIR)/tests/attribute_explicit_setter_tests.cpp \
	$(SRCDIR)/tests/attribute_float_tests.cpp \
	$(SRCDIR)/tests/clear_tests.cpp \
	$(SRCDIR)/tests/color_mask_blend_tests.cpp \
	$(SRCDIR)/tests/color_zeta_overlap_tests.cpp \
	$(SRCDIR)/tests/combiner_tests.cpp \
	$(SRCDIR)/tests/depth_format_fixed_function_tests.cpp \
	$(SRCDIR)/tests/depth_format_tests.cpp \
	$(SRCDIR)/tests/fog_tests.cpp \
	$(SRCDIR)/tests/front_face_tests.cpp \
	$(SRCDIR)/tests/image_blit_tests.cpp \
	$(SRCDIR)/tests/lighting_normal_tests.cpp \
	$(SRCDIR)/tests/material_alpha_tests.cpp \
	$(SRCDIR)/tests/material_color_tests.cpp \
	$(SRCDIR)/tests/material_color_source_tests.cpp \
	$(SRCDIR)/tests/overlapping_draw_modes_tests.cpp \
	$(SRCDIR)/tests/set_vertex_data_tests.cpp \
	$(SRCDIR)/tests/surface_clip_tests.cpp \
	$(SRCDIR)/tests/test_suite.cpp \
	$(SRCDIR)/tests/texgen_matrix_tests.cpp \
	$(SRCDIR)/tests/texgen_tests.cpp \
	$(SRCDIR)/tests/texture_border_tests.cpp \
	$(SRCDIR)/tests/texture_format_tests.cpp \
	$(SRCDIR)/tests/texture_framebuffer_blit_tests.cpp \
	$(SRCDIR)/tests/texture_matrix_tests.cpp \
	$(SRCDIR)/tests/texture_render_target_tests.cpp \
	$(SRCDIR)/tests/texture_render_update_in_place_tests.cpp \
	$(SRCDIR)/tests/texture_shadow_comparator_tests.cpp \
	$(SRCDIR)/tests/texture_signed_component_tests.cpp \
	$(SRCDIR)/tests/three_d_primitive_tests.cpp \
	$(SRCDIR)/tests/two_d_line_tests.cpp \
	$(SRCDIR)/tests/vertex_shader_independence_tests.cpp \
	$(SRCDIR)/tests/vertex_shader_rounding_tests.cpp \
	$(SRCDIR)/tests/volume_texture_tests.cpp \
	$(SRCDIR)/tests/w_param_tests.cpp \
	$(SRCDIR)/tests/window_clip_tests.cpp \
	$(SRCDIR)/tests/zero_stride_tests.cpp \
	$(SRCDIR)/texture_format.cpp \
	$(SRCDIR)/texture_generator.cpp \
	$(SRCDIR)/texture_stage.cpp \
	$(SRCDIR)/vertex_buffer.cpp \
	$(THIRDPARTYDIR)/swizzle.c \
	$(THIRDPARTYDIR)/printf/printf.c \
	$(THIRDPARTYDIR)/fpng/src/fpng.cpp

SHADER_OBJS = \
	$(SRCDIR)/shaders/attribute_carryover_test.inl \
	$(SRCDIR)/shaders/attribute_explicit_setter_tests.inl \
	$(SRCDIR)/shaders/mul_col0_by_const0_vertex_shader.inl \
	$(SRCDIR)/shaders/fog_infinite_fogc_test.inl \
	$(SRCDIR)/shaders/fog_vec4_unset.inl \
	$(SRCDIR)/shaders/fog_vec4_x.inl \
	$(SRCDIR)/shaders/fog_vec4_y.inl \
	$(SRCDIR)/shaders/fog_vec4_z.inl \
	$(SRCDIR)/shaders/fog_vec4_w.inl \
	$(SRCDIR)/shaders/fog_vec4_wx.inl \
	$(SRCDIR)/shaders/fog_vec4_wy.inl \
	$(SRCDIR)/shaders/fog_vec4_wzyx.inl \
	$(SRCDIR)/shaders/fog_vec4_xyzw.inl \
	$(SRCDIR)/shaders/precalculated_vertex_shader_2c_texcoords.inl \
	$(SRCDIR)/shaders/precalculated_vertex_shader_4c_texcoords.inl \
	$(SRCDIR)/shaders/projection_vertex_shader.inl \
	$(SRCDIR)/shaders/projection_vertex_shader_no_lighting.inl \
	$(SRCDIR)/shaders/projection_vertex_shader_no_lighting_4c_texcoords.inl \
	$(SRCDIR)/shaders/textured_pixelshader.inl \
	$(SRCDIR)/shaders/untextured_pixelshader.inl

CFLAGS += -I$(SRCDIR) -I$(THIRDPARTYDIR)
CXXFLAGS += -I$(SRCDIR) -I$(THIRDPARTYDIR) -DFPNG_NO_STDIO=1 -DFPNG_NO_SSE=1

ifneq ($(DEBUG),y)
CFLAGS += -O3 -fno-strict-aliasing
CXXFLAGS += -O3 -fno-strict-aliasing
endif

# Disable automatic test execution if no input is detected.
DISABLE_AUTORUN ?= n
# Remove the delay for input before starting automated testing.
AUTORUN_IMMEDIATELY ?= n
ifeq ($(DISABLE_AUTORUN),y)
CXXFLAGS += -DDISABLE_AUTORUN
else
ifeq ($(AUTORUN_IMMEDIATELY),y)
CXXFLAGS += -DAUTORUN_IMMEDIATELY
endif
endif

# Cause the program to shut down the xbox on completion instead of rebooting.
ENABLE_SHUTDOWN ?= n
ifeq ($(ENABLE_SHUTDOWN),y)
CXXFLAGS += -DENABLE_SHUTDOWN
endif

# Optionally set the root path at which test results will be written when running
# from read-only media.
# E.g., "c:"
ifdef FALLBACK_OUTPUT_ROOT_PATH
CXXFLAGS += -DFALLBACK_OUTPUT_ROOT_PATH="\"$(FALLBACK_OUTPUT_ROOT_PATH)\""
endif

# Set the path to a configuration file containing the names of tests that should
# be enabled, one per line.
# E.g., "c:/pgraph_tests.cnf"
ifdef RUNTIME_CONFIG_PATH
CXXFLAGS += -DRUNTIME_CONFIG_PATH="\"$(RUNTIME_CONFIG_PATH)\""
endif

# Cause a runtime config file enabling all tests to be generated in the standard results directory.
DUMP_CONFIG_FILE ?= n
ifeq ($(DUMP_CONFIG_FILE),y)
CXXFLAGS += -DDUMP_CONFIG_FILE
endif

# Cause a log file to be created and updated with metrics at the start and end of each test.
ENABLE_PROGRESS_LOG ?= n
ifeq ($(ENABLE_PROGRESS_LOG),y)
CXXFLAGS += -DENABLE_PROGRESS_LOG
endif

CLEANRULES = clean-resources
include $(NXDK_DIR)/Makefile

PBKIT_DEBUG ?= n
ifeq ($(PBKIT_DEBUG),y)
NXDK_CFLAGS += -DDBG
endif

XBDM_GDB_BRIDGE := xbdm
REMOTE_PATH := e:\\pgraph
XBOX ?=
.phony: deploy
deploy: $(OUTPUT_DIR)/default.xbe
	$(XBDM_GDB_BRIDGE) $(XBOX) -- mkdir $(REMOTE_PATH)
	# TODO: Support moving the actual changed files.
	# This hack will only work if the default.xbe changes when any resource changes.
	$(XBDM_GDB_BRIDGE) $(XBOX) -- putfile $(OUTPUT_DIR)/ $(REMOTE_PATH) -f

.phony: execute
execute: deploy
	$(XBDM_GDB_BRIDGE) $(XBOX) -s -- /run $(REMOTE_PATH)

.phony: debug_bridge_no_deploy
debug_bridge_no_deploy:
	$(XBDM_GDB_BRIDGE) $(XBOX) -s -- gdb :1999 '&&' /launch $(REMOTE_PATH)

.phony: debug_bridge
debug_bridge: deploy debug_bridge_no_deploy

RESOURCE_FILES = $(shell find $(RESOURCEDIR)/ -type f)
RESOURCES = \
	$(patsubst $(RESOURCEDIR)/%,$(OUTPUT_DIR)/%,$(RESOURCE_FILES))

TARGET += $(RESOURCES)
$(GEN_XISO): $(RESOURCES)

$(OUTPUT_DIR)/%: $(RESOURCEDIR)/%
	$(VE)mkdir -p '$(dir $@)'
	$(VE)cp -r '$<' '$@'

.PHONY: clean-resources
clean-resources:
	$(VE)rm -rf $(OUTPUT_DIR)/resources
