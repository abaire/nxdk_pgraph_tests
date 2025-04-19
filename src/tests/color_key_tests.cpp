#include "color_key_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/perspective_vertex_shader.h"
#include "texture_generator.h"
#include "vertex_buffer.h"

static constexpr const char kFixedTextured[] = "FixedTex";
static constexpr const char kTextured[] = "ProgTex";
static constexpr const char kUnsampledTexturesStillActivate[] = "UnsampledTex";

static constexpr uint32_t kColorKeyModes[] = {
    TextureStage::CKM_DISABLE,
    TextureStage::CKM_KILL_ALPHA,
    TextureStage::CKM_KILL_COLOR,
    TextureStage::CKM_KILL_TEXEL,
};

constexpr uint32_t kTextureSize = 64;

static constexpr uint32_t kCheckerboardA = 0xFDCCCCCC;
static constexpr uint32_t kCheckerboardB = 0xFE999999;

static constexpr uint32_t kColorKeys[] = {
    0xF00000FF,  // Blue
    0xF1FF00FF,  // Magenta
    0xF2800080,  // Deep purple
    0xF3FF0000,  // Red
};
static constexpr uint32_t kAlphaKeys[] = {
    0xF4707071,
    0xF5707072,
    0xF6707073,
    0xF7707074,
};

static constexpr uint32_t kBackgroundColor = 0xFF00CC00;

static std::string MakeTestName(const char* prefix, uint32_t mode, bool alpha_from_texture);

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc FixedTex_Alpha
 *    With the fixed function pipeline, zero out just the alpha channel for all places where the colorkey matches the
 *    value of a texel. RGB channels are unaffected. This should omit all texels that match the color key.
 *
 * @tc FixedTex_AlphaColor
 *    With the fixed function pipeline, kill the texel completely for wherever the color key matches the value of a
 *    texel. This completely masks the texel rather than simply zero-ing out values. See IgnAlphaCh_FixedTex_AlphaColor.
 *    This should omit all texels that match the color key.
 *
 * @tc FixedTex_Color
 *    With the fixed function pipeline, zero out the color and alpha channels for all places where the color key matches
 *    the value of a texel. Note that the Alpha channel is also set to zero. This should omit all texels that match the
 *    color key.
 *
 * @tc FixedTex_Disabled
 *    With the fixed function pipeline, make no changes to any texel whose value matches the color key. This should
 *    render all texels as fully opaque with their original colors.
 *
 * @tc IgnAlphaCh_FixedTex_Alpha
 *    With the fixed function pipeline, set the color key mode to clear just the alpha channel for matching texels.
 *    When rendering, set the final combiner to use an alpha value of 0xFF for all texels, forcing pixels to be
 *    rendered opaque despite matching the color key. This should render all texels fully opaque with their original
 *    colors, since the texel alpha is ignored during texturing.
 *
 * @tc IgnAlphaCh_FixedTex_AlphaColor
 *    With the fixed function pipeline, kill the texel completely for wherever the color key matches the value of a
 *    texel. This completely masks the texel rather than simply zero-ing out values. When rendering, set the final
 *    combiner to use an alpha value of 0xFF for all texels. This omits killed texels even though the combiner sets them
 *    to fully opaque during texturing.
 *
 * @tc IgnAlphaCh_FixedTex_Color
 *    With the fixed function pipeline, zero out the color and alpha channels for all places where the color key matches
 *    the value of a texel. When rendering, set the final combiner to use an alpha value of 0xFF for all texels, forcing
 *    pixels to be rendered opaque despite matching the color key. This should render matching texels as fully black,
 *    since their RGB values were zeroed out and the zeroed alpha channel is ignored.
 *
 * @tc IgnAlphaCh_FixedTex_Disabled
 *    With the fixed function pipeline, make no changes to any texel whose value matches the color key. When rendering,
 *    set the final combiner to use an alpha value of 0xFF for all texels. This should render all texels as fully opaque
 *    with their original colors.
 *
 * @tc IgnAlphaCh_ProgTex_Alpha
 *    With a programmable shader, set the color key mode to clear just the alpha channel for matching texels. When
 *    rendering, set the final combiner to use an alpha value of 0xFF for all texels, forcing pixels to be rendered
 *    opaque despite matching the color key. This should render all texels fully opaque with their original colors,
 *    since the texel alpha is ignored during texturing.
 *
 * @tc IgnAlphaCh_ProgTex_AlphaColor
 *    With a programmable shader, kill the texel completely for wherever the color key matches the value of a
 *    texel. This completely masks the texel rather than simply zero-ing out values. When rendering, set the final
 *    combiner to use an alpha value of 0xFF for all texels. This omits killed texels even though the combiner sets them
 *    to fully opaque during texturing.
 *
 * @tc IgnAlphaCh_ProgTex_Color
 *    With a programmable shader, zero out the color and alpha channels for all places where the color key matches the
 *    value of a texel. When rendering, set the final combiner to use an alpha value of 0xFF for all texels, forcing
 *    pixels to be rendered opaque despite matching the color key. This should render matching texels as fully black,
 *    since their RGB values were zeroed out and the zeroed alpha channel is ignored.
 *
 * @tc IgnAlphaCh_ProgTex_Disabled
 *    With a programmable shader, make no changes to any texel whose value matches the color key. When rendering, set
 *    the final combiner to use an alpha value of 0xFF for all texels. This should render all texels as fully opaque
 *    with their original colors.
 *
 * @tc ProgTex_Alpha
 *    With a programmable shader, zero out just the alpha channel for all places where the colorkey matches the value of
 *    a texel. RGB channels are unaffected. This should omit all texels that match the color key.
 *
 * @tc ProgTex_AlphaColor
 *    With a programmable shader, kill the texel completely for wherever the color key matches the value of a texel.
 *    This completely masks the texel rather than simply zero-ing out values. See IgnAlphaCh_FixedTex_AlphaColor.
 *    This should omit all texels that match the color key.
 *
 * @tc ProgTex_Color
 *    With a programmable shader, zero out the color and alpha channels for all places where the color key matches the
 *    value of a texel. Note that the Alpha channel is also set to zero. This should omit all texels that match the
 *    color key.
 *
 * @tc ProgTex_Disabled
 *    With a programmable shader, make no changes to any texel whose value matches the color key. This should render all
 *    texels as fully opaque with their original colors.
 *
 * @tc UnsampledTex
 *    Demonstrates that the color keys are independent of whether the associated texture stage is used in the final
 *    composition or not.
 *    With a programable shader, draw two quads with the color key mode set to kill matching texels. In both cases, the
 *    final color combiner only samples from texture 0.
 *    The left quad renders with the blue color key as key 0.
 *    The right quad enables the second texture stage, keeps key 0, and adds the magenta color key with an alpha value
 *    forced to 0xFF as key 1.
 *    The left image should have the blue color in the NW checkerboard killed.
 *    The right image should have the blue color in the NW checkerboard killed, but also the secondary blue color in the
 *    SW checkerboard killed. This is because the unsampled magenta texture's SW secondary color matches the
 *    magenta + 0xFF alpha enabled as a color key in the second operation.
 */
