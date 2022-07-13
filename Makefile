XBE_TITLE = nxdk_pgraph_tests
GEN_XISO = $(XBE_TITLE).iso
NXDK_DIR ?= $(CURDIR)/third_party/nxdk
NXDK_SDL = y
NXDK_CXX = y

# pip3 install nv2a-vsh
# https://pypi.org/project/nv2a-vsh/
# https://github.com/abaire/nv2a_vsh_asm
NV2AVSH = nv2avsh

RESOURCEDIR = $(CURDIR)/resources
SRCDIR = $(CURDIR)/src
THIRDPARTYDIR = $(CURDIR)/third_party

OPTIMIZED_SRCS = \
	$(SRCDIR)/dds_image.cpp \
	$(SRCDIR)/debug_output.cpp \
	$(SRCDIR)/main.cpp \
	$(SRCDIR)/math3d.c \
	$(SRCDIR)/menu_item.cpp \
	$(SRCDIR)/logger.cpp \
	$(SRCDIR)/pbkit_ext.cpp \
	$(SRCDIR)/pgraph_diff_token.cpp \
	$(SRCDIR)/shaders/orthographic_vertex_shader.cpp \
	$(SRCDIR)/shaders/perspective_vertex_shader.cpp \
	$(SRCDIR)/shaders/pixel_shader_program.cpp \
	$(SRCDIR)/shaders/precalculated_vertex_shader.cpp \
	$(SRCDIR)/shaders/projection_vertex_shader.cpp \
	$(SRCDIR)/shaders/vertex_shader_program.cpp \
	$(SRCDIR)/test_driver.cpp \
	$(SRCDIR)/test_host.cpp \
	$(SRCDIR)/texture_format.cpp \
	$(SRCDIR)/texture_generator.cpp \
	$(SRCDIR)/texture_stage.cpp \
	$(SRCDIR)/vertex_buffer.cpp \
	$(THIRDPARTYDIR)/swizzle.c \
	$(THIRDPARTYDIR)/printf/printf.c \
	$(THIRDPARTYDIR)/fpng/src/fpng.cpp

SRCS = \
	$(SRCDIR)/tests/antialiasing_tests.cpp \
	$(SRCDIR)/tests/attribute_carryover_tests.cpp \
	$(SRCDIR)/tests/attribute_explicit_setter_tests.cpp \
	$(SRCDIR)/tests/attribute_float_tests.cpp \
	$(SRCDIR)/tests/blend_tests.cpp \
	$(SRCDIR)/tests/clear_tests.cpp \
	$(SRCDIR)/tests/color_mask_blend_tests.cpp \
	$(SRCDIR)/tests/color_zeta_disable_tests.cpp \
	$(SRCDIR)/tests/color_zeta_overlap_tests.cpp \
	$(SRCDIR)/tests/combiner_tests.cpp \
	$(SRCDIR)/tests/depth_format_fixed_function_tests.cpp \
	$(SRCDIR)/tests/depth_format_tests.cpp \
	$(SRCDIR)/tests/fog_tests.cpp \
	$(SRCDIR)/tests/front_face_tests.cpp \
	$(SRCDIR)/tests/image_blit_tests.cpp \
	$(SRCDIR)/tests/inline_array_size_mismatch.cpp \
	$(SRCDIR)/tests/lighting_normal_tests.cpp \
	$(SRCDIR)/tests/material_alpha_tests.cpp \
	$(SRCDIR)/tests/material_color_tests.cpp \
	$(SRCDIR)/tests/material_color_source_tests.cpp \
	$(SRCDIR)/tests/null_surface_tests.cpp \
	$(SRCDIR)/tests/overlapping_draw_modes_tests.cpp \
	$(SRCDIR)/tests/pvideo_tests.cpp \
	$(SRCDIR)/tests/set_vertex_data_tests.cpp \
	$(SRCDIR)/tests/shade_model_tests.cpp \
	$(SRCDIR)/tests/stencil_tests.cpp \
	$(SRCDIR)/tests/surface_clip_tests.cpp \
	$(SRCDIR)/tests/surface_pitch_tests.cpp \
	$(SRCDIR)/tests/test_suite.cpp \
	$(SRCDIR)/tests/texgen_matrix_tests.cpp \
	$(SRCDIR)/tests/texgen_tests.cpp \
	$(SRCDIR)/tests/texture_border_tests.cpp \
	$(SRCDIR)/tests/texture_cpu_update_tests.cpp \
	$(SRCDIR)/tests/texture_cubemap_tests.cpp \
	$(SRCDIR)/tests/texture_format_dxt_tests.cpp \
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
	$(SRCDIR)/tests/vertex_shader_swizzle_tests.cpp \
	$(SRCDIR)/tests/volume_texture_tests.cpp \
	$(SRCDIR)/tests/w_param_tests.cpp \
	$(SRCDIR)/tests/window_clip_tests.cpp \
	$(SRCDIR)/tests/zero_stride_tests.cpp

