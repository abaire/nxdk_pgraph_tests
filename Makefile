XBE_TITLE = nxdk_pgraph_tests
GEN_XISO = $(XBE_TITLE).iso
NXDK_DIR ?= $(CURDIR)/../nxdk
NXDK_SDL = y
NXDK_CXX = y

SRCS = \
	$(CURDIR)/depth_format_tests.cpp \
	$(CURDIR)/main.cpp \
	$(CURDIR)/math3d.c \
	$(CURDIR)/pbkit_ext.cpp \
	$(CURDIR)/test_host.cpp \
	$(CURDIR)/texture_format.cpp \
	$(CURDIR)/texture_format_tests.cpp \
	$(CURDIR)/third_party/swizzle.c

SHADER_OBJS = ps.inl vs.inl

DEVKIT ?= y
ifeq ($(DEVKIT),y)
CXXFLAGS += -DDEVKIT
endif

include $(NXDK_DIR)/Makefile
