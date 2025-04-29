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
 */
CombinerTests::CombinerTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Combiner", config) {
  tests_[kMuxTestName] = [this]() { TestMux(); };
  tests_[kIndependenceTestName] = [this]() { TestCombinerIndependence(); };
  tests_[kColorAlphaIndependenceTestName] = [this]() { TestCombinerColorAlphaIndependence(); };
  tests_[kFlagsTestName] = [this]() { TestFlags(); };
  tests_[kUnboundTextureSamplerTestName] = [this]() { TestUnboundTextureSamplers(); };
  tests_[kAlphaFromBlueTestName] = [this]() { TestAlphaFromBlue(); };
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

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kMuxTestName);
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

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kIndependenceTestName);
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

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kColorAlphaIndependenceTestName);
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

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kFlagsTestName);
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

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kUnboundTextureSamplerTestName);

  host_.SetVertexShaderProgram(nullptr);
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

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kAlphaFromBlueTestName);

  host_.SetVertexShaderProgram(nullptr);
}