SHADER_OBJS = \
	$(SRCDIR)/shaders/attribute_carryover_test.inl \
	$(SRCDIR)/shaders/attribute_explicit_setter_tests.inl \
	$(SRCDIR)/shaders/mul_col0_by_const0_vertex_shader.inl \
	$(SRCDIR)/shaders/precalculated_vertex_shader_2c_texcoords.inl \
	$(SRCDIR)/shaders/precalculated_vertex_shader_4c_texcoords.inl \
	$(SRCDIR)/shaders/projection_vertex_shader.inl \
	$(SRCDIR)/shaders/projection_vertex_shader_no_lighting.inl \
	$(SRCDIR)/shaders/projection_vertex_shader_no_lighting_4c_texcoords.inl \
	$(SRCDIR)/shaders/textured_pixelshader.inl \
	$(SRCDIR)/shaders/untextured_pixelshader.inl

NV2A_VSH_OBJS = \
	$(SRCDIR)/shaders/fog_infinite_fogc_test.vshinc \
	$(SRCDIR)/shaders/fog_vec4_w.vshinc \
	$(SRCDIR)/shaders/fog_vec4_w_x.vshinc \
	$(SRCDIR)/shaders/fog_vec4_w_y.vshinc \
	$(SRCDIR)/shaders/fog_vec4_w_z_y_x.vshinc \
	$(SRCDIR)/shaders/fog_vec4_x.vshinc \
	$(SRCDIR)/shaders/fog_vec4_xw.vshinc \
	$(SRCDIR)/shaders/fog_vec4_xy.vshinc \
	$(SRCDIR)/shaders/fog_vec4_xyz.vshinc \
	$(SRCDIR)/shaders/fog_vec4_x_y_z_w.vshinc \
	$(SRCDIR)/shaders/fog_vec4_xyzw.vshinc \
	$(SRCDIR)/shaders/fog_vec4_xz.vshinc \
	$(SRCDIR)/shaders/fog_vec4_xzw.vshinc \
	$(SRCDIR)/shaders/fog_vec4_y.vshinc \
	$(SRCDIR)/shaders/fog_vec4_yw.vshinc \
	$(SRCDIR)/shaders/fog_vec4_yz.vshinc \
	$(SRCDIR)/shaders/fog_vec4_yzw.vshinc \
	$(SRCDIR)/shaders/fog_vec4_z.vshinc \
	$(SRCDIR)/shaders/fog_vec4_zw.vshinc \
	$(SRCDIR)/shaders/fog_vec4_unset.vshinc

CFLAGS += -I$(SRCDIR) -I$(THIRDPARTYDIR)
CXXFLAGS += -I$(SRCDIR) -I$(THIRDPARTYDIR) -DFPNG_NO_STDIO=1 -DFPNG_NO_SSE=1

OPTIMIZE_COMPILE_FLAGS = -O3 -fno-strict-aliasing
ifneq ($(DEBUG),y)
CFLAGS += $(OPTIMIZE_COMPILE_FLAGS)
CXXFLAGS += $(OPTIMIZE_COMPILE_FLAGS)
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

# Uses the result of the last progress log to ask the user whether tests that appeared to crash historically should be
# skipped.
ENABLE_INTERACTIVE_CRASH_AVOIDANCE ?= n
ifeq ($(ENABLE_INTERACTIVE_CRASH_AVOIDANCE),y)
ifneq ($(ENABLE_PROGRESS_LOG),y)
$(error ENABLE_INTERACTIVE_CRASH_AVOIDANCE may not be enabled without ENABLE_PROGRESS_LOG)
endif
CXXFLAGS += -DENABLE_INTERACTIVE_CRASH_AVOIDANCE
endif

