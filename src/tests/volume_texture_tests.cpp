#include "volume_texture_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "shaders/perspective_vertex_shader.h"
#include "test_host.h"
#include "texture_format.h"
#include "vertex_buffer.h"

static int GeneratePalettizedSurface(uint8_t **ret, uint32_t width, uint32_t height, uint32_t depth,
                                     TestHost::PaletteSize palette_size);
static uint32_t *GeneratePalette(TestHost::PaletteSize size);

static const VolumeTextureTests::TestConfig kTestConfigs[] = {
    {"Indexed", GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8B8G8R8), false},
};

static constexpr const char *kTestName = "Palettized";

VolumeTextureTests::VolumeTextureTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Volume texture") {
  for (auto &config : kTestConfigs) {
    tests_[kTestName] = [this, &config]() { Test(config); };
  }
}

void VolumeTextureTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  host_.SetShaderProgram(nullptr);
  CreateGeometry();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetTextureStageEnabled(0, true);

  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  host_.SetInputColorCombiner(0, TestHost::SRC_TEX0, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false,
                              TestHost::MAP_UNSIGNED_INVERT);
  host_.SetInputAlphaCombiner(0, TestHost::SRC_TEX0, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false,
                              TestHost::MAP_UNSIGNED_INVERT);

  host_.SetOutputColorCombiner(0, TestHost::DST_DIFFUSE);
  host_.SetOutputAlphaCombiner(0, TestHost::DST_DIFFUSE);

  host_.SetFinalCombiner0(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false,
                          false, TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_DIFFUSE,
                          true);
}

void VolumeTextureTests::CreateGeometry() {
  const float left = -2.75f;
  const float right = 2.75f;
  const float top = 1.75f;
  const float bottom = -1.75f;
  const float mid_width = 0;
  const float mid_height = 0;

  //  const uint32_t num_quads = 4;
  //  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6 * num_quads);
  //  buffer->SetTexCoord0Count(2);
  //
  //  const float spacing = 0.05f;
  //  int index = 0;
  //
  //  buffer->DefineBiTri(index++, left, top, mid_width - spacing, mid_height + spacing);
  //
  //  buffer->DefineBiTri(index++, mid_width + spacing, top, right, mid_height + spacing);
  //
  //  buffer->DefineBiTri(index++, left, mid_height - spacing, mid_width - spacing, bottom);
  //
  //  buffer->DefineBiTri(index++, mid_width + spacing, mid_height - spacing, right, bottom);
  //
  //  // Set texcoords.
  //  auto vertex = buffer->Lock();
  //
  //  auto set_bitri_texcoords = [&vertex](float p) {
  //    vertex++->SetTexCoord0(0.0f, 0.0f, p, 0.0);
  //    vertex++->SetTexCoord0(1.0f, 0.0f, p, 0.0);
  //    vertex++->SetTexCoord0(1.0f, 1.0f, p, 0.0);
  //
  //    vertex++->SetTexCoord0(0.0f, 0.0f, p, 0.0);
  //    vertex++->SetTexCoord0(1.0f, 1.0f, p, 0.0);
  //    vertex++->SetTexCoord0(0.0f, 1.0f, p, 0.0);
  //  };
  //
  //  set_bitri_texcoords(0.0f);
  //
  //  set_bitri_texcoords(0.33f);
  //
  //  set_bitri_texcoords(0.66f);
  //
  //  set_bitri_texcoords(1.0f);
  //
  //  buffer->Unlock();

  const uint32_t num_quads = 1;
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6 * num_quads);

  int index = 0;
  buffer->DefineBiTri(index++, left, top, right, bottom);
  buffer->Linearize(static_cast<float>(host_.GetMaxTextureWidth()), static_cast<float>(host_.GetMaxTextureHeight()));
}

void VolumeTextureTests::Test(const TestConfig &config) {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  TestHost::PaletteSize palette_size = TestHost::PALETTE_256;
  auto &texture_format = GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8);
  host_.SetTextureFormat(texture_format);

  uint8_t *surface = nullptr;
  uint32_t depth = 1;
  int err = GeneratePalettizedSurface(&surface, (int)host_.GetMaxTextureWidth(), (int)host_.GetMaxTextureHeight(),
                                      depth, palette_size);
  ASSERT(!err && "Failed to generate palettized surface");

  err = host_.SetRawTexture(surface, host_.GetMaxTextureWidth(), host_.GetMaxTextureHeight(), depth,
                            host_.GetMaxTextureWidth(), 1, texture_format.xbox_swizzled);
  delete[] surface;
  ASSERT(!err && "Failed to set texture");

  auto palette = GeneratePalette(palette_size);
  err = host_.SetPalette(palette, palette_size);
  delete[] palette;
  ASSERT(!err && "Failed to set palette");

  host_.DrawArrays();

  //  /* PrepareDraw some text on the screen */
  pb_print("N: %s\n", config.format.name);
  pb_print("F: 0x%x\n", config.format.xbox_format);
  pb_print("SZ: %d\n", config.format.xbox_swizzled);
  pb_print("C: %d\n", config.format.require_conversion);
  //  pb_print("W: %d\n", host_.GetMaxTextureWidth());
  //  pb_print("H: %d\n", host_.GetMaxTextureHeight());
  //  pb_print("P: %d\n", texture_format.xbox_bpp * host_.GetMaxTextureWidth());
  //  pb_print("ERR: %d\n", update_texture_result);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, config.name);
}

static int GeneratePalettizedSurface(uint8_t **ret, uint32_t width, uint32_t height, uint32_t depth,
                                     TestHost::PaletteSize palette_size) {
  *ret = new uint8_t[width * height * depth];
  if (!(*ret)) {
    return 1;
  }

  auto pixel = *ret;

  uint32_t layer_size = width * height;

  uint32_t half_size = layer_size >> 1;

  for (uint32_t i = 0; i < half_size; ++i, ++pixel) {
    *pixel = i & (palette_size - 1);
  }

  for (uint32_t i = half_size; i < layer_size; i += 4) {
    uint8_t value = i & (palette_size - 1);
    *pixel++ = value;
    *pixel++ = value;
    *pixel++ = value;
    *pixel++ = value;
  }

  // TODO: Layer 1+.

  return 0;
}
static uint32_t *GeneratePalette(TestHost::PaletteSize size) {
  auto ret = new uint32_t[size];

  uint32_t block_size = size / 4;
  auto component_inc = (uint32_t)ceilf(255.0f / (float)block_size);
  uint32_t i = 0;
  uint32_t component = 0;
  for (; i < block_size; ++i, component += component_inc) {
    uint32_t color_value = 0xFF - component;
    ret[i + block_size * 0] = 0xFF000000 + color_value;
    ret[i + block_size * 1] = 0xFF000000 + (color_value << 8);
    ret[i + block_size * 2] = 0xFF000000 + (color_value << 16);
    ret[i + block_size * 3] = 0xFF000000 + color_value + (color_value << 8) + (color_value << 16);
  }

  return ret;
}