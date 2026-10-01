#include "combiner_tests.h"

#include <pbkit/pbkit.h>

#include "pbkit_ext.h"
#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"
#include "texture_generator.h"
#include "vertex_buffer.h"

static constexpr const char* kMuxTestName = "Mux";
static constexpr const char* kIndependenceTestName = "Independence";
static constexpr const char* kColorAlphaIndependenceTestName = "ColorAlphaIndependence";
static constexpr const char* kFlagsTestName = "Flags";
static constexpr const char* kUnboundTextureSamplerTestName = "UnboundTexSampler";
static constexpr const char* kAlphaFromBlueTestName = "AlphaFromBlue";
static constexpr const char* kCombinerOpsTestName = "CombinerOps";
static constexpr const char* kFinalCombinerSpecialInputsTestName = "SpecialInputs";
static constexpr const char* kSignedCombinerOpsTestName = "SignedCombinerOps";
static constexpr const char* kSignedPassThroughPipelineTestName = "SignedPassThroughPipeline";
static constexpr const char* kSignedToUnsignedMappingTestName = "SignedToUnsignedMapping";

static constexpr vector_t kDiffuseUL{1.f, 0.f, 0.f, 1.f};
static constexpr vector_t kDiffuseUR{0.f, 1.f, 0.f, 1.f};
static constexpr vector_t kDiffuseLR{0.f, 0.f, 1.f, 1.f};
static constexpr vector_t kDiffuseLL{0.5f, 0.5f, 0.5f, 1.f};

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc ColorAlphaIndependence
 *   Demonstrates that setting color on a combiner register does not affect alpha.
 *
 * @tc Flags
 *   Tests behavior of specular_add_invert_r0, specular_add_invert_v1, and specular_clamp on final combiners.
 *
 * @tc Independence
 *   Demonstrates that setting a register's value in a combiner stage does not mutate the value until after it is
 *   assigned as an output. The test performs several draws where R0 is initialized to some color then both mutated and
 *   copied into R1, demonstrating that the original value of R0 is copied into R1 before being replaced with the new
 *   value.
 *
 * @tc Mux
 *   Tests behavior of the "MUX" combiner mode, in which R0.a is used to select between the AB and CD outputs.
 *
 * @tc UnboundTex
 *   Demonstrates that the alpha channel for unbound textures is set to 1.0.
 *
 * @tc AlphaFromBlue
 *   Demonstrates behavior of the "blue to alpha" flags.
 *
 * @tc CombinerOps
 *   Tests the various output operations.
 *
 * @tc SpecialInputs
 *   Tests the special input registers in the final combiner.
 *
 * @tc SignedCombinerOps
 *   Tests behavior of combiner output operations (OP_IDENTITY, OP_SHIFT_LEFT_1, OP_SHIFT_LEFT_2, OP_SHIFT_RIGHT_1,
 *   OP_BIAS) in conjunction with signed (negative) values. Verifies that negative values are properly scaled and
 *   clamped when added to a positive base value in subsequent stages.
 *
 * @tc SignedPassThroughPipeline
 *   Tests a 5-stage register combiner pipeline with OP_SHIFT_LEFT_2 where Tex3 operates in STAGE_PASS_THROUGH mode
 *   with negative values. Verifies blue-to-alpha routing (R1.a = Tex3.b), multiplication by Tex2, scaling by 4.0
 *   (OP_SHIFT_LEFT_2), and combination with the base diffuse color (R0 + R1) across positive, zero, and negative
 *   Tex3.b inputs.
 *
 * @tc SignedToUnsignedMapping
 *   Validates behavior when known signed (negative) values are mapped as MAP_UNSIGNED_IDENTITY versus
 *   MAP_SIGNED_IDENTITY across intermediate registers, pass-through texture coordinates, and a multi-stage pipeline.
 *   Verifies that MAP_UNSIGNED_IDENTITY properly clamps negative values to 0.0 (preserving base colors), while
 *   MAP_SIGNED_IDENTITY preserves negative values (allowing subtraction).
 */
CombinerTests::CombinerTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Combiner", config) {
  tests_[kMuxTestName] = [this]() { TestMux(); };
  tests_[kIndependenceTestName] = [this]() { TestCombinerIndependence(); };
  tests_[kColorAlphaIndependenceTestName] = [this]() { TestCombinerColorAlphaIndependence(); };
  tests_[kFlagsTestName] = [this]() { TestFlags(); };
  tests_[kUnboundTextureSamplerTestName] = [this]() { TestUnboundTextureSamplers(); };
  tests_[kAlphaFromBlueTestName] = [this]() { TestAlphaFromBlue(); };
  tests_[kCombinerOpsTestName] = [this]() { TestCombinerOps(); };
  tests_[kFinalCombinerSpecialInputsTestName] = [this]() { TestFinalCombinerSpecialInputs(); };
  tests_[kSignedCombinerOpsTestName] = [this]() { TestSignedCombinerOps(); };
  tests_[kSignedPassThroughPipelineTestName] = [this]() { TestSignedPassThroughPipeline(); };
  tests_[kSignedToUnsignedMappingTestName] = [this]() { TestSignedToUnsignedMapping(); };
}

void CombinerTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  CreateGeometry();
}

void CombinerTests::Deinitialize() {
  for (auto& buffer : vertex_buffers_) {
    buffer.reset();
  }
  TestSuite::Deinitialize();
}

void CombinerTests::CreateGeometry() {
  static constexpr float kLeft = -2.75f;
  static constexpr float kRight = 2.75f;
  static constexpr float kTop = 1.85f;
  static constexpr float kBottom = -1.75f;
  static const float kSpacing = 0.1f;
  static const float kWidth = kRight - kLeft;
  static const float kHeight = kTop - kBottom;
  static const float kUnitWidth = (kWidth - (3.0f * kSpacing)) / 4.0f;
  static const float kUnitHeight = (kHeight - (2.0f * kSpacing)) / 3.0f;

  Color c_one{0.0f, 1.0f, 0.0f};
  Color c_two{0.0f, 0.0f, 1.0f};
  Color c_three{1.0f, 0.0f, 0.0f};
  Color c_four{0.25f, 0.25f, 0.25f};

  float left = kLeft;
  float top = kTop - kSpacing;
  for (auto& buffer : vertex_buffers_) {
    uint32_t num_quads = 1;
    buffer = host_.AllocateVertexBuffer(6 * num_quads);

    const float z = 1.0f;
    buffer->DefineBiTri(0, left, top, left + kUnitWidth, top - kUnitHeight, z, z, z, z, c_one, c_two, c_three, c_four);

    left += kUnitWidth + kSpacing;
    if (left >= (kRight - kUnitWidth)) {
      left = kLeft;
      top -= kUnitHeight + kSpacing * 4.0f;
    }
  }
}

