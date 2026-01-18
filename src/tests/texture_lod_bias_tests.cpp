#include "texture_lod_bias_tests.h"

#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"
#include "texture_generator.h"

static constexpr char kLODBiasTest[] = "LODBias";

TextureLodBiasTests::TextureLodBiasTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texture LOD Bias", config) {
  tests_[kLODBiasTest] = [this]() { Test(); };
}

/**
 * Constructs the test suite and creates test cases.
 *
 * @tc LODBias
 *   Renders a series of textured quads with various levels of LOD bias configured. Demonstrates that LOD bias is in
 *   fixed point 5.8 format and that levels are blended. The left side of the image demonstrates various unbiased mipmap
 *   levels through geometry sizing.
 */
void TextureLodBiasTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
}

void TextureLodBiasTests::Test() {
  host_.PrepareDraw(0xFE202020);

  static constexpr uint32_t kTextureSize = 512;
  static constexpr float kGeometrySize = 64.f;
  static constexpr float kCheckerboardBoxSize = 4;

  static constexpr float kDemoReservedTextHeader = 64.f;
  static constexpr float kDemoHorizontalScreenSpacePercentage = 0.25f;

  struct MipmapDefinition {
    const char *name;
    uint32_t color;
    uint32_t right_shift;
  };
  static constexpr MipmapDefinition kLevels[] = {
      {"Blue", 0xFF0000FF, 0}, {"Green", 0xFF00FF00, 1},   {"Red", 0xFFFF0000, 2},    {"White", 0xFFFFFFFF, 3},
      {"Cyan", 0xFF00FFFF, 4}, {"Magenta", 0xFFFF00FF, 5}, {"Yellow", 0xFFFFFF00, 6}, {"Grey", 0xFF808080, 7},
  };

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_TENT_TENT_LOD);
  texture_stage.SetMipMapLevels(sizeof(kLevels) / sizeof(kLevels[0]));
  texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
  host_.SetupTextureStages();

  auto generate_texture = [](uint8_t *dest, uint32_t color, uint32_t size) {
    const uint32_t pitch = size * 4;
    PBKitPlusPlus::GenerateSwizzledRGBACheckerboard(dest, 0, 0, size, size, pitch, color, 0xFF050505,
                                                    kCheckerboardBoxSize);
    return size * pitch;
  };

  auto texture_memory = host_.GetTextureMemoryForStage(0);
  for (auto &level : kLevels) {
    texture_memory += generate_texture(texture_memory, level.color, kTextureSize >> level.right_shift);
  }

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  auto draw = [this](float left, float top, float size) {
    float right = left + size;
    float bottom = top + size;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(left, top, 0.1f, 1.0f);

    host_.SetTexCoord0(1.0f, 0.0f);
    host_.SetVertex(right, top, 0.1f, 1.0f);

    host_.SetTexCoord0(1.0f, 1.0f);
    host_.SetVertex(right, bottom, 0.1f, 1.0f);

    host_.SetTexCoord0(0.0f, 1.0f);
    host_.SetVertex(left, bottom, 0.1f, 1.0f);
    host_.End();
  };

  float top = floorf((host_.GetFramebufferHeightF() - kGeometrySize) * 0.5f) - 8.f;
  const float demo_vertical_center =
      floorf(kDemoReservedTextHeader + (host_.GetFramebufferHeightF() - kDemoReservedTextHeader) * 0.5f);
  const float end_of_demo = floorf(host_.GetFramebufferWidthF() * kDemoHorizontalScreenSpacePercentage);
  const float start_of_results = end_of_demo + 32.f;

  for (auto &level : kLevels) {
    const auto size = static_cast<float>(kTextureSize >> level.right_shift);
    draw(end_of_demo - size, floorf(demo_vertical_center - size * 0.5f), size);
  }

  static constexpr float kSpacing = 32.f;
#define FIXED_5_8(x) (static_cast<int32_t>((x) * (1 << 8)) & 0x1FFF)
  static constexpr float kLODBias[] = {-15.f, -4.f, -2.f,  -1.f, 0.f,  1.f,  2.f, 3.f,
                                       4.f,   15.f, -0.8f, 1.2f, 1.8f, 2.5f, 3.5f};

  float left = start_of_results;
  static constexpr float kLabelColumnStart = 20;
  int32_t label_row = 6;
  int32_t label_col = kLabelColumnStart;

  for (auto bias : kLODBias) {
    auto fixed_point_bias = FIXED_5_8(bias);
    texture_stage.SetFilter(fixed_point_bias, TextureStage::K_QUINCUNX, TextureStage::MIN_TENT_TENT_LOD);
    host_.SetupTextureStages();
    draw(left, top, kGeometrySize);

    // pb_printat does not support float specifiers.
    char buf[16];
    snprintf(buf, sizeof(buf), "%0.1f", bias);
    pb_printat(label_row, label_col, "%s", buf);

    left += kGeometrySize + kSpacing;
    label_col += 9;

    if (left + kGeometrySize > host_.GetFramebufferWidthF()) {
      left = start_of_results;
      top += kGeometrySize + kSpacing;
      label_row += 4;
      label_col = kLabelColumnStart;
    }
  }

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  texture_stage.SetMipMapLevels(1);

  pb_printat(0, 0, "LOD bias\n");
  pb_printat(1, 0, "Levels: ");
  for (auto &level : kLevels) {
    pb_print("%s ", level.name);
  }

  pb_printat(3, 0, "Nested geometry\n  no bias");
  pb_printat(3, 30, "Labeled bias (fixed 5.8)");

  pb_draw_text_screen();

  FinishDraw(kLODBiasTest);
}