ColorKeyTests::ColorKeyTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Color key", config) {
  for (auto alpha : {false, true}) {
    for (auto mode : kColorKeyModes) {
      {
        std::string name = MakeTestName(kFixedTextured, mode, alpha);
        tests_[name] = [this, name, mode, alpha]() { this->TestFixedFunction(name, mode, alpha); };
      }
      {
        std::string name = MakeTestName(kTextured, mode, alpha);
        tests_[name] = [this, name, mode, alpha]() { this->Test(name, mode, alpha); };
      }
    }
  }

  tests_[kUnsampledTexturesStillActivate] = [this]() { this->TestUnsampled(kUnsampledTexturesStillActivate); };
}

void ColorKeyTests::Initialize() {
  TestSuite::Initialize();

  constexpr uint32_t kCheckerSize = 8;

  static uint32_t half_texture_size = kTextureSize >> 1;

  auto generate = [this](uint32_t stage) {
    auto texture = host_.GetTextureMemoryForStage(stage);
    uint32_t other_colors[3];
    uint32_t other_alphas[3];
    {
      uint32_t* other_c = other_colors;
      uint32_t* other_a = other_alphas;
      for (auto i = 0; i < 4; ++i) {
        if (i == stage) {
          continue;
        }
        *other_c++ = kColorKeys[i];
        *other_a++ = kAlphaKeys[i];
      }
    }

    // This color, but with modified alpha such that it matches no key.
    uint32_t color_with_modified_alpha = kColorKeys[stage] | 0xFF000000;
    // This color but with alpha from the alpha key
    uint32_t color_with_alpha_alpha = (kColorKeys[stage] & 0x00FFFFFF) | (kAlphaKeys[stage] & 0xFF000000);

    // Color keyed
    GenerateRGBACheckerboard(texture, 0, 0, half_texture_size, half_texture_size, kTextureSize * 4, kCheckerboardA,
                             kColorKeys[stage], kCheckerSize);
    // Alpha keyed
    GenerateRGBACheckerboard(texture, half_texture_size, 0, half_texture_size, half_texture_size, kTextureSize * 4,
                             kCheckerboardB, kAlphaKeys[stage], kCheckerSize >> 1);

    GenerateRGBACheckerboard(texture, 0, half_texture_size, half_texture_size, half_texture_size, kTextureSize * 4,
                             color_with_alpha_alpha, color_with_modified_alpha, kCheckerSize);

    uint32_t other_color_with_color_alpha = (other_colors[0] & 0x00FFFFFF) | (kColorKeys[stage] & 0xFF000000);
    uint32_t other_color_with_alpha_alpha = (other_colors[0] & 0x00FFFFFF) | (kAlphaKeys[stage] & 0xFF000000);
    // Half other color but with alpha set to match the color key and half other color with alpha set to match the alpha
    // key.
    GenerateRGBACheckerboard(texture, half_texture_size, half_texture_size, half_texture_size, half_texture_size,
                             kTextureSize * 4, other_color_with_color_alpha, other_color_with_alpha_alpha,
                             kCheckerSize);
  };

  generate(0);
  generate(1);
  generate(2);
  generate(3);
}