void CombinerTests::TestMux() {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  uint32_t vertex_elements = host_.POSITION | host_.DIFFUSE | host_.SPECULAR;

  host_.SetCombinerControl(2, false, false, false);

  // TODO: Test behavior when r0 is not explicitly set.
  host_.SetInputColorCombiner(0, TestHost::OneInput(), TestHost::OneInput());
  host_.SetOutputColorCombiner(0, TestHost::DST_R0);

  host_.SetOutputAlphaCombiner(0, TestHost::DST_R0);

  host_.SetCombinerFactorC0(1, 1.0f, 0.0f, 0.0f, 1.0f);
  host_.SetCombinerFactorC1(1, 0.0f, 0.0f, 1.0f, 1.0f);
  host_.SetInputColorCombiner(1, TestHost::ColorInput(TestHost::SRC_C0), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_C1), TestHost::OneInput());
  host_.SetOutputColorCombiner(1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_DIFFUSE, false, false,
                               TestHost::SM_MUX);

  host_.SetInputAlphaCombiner(1, TestHost::OneInput(), TestHost::OneInput());
  host_.SetOutputAlphaCombiner(1, TestHost::DST_DIFFUSE);

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  int row = 5;

  // Set an alpha value with the MSB set and the LSB unset.
  uint32_t c0 = 0x82000000;
  host_.SetCombinerFactorC0(0, c0);
  host_.SetInputAlphaCombiner(0, TestHost::AlphaInput(TestHost::SRC_C0), TestHost::OneInput());
  host_.SetCombinerControl(2, false, false, false);
  pb_printat(row, 11, (char*)"LSB 0x%x", (c0 >> 24));
  host_.SetVertexBuffer(vertex_buffers_[0]);
  host_.DrawArrays(vertex_elements);

  host_.SetCombinerControl(2, false, false, true);
  pb_printat(row, 21, (char*)"MSB 0x%x", (c0 >> 24));
  host_.SetVertexBuffer(vertex_buffers_[1]);
  host_.DrawArrays(vertex_elements);

  // Set an alpha value with the MSB and LSB set.
  c0 = 0x81000000;
  host_.SetCombinerFactorC0(0, c0);
  host_.SetCombinerControl(2, false, false, false);
  pb_printat(row, 31, (char*)"LSB 0x%x", (c0 >> 24));
  host_.SetVertexBuffer(vertex_buffers_[2]);
  host_.DrawArrays(vertex_elements);

  host_.SetCombinerControl(2, false, false, true);
  pb_printat(row, 41, (char*)"MSB 0x%x", (c0 >> 24));
  host_.SetVertexBuffer(vertex_buffers_[3]);
  host_.DrawArrays(vertex_elements);

  row = 9;

  // Set an alpha value with the MSB and LSB unset.
  c0 = 0x00000000;
  host_.SetCombinerFactorC0(0, c0);
  host_.SetCombinerControl(2, false, false, false);
  pb_printat(row, 11, (char*)"LSB 0x%x", (c0 >> 24));
  host_.SetVertexBuffer(vertex_buffers_[4]);
  host_.DrawArrays(vertex_elements);

  host_.SetCombinerControl(2, false, false, true);
  pb_printat(row, 21, (char*)"MSB 0x%x", (c0 >> 24));
  host_.SetVertexBuffer(vertex_buffers_[5]);
  host_.DrawArrays(vertex_elements);

  pb_printat(0, 0, (char*)"%s\n", kMuxTestName);
  pb_printat(1, 0, (char*)"Unset = Red");
  pb_printat(2, 0, (char*)"Set = Blue");
  pb_draw_text_screen();

  host_.SetCombinerControl();
  FinishDraw(kMuxTestName);
}

void CombinerTests::TestCombinerIndependence() {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  uint32_t vertex_elements = host_.POSITION | host_.DIFFUSE | host_.SPECULAR;

  host_.SetCombinerControl(2);

  // Turn R0 green.
  host_.SetCombinerFactorC0(0, 0.0f, 1.0f, 0.0f, 1.0f);
  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_C0), TestHost::OneInput());
  host_.SetOutputColorCombiner(0, TestHost::DST_R0);

  // Turn R0 red, and set R1 to the previous (green) value of R0.
  host_.SetCombinerFactorC0(1, 1.0f, 0.0f, 0.0f, 1.0f);
  host_.SetInputColorCombiner(1, TestHost::ColorInput(TestHost::SRC_C0), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_R0), TestHost::OneInput());
  host_.SetOutputColorCombiner(1, TestHost::DST_R0, TestHost::DST_R1);

  // Show a green quad.
  host_.SetFinalCombiner0Just(TestHost::SRC_R1);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  pb_printat(2, 6, (char*)"Green from r0 stage 0");
  host_.SetVertexBuffer(vertex_buffers_[0]);
  host_.DrawArrays(vertex_elements);

  host_.SetCombinerControl(3);

  // Set R0 blue to 25%
  host_.SetCombinerFactorC0(0, 0.0f, 0.0f, 0.25f, 0.0f);
  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_C0), TestHost::OneInput());
  host_.SetOutputColorCombiner(0, TestHost::DST_R0);

  // Turn R0 75% white and R1 50% white
  host_.SetCombinerFactorC0(1, 0.0f, 0.0f, 0.75f, 0.0f);
  host_.SetInputColorCombiner(1, TestHost::ColorInput(TestHost::SRC_C0), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_R0), TestHost::OneInput());
  host_.SetOutputColorCombiner(1, TestHost::DST_R0, TestHost::DST_R1, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY, true, true);

  host_.SetInputColorCombiner(2, TestHost::AlphaInput(TestHost::SRC_R0), TestHost::OneInput(),
                              TestHost::AlphaInput(TestHost::SRC_R1), TestHost::OneInput());
  host_.SetOutputColorCombiner(2, TestHost::DST_R0, TestHost::DST_R1);

  host_.SetFinalCombiner0Just(TestHost::SRC_R1);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  pb_printat(7, 20, (char*)"DGrey from r0 stage 1 alpha");
  host_.SetVertexBuffer(vertex_buffers_[2]);
  host_.DrawArrays(vertex_elements);

  host_.SetFinalCombiner0Just(TestHost::SRC_R0);
  pb_printat(12, 6, (char*)"LGrey from r0 stage 1");
  host_.SetVertexBuffer(vertex_buffers_[4]);
  host_.DrawArrays(vertex_elements);

  pb_printat(0, 0, (char*)"%s\n", kIndependenceTestName);
  pb_draw_text_screen();

  host_.SetCombinerControl();
  FinishDraw(kIndependenceTestName);
}

void CombinerTests::TestCombinerColorAlphaIndependence() {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  auto draw_quad = [this]() {
    static constexpr float kLeft = -2.75f;
    static constexpr float kRight = 2.75f;
    static constexpr float kTop = 1.75f;
    static constexpr float kBottom = -1.75f;
    static constexpr float z = 0.0f;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0.1f, 1.0f, 0.1f, 1.0f);
    host_.SetVertex(kLeft, kTop, z, 1.0f);
    host_.SetVertex(kRight, kTop, z, 1.0f);
    host_.SetVertex(kRight, kBottom, z, 1.0f);
    host_.SetVertex(kLeft, kBottom, z, 1.0f);
    host_.End();
  };

  // Draw a green quad.
  {
    host_.SetCombinerControl(1);
    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
    draw_quad();
  }

  // Overlay a transparent blue quad.
  {
    host_.SetCombinerControl(2);
    // Set R0 blue to 0%
    host_.SetCombinerFactorC0(0, 0.0f, 0.0f, 0.0f, 0.0f);
    host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_C0), TestHost::OneInput());
    host_.SetOutputColorCombiner(0, TestHost::DST_R0);

    // Turn R0 100% blue and set R1 alpha to R0 (which is 0% when entering this stage).
    host_.SetCombinerFactorC0(1, 0.0f, 0.0f, 1.0f, 1.0f);
    host_.SetInputColorCombiner(1, TestHost::ColorInput(TestHost::SRC_C0), TestHost::OneInput());
    host_.SetOutputColorCombiner(1, TestHost::DST_R0);

    host_.SetInputAlphaCombiner(1, TestHost::ColorInput(TestHost::SRC_R0), TestHost::OneInput());
    host_.SetOutputAlphaCombiner(1, TestHost::DST_R1);

    host_.SetFinalCombiner0Just(TestHost::SRC_R0);
    host_.SetFinalCombiner1Just(TestHost::SRC_R1, true);

    draw_quad();
  }

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  pb_print("%s\n", kColorAlphaIndependenceTestName);
  pb_print("Expect a green quad\n");
  pb_draw_text_screen();

  host_.SetCombinerControl();
  FinishDraw(kColorAlphaIndependenceTestName);
}

