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
	$(SRCDIR)/main.cpp \
	$(SRCDIR)/math3d.c \
	$(SRCDIR)/pbkit_ext.cpp \
	$(SRCDIR)/menu_item.cpp \
	$(SRCDIR)/shaders/orthographic_vertex_shader.cpp \
	$(SRCDIR)/shaders/perspective_vertex_shader.cpp \
	$(SRCDIR)/shaders/precalculated_vertex_shader.cpp \
	$(SRCDIR)/shaders/projection_vertex_shader.cpp \
	$(SRCDIR)/shaders/shader_program.cpp \
	$(SRCDIR)/test_driver.cpp \
	$(SRCDIR)/test_host.cpp \
	$(SRCDIR)/tests/depth_format_tests.cpp \
	$(SRCDIR)/tests/fog_tests.cpp \
	$(SRCDIR)/tests/front_face_tests.cpp \
	$(SRCDIR)/tests/image_blit_tests.cpp \
	$(SRCDIR)/tests/lighting_normal_tests.cpp \
	$(SRCDIR)/tests/material_alpha_tests.cpp \
	$(SRCDIR)/tests/material_color_tests.cpp \
	$(SRCDIR)/tests/material_color_source_tests.cpp \
	$(SRCDIR)/tests/set_vertex_data_tests.cpp \
	$(SRCDIR)/tests/test_suite.cpp \
	$(SRCDIR)/tests/texture_format_tests.cpp \
	$(SRCDIR)/tests/three_d_primitive_tests.cpp \
	$(SRCDIR)/tests/two_d_line_tests.cpp \
	$(SRCDIR)/texture_format.cpp \
	$(SRCDIR)/vertex_buffer.cpp \
	$(THIRDPARTYDIR)/swizzle.c \
	$(THIRDPARTYDIR)/printf/printf.c

SHADER_OBJS = \
	$(SRCDIR)/shaders/fog_infinite_fogc_test.inl \
	$(SRCDIR)/shaders/precalculated_vertex_shader.inl \
	$(SRCDIR)/shaders/projection_vertex_shader.inl \
	$(SRCDIR)/shaders/projection_vertex_shader_no_lighting.inl \
	$(SRCDIR)/shaders/textured_pixelshader.inl \
	$(SRCDIR)/shaders/untextured_pixelshader.inl

DEBUG := y
CFLAGS += -I$(SRCDIR) -I$(THIRDPARTYDIR)
CXXFLAGS += -I$(SRCDIR) -I$(THIRDPARTYDIR)

DEVKIT ?= y
ifeq ($(DEVKIT),y)
CXXFLAGS += -DDEVKIT
endif

DISABLE_AUTORUN ?= n
ifeq ($(DISABLE_AUTORUN),y)
CXXFLAGS += -DDISABLE_AUTORUN
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

RESOURCES = \
	$(patsubst $(RESOURCEDIR)/%,$(OUTPUT_DIR)/%,$(wildcard $(RESOURCEDIR)/image_blit/*))

TARGET += $(RESOURCES)
$(GEN_XISO): $(RESOURCES)

$(OUTPUT_DIR)/%: $(RESOURCEDIR)/%
	$(VE)mkdir -p '$(dir $@)'
	$(VE)cp -r '$<' '$@'

.PHONY: clean-resources
clean-resources:
	$(VE)rm -rf $(OUTPUT_DIR)/resources