void ColorKeyTests::TearDownTest() {
  TestSuite::TearDownTest();
  Pushbuffer::Begin();
  // Zero out the color keys.
  Pushbuffer::Push(NV097_SET_COLOR_KEY_COLOR, 0, 0, 0, 0);
  Pushbuffer::End();
}

static void AddVertex(TestHost& host, float x, float y, float u, float v) {
  host.SetDiffuse(0.4f, 0.1f, 0.8f);
  host.SetTexCoord0(u, v);
  host.SetTexCoord1(u, v);
  host.SetTexCoord2(u, v);
  host.SetTexCoord3(u, v);
  host.SetVertex(x, y, 0.1f, 1.f);
};

static void DrawQuads(TestHost& host, float x = 0.f, float y = 0.f) {
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_BREAK_VERTEX_BUFFER_CACHE, 0);
  Pushbuffer::End();

  host.Begin(TestHost::PRIMITIVE_QUADS);

  auto size = 0.75f;

  AddVertex(host, -size + x, size + y, 0.f * kTextureSize, 0.f * kTextureSize);
  AddVertex(host, size + x, size + y, 1.f * kTextureSize, 0.f * kTextureSize);
  AddVertex(host, size + x, -size + y, 1.f * kTextureSize, 1.f * kTextureSize);
  AddVertex(host, -size + x, -size + y, 0.f * kTextureSize, 1.f * kTextureSize);

  host.End();
}

/**
 * Draws a set of four pairs of quads, one for each texture stage (NW = stage 0, NE = stage 1, SW = stage 2, SE = stage
 * 3). For each pair, set the color key for the stage to the kColorKeys entry for that stage for the left quad, and set
 * it to the kAlphaKeys entry for the right quad.
 *
 * If `alpha_from_texture` is false, ignores the alpha provided by the texture unit after the color key operation and
 * forces it to 0xFF. This means that all pixels will be rendered, even if the color key matches and the color key mode
 * indicates that the alpha of the texel should be set to 0.
 */
static void DrawScreen(TestHost& host, bool alpha_from_texture) {
  // Color key killing will take effect even if the final combiner does not pull from the texture channel with a
  // matching key. In this test case, each quad has valid UV coords for every texture stage, so if they are enabled,
  // pixels that happen to coincide with pixels in textures that aren't being sampled from by the combiner will be
  // killed. Thus, it is important to only enable the stage(s) that are relevant for the rendering.
  host.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  host.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  if (alpha_from_texture) {
    host.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
  } else {
    host.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  }
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_COLOR_KEY_COLOR + 0x0, kColorKeys[0]);
    Pushbuffer::End();
    DrawQuads(host, -2.6f, 1.25f);
  }
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_COLOR_KEY_COLOR + 0x0, kAlphaKeys[0]);
    Pushbuffer::End();
    DrawQuads(host, -1.f, 1.25f);
  }

  host.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_2D_PROJECTIVE);
  host.SetFinalCombiner0Just(TestHost::SRC_TEX1);
  if (alpha_from_texture) {
    host.SetFinalCombiner1Just(TestHost::SRC_TEX1, true);
  }
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_COLOR_KEY_COLOR + 0x4, kColorKeys[1]);
    Pushbuffer::End();
    DrawQuads(host, 1.f, 1.25f);
  }
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_COLOR_KEY_COLOR + 0x4, kAlphaKeys[1]);
    Pushbuffer::End();
    DrawQuads(host, 2.6f, 1.25f);
  }

  host.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_2D_PROJECTIVE);
  host.SetFinalCombiner0Just(TestHost::SRC_TEX2);
  if (alpha_from_texture) {
    host.SetFinalCombiner1Just(TestHost::SRC_TEX2, true);
  }
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_COLOR_KEY_COLOR + 0x8, kColorKeys[2]);
    Pushbuffer::End();
    DrawQuads(host, -2.6f, -1.25f);
  }
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_COLOR_KEY_COLOR + 0x8, kAlphaKeys[2]);
    Pushbuffer::End();
    DrawQuads(host, -1.f, -1.25f);
  }

  host.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE,
                             TestHost::STAGE_2D_PROJECTIVE);
  host.SetFinalCombiner0Just(TestHost::SRC_TEX3);
  if (alpha_from_texture) {
    host.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);
  }
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_COLOR_KEY_COLOR + 0xC, kColorKeys[3]);
    Pushbuffer::End();
    DrawQuads(host, 1.f, -1.25f);
  }
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_COLOR_KEY_COLOR + 0xC, kAlphaKeys[3]);
    Pushbuffer::End();
    DrawQuads(host, 2.6f, -1.25f);
  }
}

