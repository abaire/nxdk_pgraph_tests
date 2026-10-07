#include "point_sprite_tests.h"

#include "shaders/perspective_vertex_shader_no_lighting.h"
#include "test_host.h"
#include "texture_generator.h"
#include "xbox-swizzle/swizzle.h"
#include "xbox_math_vector.h"

static constexpr char kAlphaTestTest[] = "AlphaTest";
static constexpr char kAlphaMaskTest[] = "AlphaMask";

PointSpriteTests::PointSpriteTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Point sprite", config) {
  tests_[kAlphaTestTest] = [this]() { TestAlphaTest(); };
  for (auto use_shader : {false, true}) {
    std::string name = kAlphaMaskTest;
    name += use_shader ? "_VS" : "_FF";
    tests_[name] = [this, use_shader]() { TestAlphaMasking(use_shader); };
  }
}

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc AlphaTest
 *   Basic test of POINT_SMOOTH_ENABLE true behavior. Renders a textured point, then the same point with the color
 *   combiner set to just diffuse, then the same pair again with point scaling enabled. The second row is the same
 *   rendering with alpha testing enabled.
 *
 * @tc AlphaMask_FF
 *   Tests point sprite alpha masking behavior using the fixed function pipeline matching hardware usage where diffuse
 *   RGB (black) is passed through, diffuse alpha is modulated with texture alpha on stage 3, and alpha testing
 *   (GREATER than 1) is applied with SRC_ALPHA / ONE_MINUS_SRC_ALPHA blending and point smoothing/scaling.
 *
 * @tc AlphaMask_VS
 *   Tests point sprite alpha masking behavior using a perspective vertex shader (programmable pipeline) matching
 *   hardware usage where diffuse RGB (black) is passed through, diffuse alpha is modulated with texture alpha on
 *   stage 3, and alpha testing (GREATER than 1) is applied with SRC_ALPHA / ONE_MINUS_SRC_ALPHA blending and point
 *   smoothing/scaling.
 */
void PointSpriteTests::Initialize() {
  TestSuite::Initialize();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void PointSpriteTests::TestAlphaTest() {
  static constexpr uint32_t kTextureSize = 64;
  GenerateSwizzledRGBRadialATestPattern(host_.GetTextureMemoryForStage(3), kTextureSize, kTextureSize);

  host_.PrepareDraw(0xFF333333);

  auto &texture_stage = host_.GetTextureStage(3);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));
  texture_stage.SetEnabled(true);
  texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
  host_.SetupTextureStages();

  host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE,
                              TestHost::STAGE_2D_PROJECTIVE);

  host_.SetPointSize(64.f);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_POINT_SMOOTH_ENABLE, true);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_FACTOR_A, 0.f);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_FACTOR_B, 0.f);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_FACTOR_C, 2.7125650614578944e-09);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SIZE_RANGE, 0.9999799728393555);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SIZE_RANGE_DUP_1, 0.9999799728393555);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SIZE_RANGE_DUP_2, 0.9999799728393555);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_BIAS, -2.0000399672426283e-05);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_MIN_SIZE, 1.9999999494757503e-05);
  Pushbuffer::End();

  static constexpr float kMargin = 180.f;
  static constexpr float kTop = 96.f;
  static constexpr float kSpacing = 96.f;

  host_.SetDiffuse(0xFFFFFFFF);
  host_.SetBlend();

  auto render_row = [this](float left, float top) {
    auto draw_point = [this](float cx, float cy) {
      host_.Begin(TestHost::PRIMITIVE_POINTS);
      vector_t screen_point{cx, cy, 1.f, 1.f};
      vector_t transformed;
      host_.UnprojectPoint(transformed, screen_point);
      VectorCopyVector(screen_point, transformed);
      host_.SetVertex(screen_point);
      host_.End();
    };

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_PARAMS_ENABLE, false);
    Pushbuffer::End();

    // Render a normal textured point sprite.
    {
      host_.SetFinalCombiner0Just(TestHost::SRC_TEX3);
      host_.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);

      draw_point(left, top);
      left += kSpacing;
    }

    // Render the diffuse color with the point sprite texture still enabled but not referenced at all.
    {
      host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
      host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

      draw_point(left, top);
      left += kSpacing;
    }

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_PARAMS_ENABLE, true);
    Pushbuffer::End();
    // Render a normal textured point sprite.
    {
      host_.SetFinalCombiner0Just(TestHost::SRC_TEX3);
      host_.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);

      draw_point(left, top);
      left += kSpacing;
    }

    // Render the diffuse color with the point sprite texture still enabled but not referenced at all.
    {
      host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
      host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

      draw_point(left, top);
      left += kSpacing;
    }
  };

  float top = kTop;
  render_row(kMargin, top);
  top += kSpacing;

  // Enable alpha testing for alpha > 0 and do the same
  host_.SetAlphaFunc(true, NV097_SET_ALPHA_FUNC_V_GREATER);
  host_.SetAlphaReference(0x7F);
  render_row(kMargin, top);

  {
    // Cleanup
    host_.SetAlphaFunc(false);
    host_.SetPointSize(1.f);
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_SMOOTH_ENABLE, false);
    Pushbuffer::Push(NV097_SET_POINT_PARAMS_ENABLE, false);
    Pushbuffer::End();
  }
  pb_print("%s\n", kAlphaTestTest);
  pb_printat(3, 0, "No test");
  pb_printat(7, 0, "Test 0x7f");
  pb_draw_text_screen();

  FinishDraw(kAlphaTestTest);
}