void CombinerTests::TestFlags() {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  uint32_t vertex_elements = host_.POSITION | host_.DIFFUSE | host_.SPECULAR;

  host_.SetCombinerControl(1);

  // Set V1 and R0 to 1.0
  host_.SetInputColorCombiner(0, TestHost::OneInput(), TestHost::OneInput(), TestHost::OneInput(),
                              TestHost::OneInput());
  host_.SetOutputColorCombiner(0, TestHost::DST_SPECULAR, TestHost::DST_R0);

  // Set the final output to (D=0) + (A=0.5) * (B=V1+R0) + (1 - A=0.5) * (C=0)
  // Set alpha (G) to 1.0
  host_.SetFinalCombinerFactorC0(0.5f, 0.5f, 0.5f, 0.5f);
  host_.SetFinalCombiner0(TestHost::SRC_C0, false, false, TestHost::SRC_SPEC_R0_SUM, false, false);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, true,
                          true);

  // The expected output is full brightness white.
  pb_printat(2, 10, (char*)"Uncapped");
  host_.SetVertexBuffer(vertex_buffers_[0]);
  host_.DrawArrays(vertex_elements);

  // Do the same thing, but clamp the V1+R0 sum
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, true,
                          true, false, false, true);
  pb_printat(2, 22, (char*)"Capped");
  host_.SetVertexBuffer(vertex_buffers_[1]);
  host_.DrawArrays(vertex_elements);

  // Set v1 to 0, r0 to 0.75.
  host_.SetCombinerFactorC0(0, 0.75f, 0.75f, 0.75f, 0.75f);
  host_.SetInputColorCombiner(0, TestHost::ZeroInput(), TestHost::ZeroInput(), TestHost::ColorInput(TestHost::SRC_C0),
                              TestHost::OneInput());
  host_.SetOutputColorCombiner(0, TestHost::DST_SPECULAR, TestHost::DST_R0);

  // Set A to 1.0 so the final output is just B(the V1 + R0 sum).
  host_.SetFinalCombiner0(TestHost::SRC_ZERO, false, true, TestHost::SRC_SPEC_R0_SUM, false, false);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, true,
                          true);

  pb_printat(2, 31, (char*)"Normal R0");
  host_.SetVertexBuffer(vertex_buffers_[2]);
  host_.DrawArrays(vertex_elements);

  // Now invert R0.
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, true,
                          true, true, false, false);

  pb_printat(2, 42, (char*)"1 - R0");
  host_.SetVertexBuffer(vertex_buffers_[3]);
  host_.DrawArrays(vertex_elements);

  // Essentially the same test, but using v1 instead of r0
  // Set r0 to 0, v1 to 0.75.
  host_.SetInputColorCombiner(0, TestHost::ZeroInput(), TestHost::ZeroInput(), TestHost::ColorInput(TestHost::SRC_C0),
                              TestHost::OneInput());
  host_.SetOutputColorCombiner(0, TestHost::DST_R0, TestHost::DST_SPECULAR);

  // Set A to 1.0 so the final output is just B(the V1 + R0 sum).
  host_.SetFinalCombiner0(TestHost::SRC_ZERO, false, true, TestHost::SRC_SPEC_R0_SUM, false, false);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, true,
                          true);

  pb_printat(7, 14, (char*)"V1");
  host_.SetVertexBuffer(vertex_buffers_[4]);
  host_.DrawArrays(vertex_elements);

  // Now invert V1.
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, true,
                          true, false, true, false);

  pb_printat(7, 23, (char*)"1 - V1");
  host_.SetVertexBuffer(vertex_buffers_[5]);
  host_.DrawArrays(vertex_elements);

  pb_printat(0, 0, (char*)"%s\n", kFlagsTestName);
  pb_draw_text_screen();

  host_.SetCombinerControl();
  FinishDraw(kFlagsTestName);
}

void CombinerTests::TestUnboundTextureSamplers() {
  static constexpr uint32_t kBackgroundColor = 0xFF6A6A6A;
  host_.PrepareDraw(kBackgroundColor);

  auto unproject = [this](vector_t& world_point, float x, float y, float z) {
    vector_t screen_point{x, y, z, 1.f};
    host_.UnprojectPoint(world_point, screen_point, z);
  };

  static constexpr auto kQuadSize = 64.f;
  static constexpr float kQuadZ = 0.f;

  auto draw_quad = [this, unproject](float left, float top) {
    const auto right = left + kQuadSize;
    const auto bottom = top + kQuadSize;

    host_.Begin(TestHost::PRIMITIVE_QUADS);

    vector_t world_point{0.f, 0.f, 0.f, 1.f};

    host_.SetDiffuse(kDiffuseUL);
    unproject(world_point, left, top, kQuadZ);
    host_.SetVertex(world_point);

    host_.SetDiffuse(kDiffuseUR);
    unproject(world_point, right, top, kQuadZ);
    host_.SetVertex(world_point);

    host_.SetDiffuse(kDiffuseLR);
    unproject(world_point, right, bottom, kQuadZ);
    host_.SetVertex(world_point);

    host_.SetDiffuse(kDiffuseLL);
    unproject(world_point, left, bottom, kQuadZ);
    host_.SetVertex(world_point);

    host_.End();
  };

  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  static constexpr auto kQuadSpacing = kQuadSize + 8.f;
  const auto kLeft = floor((host_.GetFramebufferWidthF() - (kQuadSize + kQuadSpacing)) * 0.5f);
  float top = 96.f;

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  draw_quad(kLeft, top);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0, true);
  draw_quad(kLeft + kQuadSpacing, top);

  top += kQuadSpacing;
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX1);
  draw_quad(kLeft, top);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX1, true);
  draw_quad(kLeft + kQuadSpacing, top);

  top += kQuadSpacing;
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX2);
  draw_quad(kLeft, top);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX2, true);
  draw_quad(kLeft + kQuadSpacing, top);

  top += kQuadSpacing;
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX3);
  draw_quad(kLeft, top);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX3, true);
  draw_quad(kLeft + kQuadSpacing, top);

  pb_printat(0, 0, "%s", kUnboundTextureSamplerTestName);
  pb_printat(4, 18, "tex0");
  pb_printat(7, 18, "tex1");
  pb_printat(10, 18, "tex2");
  pb_printat(12, 18, "tex3");
  pb_draw_text_screen();

  host_.SetCombinerControl();
  FinishDraw(kUnboundTextureSamplerTestName);
}

