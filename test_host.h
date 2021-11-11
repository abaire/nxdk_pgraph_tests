#ifndef NXDK_PGRAPH_TESTS_TEST_HOST_H
#define NXDK_PGRAPH_TESTS_TEST_HOST_H

#include <pbkit/pbkit.h>

#include <cstdint>

#include "math3d.h"
#include "texture_format.h"

#pragma pack(1)
typedef struct Vertex {
  float pos[3];
  float texcoord[2];
  float normal[3];
} Vertex;
#pragma pack()

class TestHost {
 public:
  TestHost(uint32_t framebuffer_width, uint32_t framebuffer_height, uint32_t texture_width, uint32_t texture_height);

  void SetTextureFormat(const TextureFormatInfo &fmt);
  void SetDepthBufferFormat(uint32_t fmt);
  int SetTexture(SDL_Surface *gradient_surface);

  uint32_t GetTextureWidth() const { return texture_width; }
  uint32_t GetTextureHeight() const { return texture_height; }

  void Clear();
  void StartDraw();
  void FinishDrawAndSave(const char *output_directory, const char *name);

 private:
  void init_vertices();
  void init_matrices();
  static void init_shader();

  void SetupTextureStages();
  void SendShaderConstants();
  static void SaveBackbuffer(const char *output_directory, const char *name);

 private:
  uint32_t framebuffer_width;
  uint32_t framebuffer_height;

  TextureFormatInfo texture_format;
  uint32_t texture_width;
  uint32_t texture_height;

  uint32_t depth_buffer_format = NV097_SET_SURFACE_FORMAT_ZETA_Z24S8;

  Vertex *alloc_vertices_ = nullptr;           // texcoords 0 to kFramebufferWidth/kFramebufferHeight
  Vertex *alloc_vertices_swizzled_ = nullptr;  // texcoords normalized 0 to 1
  uint8_t *texture_memory_ = nullptr;

  MATRIX m_model{}, m_view{}, m_proj{};

  VECTOR v_cam_pos = {0, 0.05, 1.07, 1};
  VECTOR v_cam_rot = {0, 0, 0, 1};
  VECTOR v_light_dir = {0, 0, 1, 1};
};

#endif  // NXDK_PGRAPH_TESTS_TEST_HOST_H