# Causes a diff of the nv2a PGRAPH registers to be done between the start and end of each test in order to detect state
# leakage. Output is logged to XBDM and will be written into the progress log if it is enabled.
ENABLE_PGRAPH_REGION_DIFF ?= n
ifeq ($(ENABLE_PGRAPH_REGION_DIFF),y)
CXXFLAGS += -DENABLE_PGRAPH_REGION_DIFF
endif

CLEANRULES = clean-resources clean-optimized clean-nv2a-vsh-objs
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

DEPS += $(filter %.c.d, $(OPTIMIZED_SRCS:.c=.c.d))
DEPS += $(filter %.cpp.d, $(OPTIMIZED_SRCS:.cpp=.cpp.d))
$(OPTIMIZED_SRCS): $(SHADER_OBJS) $(NV2A_VSH_OBJS)
OPTIMIZED_CC_SRCS := $(filter %.c,$(OPTIMIZED_SRCS))
OPTIMIZED_CC_OBJS := $(addsuffix .obj, $(basename $(OPTIMIZED_CC_SRCS)))
OPTIMIZED_CXX_SRCS := $(filter %.cpp,$(OPTIMIZED_SRCS))
OPTIMIZED_CXX_OBJS := $(addsuffix .obj, $(basename $(OPTIMIZED_CXX_SRCS)))
OPTIMIZED_AS_SRCS := $(filter %.s,$(OPTIMIZED_SRCS))
OPTIMIZED_AS_OBJS := $(addsuffix .obj, $(basename $(OPTIMIZED_AS_SRCS)))
OPTIMIZED_OBJS := $(OPTIMIZED_CC_OBJS) $(OPTIMIZED_CXX_OBJS) $(OPTIMIZED_AS_OBJS)
OPTIMIZED_FILTER_COMPILE_FLAGS := -g -gdwarf-4

$(OPTIMIZED_CC_OBJS): %.obj: %.c
	@echo "[ CC - OPT ] $@"
	$(VE) $(CC) $(filter-out $(OPTIMIZED_FILTER_COMPILE_FLAGS),$(NXDK_CFLAGS)) $(OPTIMIZE_COMPILE_FLAGS) $(CFLAGS) -MD -MP -MT '$@' -MF '$(patsubst %.obj,%.c.d,$@)' -c -o '$@' '$<'

$(OPTIMIZED_CXX_OBJS): %.obj: %.cpp
	@echo "[ CXX - OPT] $@"
	$(VE) $(CXX) $(filter-out $(OPTIMIZED_FILTER_COMPILE_FLAGS),$(NXDK_CXXFLAGS)) $(OPTIMIZE_COMPILE_FLAGS) $(CXXFLAGS) -MD -MP -MT '$@' -MF '$(patsubst %.obj,%.cpp.d,$@)' -c -o '$@' '$<'

$(OPTIMIZED_AS_OBJS): %.obj: %.s
	@echo "[ AS - OPT ] $@"
	$(VE) $(AS) $(filter-out $(OPTIMIZED_FILTER_COMPILE_FLAGS),$(NXDK_ASFLAGS)) $(OPTIMIZE_COMPILE_FLAGS) $(ASFLAGS) -c -o '$@' '$<'

optimized.lib: $(OPTIMIZED_OBJS)

.PHONY: clean-optimized
clean-optimized:
	$(VE)rm -f optimized.lib $(OPTIMIZED_OBJS)

main.exe: optimized.lib

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
	$(VE)rm -rf $(patsubst $(RESOURCEDIR)/%,$(OUTPUT_DIR)/%,$(RESOURCES))

# nv2avsh assembler rules:
$(SRCS): $(NV2A_VSH_OBJS)
.PHONY: clean-nv2a-vsh-objs
clean-nv2a-vsh-objs:
	$(VE)rm -f $(NV2A_VSH_OBJS)

$(NV2A_VSH_OBJS): %.vshinc: %.vsh
	@echo "[ nv2avsh  ] $@"
	$(VE) $(NV2AVSH) '$<' '$@'