void CombinerTests::TestAlphaFromBlue() {
  static constexpr uint32_t kBackgroundColor = 0xFF6A6A6A;
  host_.PrepareDraw(kBackgroundColor);

  host_.DrawCheckerboardUnproject(0xFF333333, 0xFF444444);

  auto unproject = [this](vector_t& world_point, float x, float y, float z) {
    vector_t screen_point{x, y, z, 1.f};
    host_.UnprojectPoint(world_point, screen_point, z);
  };

  static constexpr auto kQuadSize = 64.f;
  static constexpr float kQuadZ = 0.f;
  static constexpr vector_t kDiffuse{1.f, 1.f, 1.f, 0.f};

  auto draw_quad = [this, unproject](float left, float top) {
    auto right = left + kQuadSize * 0.5f;
    const auto bottom = top + kQuadSize;

    vector_t world_point{0.f, 0.f, 0.f, 1.f};
    host_.SetDiffuse(kDiffuse);

    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    unproject(world_point, left, top, kQuadZ);
    host_.SetVertex(world_point);
    unproject(world_point, right, top, kQuadZ);
    host_.SetVertex(world_point);
    unproject(world_point, right, bottom, kQuadZ);
    host_.SetVertex(world_point);
    unproject(world_point, left, bottom, kQuadZ);
    host_.SetVertex(world_point);
    host_.End();

    host_.SetFinalCombiner1Just(TestHost::SRC_R0, true, false);
    left = right;
    right += kQuadSize * 0.5f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    unproject(world_point, left, top, kQuadZ);
    host_.SetVertex(world_point);
    unproject(world_point, right, top, kQuadZ);
    host_.SetVertex(world_point);
    unproject(world_point, right, bottom, kQuadZ);
    host_.SetVertex(world_point);
    unproject(world_point, left, bottom, kQuadZ);
    host_.SetVertex(world_point);

    host_.End();
  };

  host_.SetOutputAlphaCombiner(0, TestHost::DST_DISCARD);
  host_.SetFinalCombiner0Just(TestHost::SRC_R0);

  static constexpr auto kQuadSpacing = kQuadSize + 8.f;
  const auto kLeft = 128.f;
  float left = kLeft;
  float top = 192.f;

  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_C0), TestHost::ColorInput(TestHost::SRC_DIFFUSE),
                              TestHost::ColorInput(TestHost::SRC_C0), TestHost::ColorInput(TestHost::SRC_DIFFUSE));

  host_.SetOutputColorCombiner(0, TestHost::DST_R0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY, true);

  host_.SetCombinerFactorC0(0, 0.25f, 0.5f, 1.f, 0.f);
  draw_quad(left, top);
  left += kQuadSpacing;
  host_.SetCombinerFactorC0(0, 0.25f, 0.5f, 0.75f, 0.f);
  draw_quad(left, top);
  left += kQuadSpacing;
  host_.SetCombinerFactorC0(0, 0.25f, 0.5f, 0.5f, 0.f);
  draw_quad(left, top);
  left += kQuadSpacing;
  host_.SetCombinerFactorC0(0, 0.25f, 0.5f, 0.25f, 0.f);
  draw_quad(left, top);
  left += kQuadSpacing;
  host_.SetCombinerFactorC0(0, 0.25f, 0.5f, 0.f, 0.f);
  draw_quad(left, top);

  host_.SetOutputColorCombiner(0, TestHost::DST_DISCARD, TestHost::DST_R0, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY, false, true);
  left = kLeft;
  top += kQuadSpacing;
  host_.SetCombinerFactorC0(0, 0.25f, 0.5f, 1.f, 0.f);
  draw_quad(left, top);
  left += kQuadSpacing;
  host_.SetCombinerFactorC0(0, 0.25f, 0.5f, 0.75f, 0.f);
  draw_quad(left, top);
  left += kQuadSpacing;
  host_.SetCombinerFactorC0(0, 0.25f, 0.5f, 0.5f, 0.f);
  draw_quad(left, top);
  left += kQuadSpacing;
  host_.SetCombinerFactorC0(0, 0.25f, 0.5f, 0.25f, 0.f);
  draw_quad(left, top);
  left += kQuadSpacing;
  host_.SetCombinerFactorC0(0, 0.25f, 0.5f, 0.f, 0.f);
  draw_quad(left, top);

  pb_printat(0, 0, "%s", kAlphaFromBlueTestName);
  pb_printat(2, 0, "The left half of each quad has alpha forced to 1.");
  pb_printat(3, 0, "The right is taken from the final combiner blue channel.");
  pb_printat(8, 8, "AB");
  pb_printat(11, 8, "CD");
  pb_draw_text_screen();

  host_.SetCombinerControl();
  FinishDraw(kAlphaFromBlueTestName);
}

void CombinerTests::TestCombinerOps() {
  static constexpr uint32_t kBackgroundColor = 0xFF6A6A6A;
  host_.PrepareDraw(kBackgroundColor);

  host_.DrawCheckerboardUnproject(0xFF333333, 0xFF444444);

  auto unproject = [this](vector_t& world_point, float x, float y, float z) {
    vector_t screen_point{x, y, z, 1.f};
    host_.UnprojectPoint(world_point, screen_point, z);
  };

  static constexpr auto kQuadSize = 64.f;
  static constexpr float kQuadZ = 0.f;

  auto draw_quad = [this, unproject](float left, float top) {
    auto right = left + kQuadSize * 0.5f;
    const auto bottom = top + kQuadSize;

    vector_t world_point{0.f, 0.f, 0.f, 1.f};

    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    unproject(world_point, left, top, kQuadZ);
    host_.SetVertex(world_point);
    unproject(world_point, right, top, kQuadZ);
    host_.SetVertex(world_point);
    unproject(world_point, right, bottom, kQuadZ);
    host_.SetVertex(world_point);
    unproject(world_point, left, bottom, kQuadZ);
    host_.SetVertex(world_point);
    host_.End();

    host_.SetFinalCombiner1Just(TestHost::SRC_R0, true, false);
    left = right;
    right += kQuadSize * 0.5f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    unproject(world_point, left, top, kQuadZ);
    host_.SetVertex(world_point);
    unproject(world_point, right, top, kQuadZ);
    host_.SetVertex(world_point);
    unproject(world_point, right, bottom, kQuadZ);
    host_.SetVertex(world_point);
    unproject(world_point, left, bottom, kQuadZ);
    host_.SetVertex(world_point);

    host_.End();
  };

  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_C0), TestHost::OneInput());
  host_.SetInputAlphaCombiner(0, TestHost::ColorInput(TestHost::SRC_C0), TestHost::OneInput());
  host_.SetFinalCombiner0Just(TestHost::SRC_R0);

  static constexpr auto kQuadSpacing = kQuadSize + 8.f;
  const auto kTop = 74.f;
  float left = 200.f;
  float top = kTop;

  auto set_op = [this](TestHost::CombinerOutOp op) {
    host_.SetOutputColorCombiner(0, TestHost::DST_R0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                                 TestHost::SM_SUM, op);
    host_.SetOutputAlphaCombiner(0, TestHost::DST_R0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                                 TestHost::SM_SUM, op);
  };

  host_.SetCombinerFactorC0(0, 0.1f, 0.25f, 1.f, 0.5f);

  set_op(TestHost::OP_IDENTITY);
  draw_quad(left, top);
  pb_printat(3, 0, "Identity");
  top += kQuadSpacing;

  set_op(TestHost::OP_BIAS);
  draw_quad(left, top);
  pb_printat(6, 0, "Bias");
  top += kQuadSpacing;

  set_op(TestHost::OP_SHIFT_LEFT_1);
  draw_quad(left, top);
  pb_printat(9, 0, "Shift Left 1");
  top += kQuadSpacing;

  set_op(TestHost::OP_SHIFT_LEFT_1_BIAS);
  draw_quad(left, top);
  pb_printat(12, 0, "Shift Left 1 Bias");
  top += kQuadSpacing;

  set_op(TestHost::OP_SHIFT_LEFT_2);
  draw_quad(left, top);
  pb_printat(15, 0, "Shift Left 2");
  top = kTop;
  left = 500.f;

  set_op(TestHost::OP_SHIFT_RIGHT_1);
  draw_quad(left, top);
  pb_printat(3, 30, "Shift Right 1");

  pb_printat(0, 0, "%s", kCombinerOpsTestName);
  pb_draw_text_screen();

  host_.SetCombinerControl();
  FinishDraw(kCombinerOpsTestName);
}

