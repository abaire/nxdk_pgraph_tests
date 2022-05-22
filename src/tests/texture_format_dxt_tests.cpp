#include "texture_format_dxt_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "dds_image.h"
#include "debug_output.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"

static constexpr const char kAlphaDXT1[] = "D:\\dxt_images\\plasma_alpha_dxt1.dds";
static constexpr const char kAlphaDXT3[] = "D:\\dxt_images\\plasma_alpha_dxt3.dds";
static constexpr const char kAlphaDXT5[] = "D:\\dxt_images\\plasma_alpha_dxt5.dds";
static constexpr const char kOpaqueDXT1[] = "D:\\dxt_images\\plasma_dxt1.dds";
static constexpr const char kOpaqueDXT3[] = "D:\\dxt_images\\plasma_dxt3.dds";
static constexpr const char kOpaqueDXT5[] = "D:\\dxt_images\\plasma_dxt5.dds";

struct TestCase {
  const char *filename;
  TextureFormatDXTTests::CompressedTextureFormat format;
};

static constexpr TestCase kTestCases[] = {
    {kAlphaDXT1, TextureFormatDXTTests::CompressedTextureFormat::DXT1},
    {kAlphaDXT3, TextureFormatDXTTests::CompressedTextureFormat::DXT3},
    {kAlphaDXT5, TextureFormatDXTTests::CompressedTextureFormat::DXT5},
    {kOpaqueDXT1, TextureFormatDXTTests::CompressedTextureFormat::DXT1},
    {kOpaqueDXT3, TextureFormatDXTTests::CompressedTextureFormat::DXT3},
    {kOpaqueDXT5, TextureFormatDXTTests::CompressedTextureFormat::DXT5},
};

static std::string GetFormatName(TextureFormatDXTTests::CompressedTextureFormat texture_format);

TextureFormatDXTTests::TextureFormatDXTTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texture DXT") {
  for (auto &test : kTestCases) {
    tests_[MakeTestName(test.filename, test.format)] = [this, &test]() { Test(test.filename, test.format); };
  }
}

void TextureFormatDXTTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);
  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
}

void TextureFormatDXTTests::Test(const char *filename, CompressedTextureFormat texture_format) {
  host_.PrepareDraw(0xFE101010);

  DDSImage img;
  bool loaded = img.LoadFile(filename);
  ASSERT(loaded && "Failed to load test image from file.");

  auto data = img.GetPrimaryImage();
  uint32_t nv2a_format = 0;
  switch (data->format) {
    case DDSImage::SubImage::Format::DXT1:
      nv2a_format = NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT1_A1R5G5B5;
      break;

    case DDSImage::SubImage::Format::DXT3:
      nv2a_format = NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT23_A8R8G8B8;
      break;

    case DDSImage::SubImage::Format::DXT5:
      nv2a_format = NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT45_A8R8G8B8;
      break;

    default:
      ASSERT(!"Unimplemented subimage format.");
  }

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetFormat(GetTextureFormatInfo(nv2a_format));
  texture_stage.SetTextureDimensions(data->width, data->height);
  host_.SetRawTexture(data->data.data(), data->compressed_width, data->compressed_height, data->depth, data->pitch,
                      data->bytes_per_pixel, false);
  host_.SetupTextureStages();

  {
    static constexpr float kOutputSize = 256.0f;
    float left = floorf((host_.GetFramebufferWidthF() - kOutputSize) * 0.5f);
    float right = left + kOutputSize;
    float top = floorf((host_.GetFramebufferHeightF() - kOutputSize) * 0.5f);
    float bottom = top + kOutputSize;

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
  }
  std::string test_name = MakeTestName(filename, texture_format);
  pb_print("%s\n", test_name.c_str());
  pb_print("FMT: %s\n", GetFormatName(texture_format).c_str());
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}

static std::string GetFormatName(TextureFormatDXTTests::CompressedTextureFormat texture_format) {
  switch (texture_format) {
    case TextureFormatDXTTests::CompressedTextureFormat::DXT1:
      return "DXT1";

    case TextureFormatDXTTests::CompressedTextureFormat::DXT3:
      return "DXT3";

    case TextureFormatDXTTests::CompressedTextureFormat::DXT5:
      return "DXT5";
  }
}

std::string TextureFormatDXTTests::MakeTestName(const std::string &filename, CompressedTextureFormat texture_format) {
  std::string test_name = GetFormatName(texture_format);

  const auto begin = filename.rfind('\\');
  const auto end = filename.rfind('.');
  ASSERT(begin != filename.npos && end != filename.npos && "Invalid filename");
  test_name += "_" + filename.substr(begin + 1, end - begin - 1);

  return std::move(test_name);
}