static std::shared_ptr<PerspectiveVertexShader> SetupVertexShader(TestHost &host) {
  float depth_buffer_max_value = host.GetMaxDepthBufferValue();
  auto shader =
      std::make_shared<PerspectiveVertexShaderNoLighting>(host.GetFramebufferWidth(), host.GetFramebufferHeight(), 0.0f,
                                                          depth_buffer_max_value, M_PI * 0.25f, 1.0f, 200.0f);
  shader->SetUseD3DStyleViewport();
  vector_t camera_position = {0.0f, 0.0f, -7.0f, 1.0f};
  vector_t camera_look_at = {0.0f, 0.0f, 0.0f, 1.0f};
  shader->LookAt(camera_position, camera_look_at);

  return shader;
}

void PointSpriteTests::TestAlphaMasking(bool use_shader) {
  std::string test_name = std::string(kAlphaMaskTest) + (use_shader ? "_VS" : "_FF");

  std::shared_ptr<PerspectiveVertexShader> shader = use_shader ? SetupVertexShader(host_) : nullptr;
  host_.SetVertexShaderProgram(shader);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  static constexpr uint32_t kTextureWidth = 32;
  static constexpr uint32_t kTextureHeight = 32;

  // Generate a 32x32 swizzled texture with black RGB and alpha stripes/gradients.
  {
    uint32_t temp_pixels[kTextureWidth * kTextureHeight];
    auto pixel = &temp_pixels[0];
    for (uint32_t y = 0; y < kTextureHeight; ++y) {
      for (uint32_t x = 0; x < kTextureWidth; ++x) {
        // Vary alpha across both dimensions (e.g. horizontal bands with vertical ramp)
        // producing a rich mix of low, intermediate, and high alpha values.
        auto alpha = static_cast<uint8_t>(((x * 8) ^ (y * 8)) & 0xFF);
        *pixel++ = static_cast<uint32_t>(alpha) << 24;
      }
    }
    swizzle_rect(reinterpret_cast<const uint8_t *>(temp_pixels), kTextureWidth, kTextureHeight,
                 host_.GetTextureMemoryForStage(3), kTextureWidth * 4, 4);
  }

  // Draw against a light background to clearly see the black masking / alpha blend.
  host_.PrepareDraw(0xFFAACCCC);

  auto &texture_stage = host_.GetTextureStage(3);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));
  texture_stage.SetEnabled(true);
  texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
  texture_stage.SetUWrap(TextureStage::WRAP_CLAMP_TO_EDGE, false);
  texture_stage.SetVWrap(TextureStage::WRAP_CLAMP_TO_EDGE, false);
  texture_stage.SetPWrap(TextureStage::WRAP_REPEAT, false);
  texture_stage.SetFilter(0x1012000);  // Quincunx, BoxLOD0 min/mag
  host_.SetupTextureStages();

  host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE,
                              TestHost::STAGE_2D_PROJECTIVE);

  host_.ClearInputColorCombiners();
  host_.ClearInputAlphaCombiners();
  host_.ClearOutputColorCombiners();
  host_.ClearOutputAlphaCombiners();

  {
    // Combiner from the start screen of "Triangle Again 2"
    host_.SetCombinerControl(2);

    // Stage 0 Alpha: Diffuse.a * Tex3.a -> R0.a
    host_.SetInputAlphaCombiner(0, TestHost::AlphaInput(TestHost::SRC_DIFFUSE),
                                TestHost::AlphaInput(TestHost::SRC_TEX3));
    host_.SetOutputAlphaCombiner(0, TestHost::DST_R0);

    // Stage 1 Color: Diffuse.rgb * 1 -> R0.rgb
    host_.SetInputColorCombiner(1, TestHost::ColorInput(TestHost::SRC_DIFFUSE), TestHost::OneInput());
    host_.SetOutputColorCombiner(1, TestHost::DST_R0);

    // Final Combiner: R0.rgb, R0.a
    host_.SetFinalCombiner0Just(TestHost::SRC_R0);
    host_.SetFinalCombiner1Just(TestHost::SRC_R0, true);
  }

  // Blend: SRC_ALPHA, ONE_MINUS_SRC_ALPHA
  host_.SetBlend(true, NV097_SET_BLEND_EQUATION_V_FUNC_ADD, NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA,
                 NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_SRC_ALPHA);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_POINT_SMOOTH_ENABLE, true);
  Pushbuffer::Push(NV097_SET_POINT_PARAMS_ENABLE, true);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_FACTOR_A, 0.0204081628f);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_FACTOR_B, 0.0f);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_FACTOR_C, 0.0f);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SIZE_RANGE, 64.0f);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SIZE_RANGE_DUP_1, 64.0f);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SIZE_RANGE_DUP_2, 64.0f);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_BIAS, -0.0f);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_MIN_SIZE, 0.0f);
  Pushbuffer::End();

  // Diffuse: RGB = 0, Alpha = 1.0 (0xFF)
  host_.SetDiffuse(0.0f, 0.0f, 0.0f, 1.0f);

  auto unproject = [this, shader, use_shader](vector_t &world_point, float x, float y, float z) {
    vector_t screen_point{x, y, z, 1.f};
    if (!use_shader) {
      host_.UnprojectPoint(world_point, screen_point, z);
    } else {
      shader->UnprojectPoint(world_point, screen_point, z);
    }
  };

  auto draw_point = [this, &unproject, use_shader](float cx, float cy, float point_size) {
    vector_t world_point{0.f, 0.f, 0.f, 1.f};
    unproject(world_point, cx, cy, 1.f);

    if (use_shader) {
      // In programmable shader mode, NV097_SET_POINT_SIZE does not populate v6 (iPts).
      // Vertex attribute v6 must be supplied via a vertex array.
      auto buffer = host_.AllocateVertexBuffer(1);
      auto vertex = buffer->Lock();
      vertex->SetDiffuse(0.0f, 0.0f, 0.0f, 1.0f);
      vertex->SetPointSize(point_size);
      vertex->SetPosition(world_point[0], world_point[1], world_point[2], 1.f);
      vertex->SetTexCoord3(0.f, 0.f, 0.f, 1.f);
      buffer->Unlock();

      host_.SetVertexBuffer(buffer);
      host_.DrawArrays(TestHost::POSITION | TestHost::POINT_SIZE | TestHost::DIFFUSE | TestHost::TEXCOORD3,
                       TestHost::PRIMITIVE_POINTS);
      TestHost::PBKitBusyWait();
    } else {
      host_.Begin(TestHost::PRIMITIVE_POINTS);
      host_.SetPointSize(point_size);
      host_.SetVertex(world_point);
      host_.End();
    }
  };

  auto draw_quad = [this, &unproject](float left, float top, float size) {
    const auto right = left + size;
    const auto bottom = top + size;
    const auto world_z = 0.f;
    vector_t world_point{0.f, 0.f, 0.f, 1.f};

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord3(0.f, 0.f);
    unproject(world_point, left, top, world_z);
    host_.SetVertex(world_point);

    host_.SetTexCoord3(1.f, 0.f);
    unproject(world_point, right, top, world_z);
    host_.SetVertex(world_point);

    host_.SetTexCoord3(1.f, 1.f);
    unproject(world_point, right, bottom, world_z);
    host_.SetVertex(world_point);

    host_.SetTexCoord3(0.f, 1.f);
    unproject(world_point, left, bottom, world_z);
    host_.SetVertex(world_point);
    host_.End();
  };

  static constexpr float kColX[4] = {130.f, 250.f, 370.f, 490.f};
  static constexpr float kRowY[3] = {150.f, 250.f, 350.f};
  static constexpr float kSizes[3] = {32.f, 40.f, 48.f};

  for (size_t r = 0; r < 3; ++r) {
    float y = kRowY[r];
    float sz = kSizes[r];

    // Col 0: Alpha test OFF, point sprite
    host_.SetAlphaFunc(false);
    draw_point(kColX[0], y, sz);

    // Col 1: Alpha test GREATER than 1, point sprite
    host_.SetAlphaFunc(true, NV097_SET_ALPHA_FUNC_V_GREATER);
    host_.SetAlphaReference(1);
    draw_point(kColX[1], y, sz);

    // Col 2: Alpha test GREATER than 128, point sprite
    host_.SetAlphaFunc(true, NV097_SET_ALPHA_FUNC_V_GREATER);
    host_.SetAlphaReference(128);
    draw_point(kColX[2], y, sz);

    // Col 3: Comparison textured quad (Alpha test GREATER than 1)
    host_.SetAlphaFunc(true, NV097_SET_ALPHA_FUNC_V_GREATER);
    host_.SetAlphaReference(1);
    draw_quad(kColX[3] - sz * 0.5f, y - sz * 0.5f, sz);
  }

  host_.SetAlphaFunc(false);
  host_.SetPointSize(1.f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_POINT_SMOOTH_ENABLE, false);
  Pushbuffer::Push(NV097_SET_POINT_PARAMS_ENABLE, false);
  Pushbuffer::End();
  host_.SetVertexShaderProgram(nullptr);

  pb_printat(1, 1, "%s", test_name.c_str());
  pb_printat(3, 9, "NoTest      Ref 1      Ref 128      Quad");
  pb_printat(5, 1, "32px");
  pb_printat(9, 1, "40px");
  pb_printat(13, 1, "48px");
  pb_draw_text_screen();

  FinishDraw(test_name);
}