void CombinerTests::TestFinalCombinerSpecialInputs() {
  static constexpr uint32_t kBackgroundColor = 0xFF6A6A6A;
  host_.PrepareDraw(kBackgroundColor);

  host_.DrawCheckerboardUnproject(0xFF333333, 0xFF444444);

  auto unproject = [this](vector_t& world_point, float x, float y, float z) {
    vector_t screen_point{x, y, z, 1.f};
    host_.UnprojectPoint(world_point, screen_point, z);
  };

  static constexpr auto kQuadSize = 64.f;
  static constexpr float kQuadZ = 0.f;

  auto draw_quad = [this, unproject](float left, float top) {
    auto right = left + kQuadSize;
    const auto bottom = top + kQuadSize;

    vector_t world_point{0.f, 0.f, 0.f, 1.f};

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    unproject(world_point, left, top, kQuadZ);
    host_.SetVertex(world_point);
    unproject(world_point, right, top, kQuadZ);
    host_.SetVertex(world_point);
    unproject(world_point, right, bottom, kQuadZ);
    host_.SetVertex(world_point);
    unproject(world_point, left, bottom, kQuadZ);
    host_.SetVertex(world_point);
    host_.End();
  };

  static constexpr auto kQuadSpacing = kQuadSize + 8.f;
  static constexpr auto kTop = 74.f;
  static constexpr auto kLeftCol = 200.f;
  static constexpr auto kRightCol = 500.f;
  float top = kTop;

  host_.SetFinalCombinerFactorC0(1.f, 0.75f, 0.5f, 1.f);
  host_.SetFinalCombinerFactorC1(0.5f, 0.5f, 0.5f, 0.5f);
  host_.SetFinalCombiner0Just(TestHost::SRC_EF_PROD);
  host_.SetFinalCombiner1(TestHost::SRC_C0, false, false, TestHost::SRC_C1, false, false, TestHost::SRC_ZERO, true,
                          true);
  draw_quad(kLeftCol, top);
  pb_printat(3, 0, "EFProd");

  auto set_spec_flags = [this](bool specular_add_invert_r0 = false, bool specular_add_invert_v1 = false,
                               bool specular_clamp = false) {
    host_.SetFinalCombiner1(TestHost::SRC_C0, false, false, TestHost::SRC_C1, false, false, TestHost::SRC_ZERO, true,
                            true, specular_add_invert_r0, specular_add_invert_v1, specular_clamp);
  };

  pb_printat(5, 22, "SPEC_R0_SUM");
  host_.SetCombinerFactorC0(0, 0.25f, 0.5f, 0.75f, 0.f);
  host_.SetCombinerFactorC1(0, 0.70f, 0.5f, 0.15f, 0.5f);
  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_C0), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_C1), TestHost::OneInput());
  host_.SetInputAlphaCombiner(0, TestHost::ColorInput(TestHost::SRC_C0), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_C1), TestHost::OneInput());
  host_.SetOutputColorCombiner(0, TestHost::DST_R0, TestHost::DST_SPECULAR);
  host_.SetFinalCombiner0Just(TestHost::SRC_SPEC_R0_SUM);

  top += kQuadSpacing + 24.f;
  set_spec_flags(false, false, false);
  draw_quad(kLeftCol, top);
  pb_printat(7, 0, "No flags");

  pb_printat(7, 30, "INV R0");
  set_spec_flags(true, false, false);
  draw_quad(kRightCol, top);

  top += kQuadSpacing;

  pb_printat(10, 0, "INV SPEC");
  set_spec_flags(false, true, false);
  draw_quad(kLeftCol, top);

  // TODO: Figure out how to get clamp to something interesting.
  // The inputs to the SPEC_R0_SUM operation are always clamped to [0..1], and it seems that the sum is also always
  // clamped to 0..1 even without the flag.
  // The code below attempts to have SPEC_R0_SUM be > 1, then inverts it to get a negative value which is added to R1.
  // In practice, the (1 - SPEC_R0_SUM) is set to 0 even without the clamp, so this just writes R1 in both cases.
  // {
  //   pb_printat(12, 11, "SPEC_R0_SUM - Clamp (negative R0)");
  //   top += kQuadSpacing + 30.f;
  //   host_.SetCombinerControl(2, true, true);
  //
  //   host_.SetCombinerFactorC0(0, 1.f, 0.5f, 0.1f, 0.25f);
  //   host_.SetCombinerFactorC1(0, 1.f, 0.25f, 0.1f, 0.5f);
  //
  //   host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_C0), TestHost::OneInput(),
  //                               TestHost::ColorInput(TestHost::SRC_C1), TestHost::OneInput());
  //   host_.SetOutputColorCombiner(0, TestHost::DST_R0, TestHost::DST_SPECULAR, TestHost::DST_DISCARD, false, false,
  //                                TestHost::SM_SUM);
  //
  //   host_.SetInputColorCombiner(1, TestHost::ColorInput(TestHost::SRC_C0), TestHost::OneInput());
  //   host_.SetOutputColorCombiner(1, TestHost::DST_R1);
  //
  //   // rgb = 1 - (spec + r0) + mix(r1, 0, 0)
  //   host_.SetFinalCombiner0(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_R1,
  //   false,
  //                           false, TestHost::SRC_SPEC_R0_SUM, false, true);
  //
  //   pb_printat(14, 0, "No clamp");
  //   set_spec_flags(false, false, false);
  //   draw_quad(kLeftCol, top);
  //
  //   pb_printat(14, 30, "CLAMP");
  //   set_spec_flags(false, false, true);
  //   draw_quad(kRightCol, top);
  //
  //   host_.SetCombinerControl(1);
  // }

  pb_printat(0, 0, "%s", kFinalCombinerSpecialInputsTestName);
  pb_draw_text_screen();

  host_.SetCombinerControl();
  FinishDraw(kFinalCombinerSpecialInputsTestName);
}

