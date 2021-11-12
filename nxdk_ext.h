#ifndef NXDK_ZBUFFER_TESTS_NXDK_MISSING_DEFINES_H
#define NXDK_ZBUFFER_TESTS_NXDK_MISSING_DEFINES_H

// TODO: upstream missing nv2a defines
// See https://github.com/xqemu/xqemu/blob/f9b9a9bad88b3b2df53a1b01db6a37783aa2eb27/hw/xbox/nv2a/nv2a_shaders.c#L296-L312
#define NV2A_VERTEX_ATTR_POSITION 0
#define NV2A_VERTEX_ATTR_WEIGHT 1
#define NV2A_VERTEX_ATTR_NORMAL 2
#define NV2A_VERTEX_ATTR_DIFFUSE 3
#define NV2A_VERTEX_ATTR_SPECULAR 4
#define NV2A_VERTEX_ATTR_FOG_COORD 5
#define NV2A_VERTEX_ATTR_POINT_SIZE 6
#define NV2A_VERTEX_ATTR_BACK_DIFFUSE 7
#define NV2A_VERTEX_ATTR_BACK_SPECULAR 8
#define NV2A_VERTEX_ATTR_TEXTURE0 9
#define NV2A_VERTEX_ATTR_TEXTURE1 10
#define NV2A_VERTEX_ATTR_TEXTURE2 11
#define NV2A_VERTEX_ATTR_TEXTURE3 12

#define NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_G8B8 0x17
#define NV097_SET_TEXTURE_FORMAT_COLOR_SZ_B8G8R8A8 0x3B
#define NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_CR8YB8CB8YA8 0x24
#define NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_YB8CR8YA8CB8 0x25
#define NV097_SET_TEXTURE_FORMAT_COLOR_D16 0x2C      // TODO: proper nvidia name
#define NV097_SET_TEXTURE_FORMAT_COLOR_LIN_F16 0x31  // TODO: proper nvidia name
#define NV097_SET_CONTROL0_COLOR_SPACE_CONVERT 0xF0000000
#define NV097_SET_CONTROL0_COLOR_SPACE_CONVERT_CRYCB_TO_RGB 0x1

#define NV097_SET_CONTROL0_Z_FORMAT_FIXED 0
#define NV097_SET_CONTROL0_Z_FORMAT_FLOAT 1

#define NV097_SET_DEPTH_FUNC_V_NEVER 0x00000200
#define NV097_SET_DEPTH_FUNC_V_LESS 0x00000201
#define NV097_SET_DEPTH_FUNC_V_EQUAL 0x00000202
#define NV097_SET_DEPTH_FUNC_V_LEQUAL 0x00000203
#define NV097_SET_DEPTH_FUNC_V_GREATER 0x00000204
#define NV097_SET_DEPTH_FUNC_V_NOTEQUAL 0x00000205
#define NV097_SET_DEPTH_FUNC_V_GEQUAL 0x00000206
#define NV097_SET_DEPTH_FUNC_V_ALWAYS 0x00000207

#endif  // NXDK_ZBUFFER_TESTS_NXDK_MISSING_DEFINES_H