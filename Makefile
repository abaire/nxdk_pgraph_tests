XBE_TITLE = nxdk_texture_format_tests
GEN_XISO = $(XBE_TITLE).iso
NXDK_DIR ?= $(CURDIR)/../nxdk
NXDK_SDL = y
NXDK_CXX = y

SRCS = \
	$(CURDIR)/main.cpp \
	$(CURDIR)/math3d.c \
	$(CURDIR)/third_party/swizzle.c

SHADER_OBJS = ps.inl vs.inl

include $(NXDK_DIR)/Makefile