void CombinerTests::TestSignedCombinerOps() {
  static constexpr uint32_t kBackgroundColor = 0xFF6A6A6A;
  host_.PrepareDraw(kBackgroundColor);

  host_.DrawCheckerboardUnproject(0xFF333333, 0xFF444444);

  static constexpr auto kQuadSize = 36.f;
  auto draw_quad = [this](float left, float top) {
    auto right = left + kQuadSize;
    const auto bottom = top + kQuadSize;

    host_.SetFinalCombiner0Just(TestHost::SRC_R0, false);
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetScreenVertex(left, top);
    host_.SetScreenVertex(right, top);
    host_.SetScreenVertex(right, bottom);
    host_.SetScreenVertex(left, bottom);
    host_.End();

    host_.SetFinalCombiner0Just(TestHost::SRC_R0, true);
    left = right + 4.f;
    right = left + (kQuadSize * 0.5f);
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetScreenVertex(left, top);
    host_.SetScreenVertex(right, top);
    host_.SetScreenVertex(right, bottom);
    host_.SetScreenVertex(left, bottom);
    host_.End();
  };

  host_.SetCombinerControl(2);

  // Stage 0: R1 = op(-C0)
  // Input: A = C0 (MAP_SIGNED_NEGATE), B = 1.0 (MAP_UNSIGNED_IDENTITY) -> AB = -C0
  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_C0, TestHost::MAP_SIGNED_NEGATE),
                              TestHost::OneInput());
  host_.SetInputAlphaCombiner(0, TestHost::AlphaInput(TestHost::SRC_C0, TestHost::MAP_SIGNED_NEGATE),
                              TestHost::OneInput());

  // Stage 1: R0 = C1 + R1
  // Input: A = C1, B = 1.0, C = R1 (MAP_SIGNED_IDENTITY), D = 1.0 -> AB + CD = C1 + R1
  host_.SetCombinerFactorC1(1, 0.75f, 0.75f, 0.75f, 0.75f);
  host_.SetInputColorCombiner(1, TestHost::ColorInput(TestHost::SRC_C1), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_R1, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::OneInput());
  host_.SetInputAlphaCombiner(1, TestHost::AlphaInput(TestHost::SRC_C1), TestHost::OneInput(),
                              TestHost::AlphaInput(TestHost::SRC_R1, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::OneInput());
  host_.SetOutputColorCombiner(1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY);
  host_.SetOutputAlphaCombiner(1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY);

  auto set_op = [this](TestHost::CombinerOutOp op) {
    host_.SetOutputColorCombiner(0, TestHost::DST_R1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                                 TestHost::SM_SUM, op);
    host_.SetOutputAlphaCombiner(0, TestHost::DST_R1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                                 TestHost::SM_SUM, op);
  };

  static constexpr auto kQuadSpacingY = 60.f;
  static constexpr auto kTop = 80.f;
  static constexpr auto kLeftCol = 230.f;
  static constexpr auto kRightCol = 540.f;

  float top = kTop;
  host_.SetCombinerFactorC0(0, 0.125f, 0.125f, 0.125f, 0.125f);

  set_op(TestHost::OP_IDENTITY);
  draw_quad(kLeftCol, top);
  pb_printat(3, 0, "Identity (0.625)");
  top += kQuadSpacingY;

  set_op(TestHost::OP_SHIFT_LEFT_1);
  draw_quad(kLeftCol, top);
  pb_printat(6, 0, "Shift Left 1 (0.500)");
  top += kQuadSpacingY;

  set_op(TestHost::OP_SHIFT_LEFT_2);
  draw_quad(kLeftCol, top);
  pb_printat(9, 0, "Shift Left 2 (0.250)");

  top = kTop;
  set_op(TestHost::OP_SHIFT_RIGHT_1);
  draw_quad(kRightCol, top);
  pb_printat(3, 31, "Shift Right 1 (0.688)");
  top += kQuadSpacingY;

  set_op(TestHost::OP_BIAS);
  draw_quad(kRightCol, top);
  pb_printat(6, 31, "Bias (0.125)");
  top += kQuadSpacingY;

  // Saturating test: -0.375 * 4 = -1.5 -> clamped to -1.0. 0.75 + (-1.0) = -0.25 -> clamped to 0.0
  host_.SetCombinerFactorC0(0, 0.375f, 0.375f, 0.375f, 0.375f);
  set_op(TestHost::OP_SHIFT_LEFT_2);
  draw_quad(kRightCol, top);
  pb_printat(9, 31, "Shift Left 2 Sat (0.0)");

  pb_printat(0, 0, "%s", kSignedCombinerOpsTestName);
  pb_printat(1, 0, "Stage 0: R1 = op(-C0). Stage 1: R0 = 0.75 + R1");
  pb_printat(12, 0, "Each quad: Left = RGB result, Right = Alpha result");
  pb_printat(13, 0, "Shift Left 2 Sat: Input -0.375 * 4 = -1.5 -> clamp -1.0");
  pb_printat(14, 0, "Expected: darker = more negative subtraction from 0.75");
  pb_draw_text_screen();

  host_.SetCombinerControl();
  FinishDraw(kSignedCombinerOpsTestName);
}