void ColorKeyTests::TestFixedFunction(const std::string& name, uint32_t mode, bool alpha_from_texture) {
  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  host_.PrepareDraw(kBackgroundColor);

  SetupAllTextureStages(mode);

  DrawScreen(host_, alpha_from_texture);

  DisableAllTextureStages();

  pb_print("%s\n", name.c_str());

  pb_printat(8, 0, (char*)"Quad:      ARGB, argb       Layout: ARGB0|argb0  ARGB1|argb1");
  pb_printat(9, 0, (char*)"      aRGB|xRGB, Axxx|axxx          ARGB2|argb2  ARGB3|argb3");

  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

static void SetShader(TestHost& host_) {
  float depth_buffer_max_value = host_.GetMaxDepthBufferValue();
  auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                                          0.0f, depth_buffer_max_value, M_PI * 0.25f, 1.0f, 200.0f);
  {
    shader->SetLightingEnabled(false);
    shader->SetUseD3DStyleViewport();
    vector_t camera_position = {0.0f, 0.0f, -7.0f, 1.0f};
    vector_t camera_look_at = {0.0f, 0.0f, 0.0f, 1.0f};
    shader->LookAt(camera_position, camera_look_at);
  }
  host_.SetVertexShaderProgram(shader);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void ColorKeyTests::Test(const std::string& name, uint32_t mode, bool alpha_from_texture) {
  SetShader(host_);

  host_.PrepareDraw(kBackgroundColor);

  SetupAllTextureStages(mode);

  DrawScreen(host_, alpha_from_texture);

  DisableAllTextureStages();

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void ColorKeyTests::TestUnsampled(const std::string& name) {
  SetShader(host_);

  host_.PrepareDraw(kBackgroundColor);

  SetupAllTextureStages(TextureStage::CKM_KILL_TEXEL);

  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_COLOR_KEY_COLOR + 0x0, kColorKeys[0]);
    Pushbuffer::End();
    DrawQuads(host_, -1.6f, 0.f);
  }

  // Enable the second stage and kill texels from the bottom row. Note that the key color does not match anything in
  // TEX0.
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE, TestHost::STAGE_2D_PROJECTIVE);
  uint32_t tex1_color_key = (kColorKeys[1] & 0x00FFFFFF) | 0xFF000000;
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_COLOR_KEY_COLOR + 0x4, tex1_color_key);
    Pushbuffer::End();
    DrawQuads(host_, 1.6f, 0.f);
  }

  DisableAllTextureStages();

  pb_print("%s\n", name.c_str());
  pb_printat(4, 9, (char*)"CK0: 0x%08X", kColorKeys[0]);
  pb_printat(5, 39, (char*)"Tex1 ON");
  pb_printat(12, 35, (char*)"CK0: 0x%08X", kColorKeys[0]);
  pb_printat(13, 35, (char*)"CK1: 0x%08X", tex1_color_key);
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void ColorKeyTests::SetupTextureStage(uint32_t stage, uint32_t mode) const {
  host_.SetTextureStageEnabled(stage, true);
  auto& texture_stage = host_.GetTextureStage(stage);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
  texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
  texture_stage.SetColorKeyMode(mode);
}

void ColorKeyTests::SetupAllTextureStages(uint32_t mode) const {
  SetupTextureStage(0, mode);
  SetupTextureStage(1, mode);
  SetupTextureStage(2, mode);
  SetupTextureStage(3, mode);

  host_.SetupTextureStages();
}

void ColorKeyTests::DisableAllTextureStages() const {
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, false);
  host_.SetTextureStageEnabled(3, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
}

static std::string MakeTestName(const char* prefix, uint32_t mode, bool alpha_from_texture) {
  std::string ret = alpha_from_texture ? "" : "IgnAlphaCh_";
  ret += prefix;
  switch (mode) {
    case TextureStage::CKM_DISABLE:
      ret += "_Disabled";
      break;
    case TextureStage::CKM_KILL_ALPHA:
      ret += "_Alpha";
      break;
    case TextureStage::CKM_KILL_COLOR:
      ret += "_Color";
      break;
    case TextureStage::CKM_KILL_COLOR | TextureStage::CKM_KILL_ALPHA:
      ret += "_AlphaColor";
      break;
    default:
      ASSERT(!"Unhandled test mode");
  }
  return ret;
}
