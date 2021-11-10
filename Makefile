XBE_TITLE = nxdk_texture_format_tests
GEN_XISO = $(XBE_TITLE).iso
SRCS = $(CURDIR)/math3d.c $(CURDIR)/main.c $(CURDIR)/swizzle.c
SHADER_OBJS = ps.inl vs.inl
NXDK_DIR ?= $(CURDIR)/../nxdk
NXDK_SDL = y
NXDK_CXX = y

include $(NXDK_DIR)/Makefile