void CombinerTests::TestSignedPassThroughPipeline() {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  host_.DrawCheckerboardUnproject(0xFF222222, 0xFF333333);

  // Setup texture stages:
  // Stages 0, 2, and 3 use STAGE_PASS_THROUGH so vertex texcoords act directly as color/inputs.
  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, true);
  host_.SetTextureStageEnabled(3, true);
  host_.SetupTextureStages();

  host_.SetShaderStageProgram(TestHost::STAGE_PASS_THROUGH, TestHost::STAGE_NONE, TestHost::STAGE_PASS_THROUGH,
                              TestHost::STAGE_PASS_THROUGH);

  // Configure 5 general combiner stages
  host_.SetCombinerControl(5);

  // Stage 0:
  // R0Temp.rgb = SIGNED_IDENTITY(Tex0.rgb) * SIGNED_IDENTITY(V0_Diffuse.rgb)
  // R0Temp.a   = SIGNED_IDENTITY(Tex0.a) * (1.a)
  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_TEX0, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::ColorInput(TestHost::SRC_DIFFUSE, TestHost::MAP_SIGNED_IDENTITY));
  host_.SetOutputColorCombiner(0, TestHost::DST_R0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY);
  host_.SetInputAlphaCombiner(0, TestHost::AlphaInput(TestHost::SRC_TEX0, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::OneInput());
  host_.SetOutputAlphaCombiner(0, TestHost::DST_R0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY);

  // Stage 1:
  // R1Temp.rgb = SIGNED_IDENTITY(Tex3.rgb) * (1.rgb)
  // R1Temp.a   = R1Temp.b (via alpha_from_ab_blue)
  host_.SetInputColorCombiner(1, TestHost::ColorInput(TestHost::SRC_TEX3, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::OneInput());
  host_.SetOutputColorCombiner(1, TestHost::DST_R1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY, /*alpha_from_ab_blue=*/true, false);
  host_.SetOutputAlphaCombiner(1, TestHost::DST_DISCARD);

  // Stage 2:
  // R1Temp.rgb = SIGNED_IDENTITY(Tex2.rgb) * SIGNED_IDENTITY(R1Temp.a) + (0.rgb) * (0.rgb)
  // R1Temp.a   = SIGNED_IDENTITY(R0Temp.a) * (1.a)
  host_.SetInputColorCombiner(2, TestHost::ColorInput(TestHost::SRC_TEX2, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::AlphaInput(TestHost::SRC_R1, TestHost::MAP_SIGNED_IDENTITY));
  host_.SetOutputColorCombiner(2, TestHost::DST_R1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY);
  host_.SetInputAlphaCombiner(2, TestHost::AlphaInput(TestHost::SRC_R0, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::OneInput());
  host_.SetOutputAlphaCombiner(2, TestHost::DST_R1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY);

  // Stage 3:
  // R1Temp.rgb = (SIGNED_IDENTITY(R1Temp.rgb) * SIGNED_IDENTITY(R0Temp.rgb)) * 4.0
  // R1Temp.a   = SIGNED_IDENTITY(R0Temp.a) * (1.a)
  host_.SetInputColorCombiner(3, TestHost::ColorInput(TestHost::SRC_R1, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::ColorInput(TestHost::SRC_R0, TestHost::MAP_SIGNED_IDENTITY));
  host_.SetOutputColorCombiner(3, TestHost::DST_R1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_SHIFT_LEFT_2);
  host_.SetInputAlphaCombiner(3, TestHost::AlphaInput(TestHost::SRC_R0, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::OneInput());
  host_.SetOutputAlphaCombiner(3, TestHost::DST_R1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY);

  // Stage 4:
  // R0Temp.rgb = SIGNED_IDENTITY(R0Temp.rgb) * (1.rgb) + SIGNED_IDENTITY(R1Temp.rgb) * (1.rgb)
  // R0Temp.a   = SIGNED_IDENTITY(R0Temp.a) * (1.a)
  host_.SetInputColorCombiner(
      4, TestHost::ColorInput(TestHost::SRC_R0, TestHost::MAP_SIGNED_IDENTITY), TestHost::OneInput(),
      TestHost::ColorInput(TestHost::SRC_R1, TestHost::MAP_SIGNED_IDENTITY), TestHost::OneInput());
  host_.SetOutputColorCombiner(4, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY);
  host_.SetInputAlphaCombiner(4, TestHost::AlphaInput(TestHost::SRC_R0, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::OneInput());
  host_.SetOutputAlphaCombiner(4, TestHost::DST_R0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY);

  // Final combiner:
  // out.rgb = mix(Fog.rgb, R0Temp.rgb, Fog.a)
  // out.a   = R0Temp.a
  host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_FOG, false, false, TestHost::SRC_R0, false,
                          false, TestHost::SRC_ZERO, false, false);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, true, TestHost::SRC_ZERO, false, true, TestHost::SRC_R0, true,
                          false);

  static constexpr auto kQuadSize = 36.f;
  auto draw_quad = [this](float left, float top, float tex3_b) {
    auto right = left + kQuadSize;
    const auto bottom = top + kQuadSize;

    // Left quad: Final Combiner RGB
    host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_FOG, false, false, TestHost::SRC_R0, false,
                            false, TestHost::SRC_ZERO, false, false);
    host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, true, TestHost::SRC_ZERO, false, true, TestHost::SRC_R0, true,
                            false);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(1.f, 1.f, 1.f, 1.f);
    host_.SetTexCoord0(0.80f, 0.80f, 0.80f, 1.f);
    host_.SetTexCoord2(0.50f, 0.50f, 0.50f, 1.f);
    host_.SetTexCoord3(0.f, 0.f, tex3_b, 1.f);

    host_.SetScreenVertex(left, top);
    host_.SetScreenVertex(right, top);
    host_.SetScreenVertex(right, bottom);
    host_.SetScreenVertex(left, bottom);
    host_.End();

    // Right quad: R0 Alpha shown as RGB
    host_.SetFinalCombiner0Just(TestHost::SRC_R0, true, false);
    left = right + 4.f;
    right = left + (kQuadSize * 0.5f);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(1.f, 1.f, 1.f, 1.f);
    host_.SetTexCoord0(0.80f, 0.80f, 0.80f, 1.f);
    host_.SetTexCoord2(0.50f, 0.50f, 0.50f, 1.f);
    host_.SetTexCoord3(0.f, 0.f, tex3_b, 1.f);

    host_.SetScreenVertex(left, top);
    host_.SetScreenVertex(right, top);
    host_.SetScreenVertex(right, bottom);
    host_.SetScreenVertex(left, bottom);
    host_.End();
  };

  static constexpr auto kQuadSpacingY = 60.f;
  static constexpr auto kTop = 80.f;
  static constexpr auto kLeftCol = 220.f;
  static constexpr auto kRightCol = 520.f;

  float top = kTop;
  draw_quad(kLeftCol, top, 0.10f);
  pb_printat(3, 0, "Tex3.b=+0.10: 0.96");
  top += kQuadSpacingY;

  draw_quad(kLeftCol, top, 0.00f);
  pb_printat(6, 0, "Tex3.b= 0.00: 0.80");
  top += kQuadSpacingY;

  draw_quad(kLeftCol, top, -0.10f);
  pb_printat(9, 0, "Tex3.b=-0.10: 0.64");

  top = kTop;
  draw_quad(kRightCol, top, -0.25f);
  pb_printat(3, 30, "Tex3.b=-0.25: 0.40");
  top += kQuadSpacingY;

  draw_quad(kRightCol, top, -0.50f);
  pb_printat(6, 30, "Tex3.b=-0.50: 0.00");
  top += kQuadSpacingY;

  draw_quad(kRightCol, top, -0.80f);
  pb_printat(9, 30, "Tex3.b=-0.80: 0.00");

  pb_printat(0, 0, "%s", kSignedPassThroughPipelineTestName);
  pb_printat(1, 0, "R0 = Tex0*Diff + 4*(Tex2*Tex3.b*R0). R0 base = 0.80");
  pb_printat(12, 0, "Tex3 in STAGE_PASS_THROUGH mode (SetTexCoord3 p=Tex3.b)");
  pb_printat(13, 0, "Tex0, Tex2 in STAGE_PASS_THROUGH mode. Diffuse = 1.0");
  pb_printat(14, 0, "Each quad: Left = Final RGB, Right = Final Alpha");
  pb_draw_text_screen();

  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(2, false);
  host_.SetTextureStageEnabled(3, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  host_.SetCombinerControl();
  FinishDraw(kSignedPassThroughPipelineTestName);
}

void CombinerTests::TestSignedToUnsignedMapping() {
  static constexpr uint32_t kBackgroundColor = 0xFF6A6A6A;
  host_.PrepareDraw(kBackgroundColor);

  host_.DrawCheckerboardUnproject(0xFF333333, 0xFF444444);

  static constexpr auto kQuadWidth = 40.f;
  static constexpr auto kQuadHeight = 20.f;

  auto draw_quad = [this](float left, float top) {
    auto right = left + kQuadWidth;
    const auto bottom = top + kQuadHeight;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetScreenVertex(left, top);
    host_.SetScreenVertex(right, top);
    host_.SetScreenVertex(right, bottom);
    host_.SetScreenVertex(left, bottom);
    host_.End();
  };

  auto draw_tex_quad = [this](float left, float top, float tex_val) {
    auto right = left + kQuadWidth;
    const auto bottom = top + kQuadHeight;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(tex_val, tex_val, tex_val, 1.f);
    host_.SetScreenVertex(left, top);
    host_.SetScreenVertex(right, top);
    host_.SetScreenVertex(right, bottom);
    host_.SetScreenVertex(left, bottom);
    host_.End();
  };

  auto draw_pipe_quad = [this](float left, float top, float tex3_b) {
    auto right = left + kQuadWidth;
    const auto bottom = top + kQuadHeight;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(1.f, 1.f, 1.f, 1.f);
    host_.SetTexCoord0(0.80f, 0.80f, 0.80f, 1.f);
    host_.SetTexCoord2(0.50f, 0.50f, 0.50f, 1.f);
    host_.SetTexCoord3(0.f, 0.f, tex3_b, 1.f);
    host_.SetScreenVertex(left, top);
    host_.SetScreenVertex(right, top);
    host_.SetScreenVertex(right, bottom);
    host_.SetScreenVertex(left, bottom);
    host_.End();
  };

  static constexpr auto kLeftCol = 340.f;
  static constexpr auto kRightCol = 410.f;

  // Case 1: Register source with negative value (-0.30)
  // Stage 0: R1 = -0.30
  // Stage 1: R0 = 0.75 + R1
  // Signed mapping: R0 = 0.75 + (-0.30) = 0.45.
  // Unsigned mapping: max(0, -0.30) = 0.0 -> R0 = 0.75 + 0.0 = 0.75.
  host_.SetCombinerControl(2);
  host_.SetCombinerFactorC0(0, 0.30f, 0.30f, 0.30f, 0.30f);
  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_C0, TestHost::MAP_SIGNED_NEGATE),
                              TestHost::OneInput());
  host_.SetOutputColorCombiner(0, TestHost::DST_R1);

  host_.SetCombinerFactorC1(1, 0.75f, 0.75f, 0.75f, 0.75f);
  host_.SetOutputColorCombiner(1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY);
  host_.SetFinalCombiner0Just(TestHost::SRC_R0);

  // Signed
  host_.SetInputColorCombiner(1, TestHost::ColorInput(TestHost::SRC_C1), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_R1, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::OneInput());
  draw_quad(kLeftCol, 98.f);

  // Unsigned
  host_.SetInputColorCombiner(1, TestHost::ColorInput(TestHost::SRC_C1), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_R1, TestHost::MAP_UNSIGNED_IDENTITY),
                              TestHost::OneInput());
  draw_quad(kRightCol, 98.f);

  // Case 2: Register source with positive value (+0.20)
  // Stage 0: R1 = +0.20
  // Stage 1: R0 = 0.75 + R1 = 0.95 for both signed and unsigned.
  host_.SetCombinerFactorC0(0, 0.20f, 0.20f, 0.20f, 0.20f);
  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_C0, TestHost::MAP_UNSIGNED_IDENTITY),
                              TestHost::OneInput());
  host_.SetOutputColorCombiner(0, TestHost::DST_R1);

  // Signed
  host_.SetInputColorCombiner(1, TestHost::ColorInput(TestHost::SRC_C1), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_R1, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::OneInput());
  draw_quad(kLeftCol, 148.f);

  // Unsigned
  host_.SetInputColorCombiner(1, TestHost::ColorInput(TestHost::SRC_C1), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_R1, TestHost::MAP_UNSIGNED_IDENTITY),
                              TestHost::OneInput());
  draw_quad(kRightCol, 148.f);

  // Case 3 & 4: Pass-through texture coordinates
  host_.SetTextureStageEnabled(0, true);
  host_.SetupTextureStages();
  host_.SetShaderStageProgram(TestHost::STAGE_PASS_THROUGH);
  host_.SetCombinerControl(1);
  host_.SetCombinerFactorC1(0, 0.75f, 0.75f, 0.75f, 0.75f);
  host_.SetOutputColorCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY);

  // Case 3: Tex negative (-0.30)
  // Signed
  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_C1), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_TEX0, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::OneInput());
  draw_tex_quad(kLeftCol, 198.f, -0.30f);

  // Unsigned
  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_C1), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_TEX0, TestHost::MAP_UNSIGNED_IDENTITY),
                              TestHost::OneInput());
  draw_tex_quad(kRightCol, 198.f, -0.30f);

  // Case 4: Tex positive (+0.20)
  // Signed
  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_C1), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_TEX0, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::OneInput());
  draw_tex_quad(kLeftCol, 248.f, 0.20f);

  // Unsigned
  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_C1), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_TEX0, TestHost::MAP_UNSIGNED_IDENTITY),
                              TestHost::OneInput());
  draw_tex_quad(kRightCol, 248.f, 0.20f);

  // Case 5: 5-Stage Pipeline with Tex3.b = -0.25
  // When Stage 1 maps Tex3 with MAP_SIGNED_IDENTITY: R0 = 0.40 (subtraction).
  // When Stage 1 maps Tex3 with MAP_UNSIGNED_IDENTITY: max(0, -0.25) = 0.0 -> R0 = 0.80 (no subtraction).
  host_.SetTextureStageEnabled(2, true);
  host_.SetTextureStageEnabled(3, true);
  host_.SetupTextureStages();
  host_.SetShaderStageProgram(TestHost::STAGE_PASS_THROUGH, TestHost::STAGE_NONE, TestHost::STAGE_PASS_THROUGH,
                              TestHost::STAGE_PASS_THROUGH);
  host_.SetCombinerControl(5);

  // Stage 0:
  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_TEX0, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::ColorInput(TestHost::SRC_DIFFUSE, TestHost::MAP_SIGNED_IDENTITY));
  host_.SetOutputColorCombiner(0, TestHost::DST_R0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY);

  // Stage 2:
  host_.SetInputColorCombiner(2, TestHost::ColorInput(TestHost::SRC_TEX2, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::AlphaInput(TestHost::SRC_R1, TestHost::MAP_SIGNED_IDENTITY));
  host_.SetOutputColorCombiner(2, TestHost::DST_R1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY);

  // Stage 3:
  host_.SetInputColorCombiner(3, TestHost::ColorInput(TestHost::SRC_R1, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::ColorInput(TestHost::SRC_R0, TestHost::MAP_SIGNED_IDENTITY));
  host_.SetOutputColorCombiner(3, TestHost::DST_R1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_SHIFT_LEFT_2);

  // Stage 4:
  host_.SetInputColorCombiner(
      4, TestHost::ColorInput(TestHost::SRC_R0, TestHost::MAP_SIGNED_IDENTITY), TestHost::OneInput(),
      TestHost::ColorInput(TestHost::SRC_R1, TestHost::MAP_SIGNED_IDENTITY), TestHost::OneInput());
  host_.SetOutputColorCombiner(4, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY);

  // Final combiner:
  host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_FOG, false, false, TestHost::SRC_R0, false,
                          false, TestHost::SRC_ZERO, false, false);

  // Pipeline Signed Tex3 mapping
  host_.SetInputColorCombiner(1, TestHost::ColorInput(TestHost::SRC_TEX3, TestHost::MAP_SIGNED_IDENTITY),
                              TestHost::OneInput());
  host_.SetOutputColorCombiner(1, TestHost::DST_R1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY, /*alpha_from_ab_blue=*/true, false);
  draw_pipe_quad(kLeftCol, 298.f, -0.25f);

  // Pipeline Unsigned Tex3 mapping
  host_.SetInputColorCombiner(1, TestHost::ColorInput(TestHost::SRC_TEX3, TestHost::MAP_UNSIGNED_IDENTITY),
                              TestHost::OneInput());
  host_.SetOutputColorCombiner(1, TestHost::DST_R1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, false, false,
                               TestHost::SM_SUM, TestHost::OP_IDENTITY, /*alpha_from_ab_blue=*/true, false);
  draw_pipe_quad(kRightCol, 298.f, -0.25f);

  pb_printat(0, 0, "%s", kSignedToUnsignedMappingTestName);
  pb_printat(1, 0, "Comparing SIGNED_IDENTITY (left) vs UNSIGNED_IDENTITY (right)");
  pb_printat(2, 34, "SIGNED  UNSIGNED");
  pb_printat(3, 0, "Reg -0.30 (0.45 vs 0.75)");
  pb_printat(5, 0, "Reg +0.20 (0.95 vs 0.95)");
  pb_printat(7, 0, "Tex -0.30 (0.45 vs 0.75)");
  pb_printat(9, 0, "Tex +0.20 (0.95 vs 0.95)");
  pb_printat(11, 0, "Pipe Tex3 -0.25 (0.40 vs 0.80)");
  pb_printat(13, 0, "UNSIGNED_IDENTITY clamps negative values to 0.0");
  pb_printat(14, 0, "Right quads should show unclamped base (0.75 or 0.80)");
  pb_draw_text_screen();

  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(2, false);
  host_.SetTextureStageEnabled(3, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  host_.SetCombinerControl();
  FinishDraw(kSignedToUnsignedMappingTestName);
}
