#include "texture_cpu_update_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "texture_format.h"
#include "vertex_buffer.h"

static constexpr float kTextureSize = 256.0f;

static constexpr char kRGBATest[] = "RGBA";
static constexpr char kPalettizedTest[] = "PaletteCycle";

TextureCPUUpdateTests::TextureCPUUpdateTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texture CPU Update") {
  tests_[kRGBATest] = [this]() { TestRGBA(); };
  //  tests_[kPalettizedTest] = [this]() { TestPalettized(); };
}

void TextureCPUUpdateTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
}

static void Draw(const TestHost &host_) {
  const float kLeft = 0;
  const float kRight = host_.GetFramebufferWidthF();
  const float kTop = 0;
  const float kBottom = host_.GetFramebufferHeightF();
  const float kZ = 1.0f;

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetTexCoord0(0.0f, 0.0f);
  host_.SetVertex(kLeft, kTop, kZ, 1.0f);
  host_.SetVertex(kRight, kTop, kZ, 1.0f);
  host_.SetVertex(kRight, kBottom, kZ, 1.0f);
  host_.SetVertex(kLeft, kBottom, kZ, 1.0f);
  host_.End();
}

void TextureCPUUpdateTests::TestRGBA() {
  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  auto &stage = host_.GetTextureStage(0);
  stage.SetTextureDimensions(kTextureSize, kTextureSize);
  host_.SetupTextureStages();

  pb_erase_text_screen();

  // Set the texture to pure red.
  auto texels = host_.GetTextureMemoryForStage(0);
  for (uint32_t y = 0; y < kTextureSize; ++y) {
    for (uint32_t x = 0; x < kTextureSize; ++x) {
      *texels++ = 0x66;
      *texels++ = 0x00;
      *texels++ = 0x00;
      *texels++ = 0xFF;
    }
  }

  host_.PrepareDraw(0xFE202020);
  Draw(host_);
  host_.FinishDraw(false, output_dir_, kRGBATest);

  // Set the texture to pure green.
  texels = host_.GetTextureMemoryForStage(0);
  for (uint32_t y = 0; y < kTextureSize; ++y) {
    for (uint32_t x = 0; x < kTextureSize; ++x) {
      *texels++ = 0x00;
      *texels++ = 0x66;
      *texels++ = 0x00;
      *texels++ = 0xFF;
    }
  }
  Draw(host_);

  pb_erase_text_screen();
  pb_printat(0, 0, (char *)"%s", kRGBATest);
  pb_printat(7, 12, (char *)"Expect a green screen");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, kRGBATest);
}

// This does not actually change the texture on HW
// void TextureCPUUpdateTests::TestPalettized() {
//  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8));
//
//  // Set the texture to use palette index 1 for everything.
//  auto texels = host_.GetTextureMemoryForStage(0);
//  memset(texels, 0x01, kTextureSize * kTextureSize);
//
//  auto palette = host_.GetPaletteMemoryForStage(0);
//  host_.SetPaletteSize(TestHost::PALETTE_32, 0);
//  palette[1] = 0xFF770000;
//
//  auto &stage = host_.GetTextureStage(0);
//  stage.SetTextureDimensions(kTextureSize, kTextureSize);
//  host_.SetupTextureStages();
//
//  pb_erase_text_screen();
//
//  host_.PrepareDraw(0xFE212021);
//
//  Draw(host_);
//  host_.FinishDraw(false, output_dir_, kPalettizedTest);
//
//  palette[1] = 0xFF007700;
//  Draw(host_);
//
//  pb_erase_text_screen();
//  pb_print("%s\n", kPalettizedTest);
//  pb_printat(7, 12, (char *)"Expect a green screen");
//  pb_draw_text_screen();
//
//  host_.FinishDraw(allow_saving_, output_dir_, kPalettizedTest);
//}
