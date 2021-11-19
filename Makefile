XBE_TITLE = nxdk_pgraph_tests
GEN_XISO = $(XBE_TITLE).iso
NXDK_DIR ?= $(CURDIR)/../nxdk
NXDK_SDL = y
NXDK_CXX = y

SRCS = \
	$(CURDIR)/debug_output.cpp \
	$(CURDIR)/main.cpp \
	$(CURDIR)/math3d.c \
	$(CURDIR)/pbkit_ext.cpp \
	$(CURDIR)/menu_item.cpp \
	$(CURDIR)/shaders/orthographic_vertex_shader.cpp \
	$(CURDIR)/shaders/perspective_vertex_shader.cpp \
	$(CURDIR)/shaders/precalculated_vertex_shader.cpp \
	$(CURDIR)/shaders/projection_vertex_shader.cpp \
	$(CURDIR)/shaders/shader_program.cpp \
	$(CURDIR)/test_driver.cpp \
	$(CURDIR)/test_host.cpp \
	$(CURDIR)/tests/depth_format_tests.cpp \
	$(CURDIR)/tests/front_face_tests.cpp \
	$(CURDIR)/tests/test_suite.cpp \
	$(CURDIR)/tests/texture_format_tests.cpp \
	$(CURDIR)/texture_format.cpp \
	$(CURDIR)/vertex_buffer.cpp \
	$(CURDIR)/third_party/swizzle.c \
	$(CURDIR)/third_party/printf/printf.c

SHADER_OBJS = \
	$(CURDIR)/shaders/precalculated_vertex_shader.inl \
	$(CURDIR)/shaders/projection_vertex_shader.inl \
	$(CURDIR)/shaders/textured_pixelshader.inl \
	$(CURDIR)/shaders/untextured_pixelshader.inl

DEBUG := y
CFLAGS += -I$(CURDIR) -I$(CURDIR)/third_party
CXXFLAGS += -I$(CURDIR) -I$(CURDIR)/third_party

DEVKIT ?= y
ifeq ($(DEVKIT),y)
CXXFLAGS += -DDEVKIT
endif

include $(NXDK_DIR)/Makefile

XBDM_GDB_BRIDGE := xbdm
REMOTE_PATH := e:\\pgraph
XBOX ?=
.phony: deploy
deploy: $(OUTPUT_DIR)/default.xbe
	$(XBDM_GDB_BRIDGE) $(XBOX) -- mkdir $(REMOTE_PATH)
	$(XBDM_GDB_BRIDGE) $(XBOX) -- putfile $< $(REMOTE_PATH) -f

.phony: execute
execute: deploy
	$(XBDM_GDB_BRIDGE) $(XBOX) -s -- /run $(REMOTE_PATH)



