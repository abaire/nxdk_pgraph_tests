#include "vertex_shader_independence_tests.h"

#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"

static constexpr char kMACILUTest[] = "MAC_ILU_Independence";
static constexpr char kMultioutputTest[] = "Multioutput";

// It is very difficult to craft an appropriate test case using Cg, so this shader has been written manually. The key
// lines are the dp3+rsq pairing which modifies and utilizes r0.x in one statement. The resulting value is assigned to
// the diffuse output for rendering via the pixel combiners.
// The expected behavior is that the diffuse blue component is entirely determined by the C0 uniform with the rest of
// the components determined by C1.

// clang-format off
static const uint32_t kShader[] = {
    // mov oPos, v0
    0x00000000, 0x0020001b, 0x0836106c, 0x2070f800,

    // mov oD0, v3
    0x00000000, 0x0020061b, 0x0836106c, 0x2070f818,

    //  mov r0, c0
    0x00000000, 0x002c001b, 0x0c36106c, 0x2f000ff8,

    // mov r1, c1
    0x00000000, 0x002c201b, 0x0c36106c, 0x2f100ff8,

    // mov r8, c2
    0x00000000, 0x002c401b, 0x0c36106c, 0x2f800ff8,

    // dp3 r0.x, r8.wyz, r8.wyz
    // rsq r1.z, r0.x
    0x00000000, 0x08A000DA, 0x85B50800, 0x18020000,

    // Stall a bunch of times
    // mov oPos, v0
    0x00000000, 0x0020001b, 0x0836106c, 0x2070f800,
    // mov oD0, v3
    0x00000000, 0x0020061b, 0x0836106c, 0x2070f818,
    // mov r8, c2
    0x00000000, 0x002c401b, 0x0c36106c, 0x2f800ff8,

    // mov oD0, r1
    0x00000000, 0x0020001b, 0x1436106c, 0x2070f819};
// clang-format on

// clang-format off
static const uint32_t kMacIndependenceShader[] = {
#include "vertex_shader_mac_independence_tests.vshinc"


};
// clang-format on

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc MAC_ILU_Independence
 *   Verifies that instructions running on the MAC and ILU in parallel that write to an input of the other execute using
 *   the original value of the input rather than the result of the operation.
 *
 * @tc Multioutput
 *   Verifies that MAC & ILU instructions that write to both oPos and a temporary register using the R12 input alias
 *   use the original value of oPos when generating the output.
 */
VertexShaderIndependenceTests::VertexShaderIndependenceTests(TestHost& host, std::string output_dir,
                                                             const Config& config)
    : TestSuite(host, std::move(output_dir), "Vertex shader independence tests", config) {
  tests_[kMACILUTest] = [this]() { TestMACILUIndependence(); };
  tests_[kMultioutputTest] = [this] { TestMultiOutput(); };
}

void VertexShaderIndependenceTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
}

void VertexShaderIndependenceTests::CreateGeometry() {
  const auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  const auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  const float left = floorf(fb_width / 5.0f);
  const float right = left + (fb_width - left * 2.0f);
  const float top = floorf(fb_height / 8.0f);
  const float bottom = top + (fb_height - top * 2.0f);

  auto buffer = host_.AllocateVertexBuffer(6);
  buffer->DefineBiTri(0, left, top, right, bottom);
}

void VertexShaderIndependenceTests::TestMACILUIndependence() {
  host_.PrepareDraw(0xFE333333);

  auto shader = std::make_shared<PassthroughVertexShader>();
  shader->SetShader(kShader, sizeof(kShader));
  // Only the X component is actually used. The expected blue channel should be the reciprocal square root of this
  // constant's X value.
  shader->SetUniformF(0, 1.0f, 0.0f, 0.0f, 0.0f);
  // The output R, G, and A are set from this constant.
  shader->SetUniformF(1, 0.5f, 0.5, 0.0f, 1.0f);
  // This should have no effect. In an erroneously coupled MAC/ILU case, the dot product of this value with itself will
  // be used in the reciprocal square root instead of c0.x.
  // For example, with a value of 8, the erroneous output will be RGBA: 1.0, 0.0, 0.125, 1.0
  shader->SetUniformF(2, 0.0f, 0.0f, 0.0f, 8.0f);
  host_.SetVertexShaderProgram(shader);

  host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE | TestHost::SPECULAR);

  pb_print("Expect a light blue square");
  pb_draw_text_screen();

  host_.SetVertexShaderProgram(nullptr);
  FinishDraw(kMACILUTest);
}

void VertexShaderIndependenceTests::TestMultiOutput() {
  host_.PrepareDraw(0xFE333333);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_SPECULAR_ENABLE, true);
  Pushbuffer::End();

  static constexpr auto kQuadSize = 256.f;
  static constexpr auto kHalfSize = kQuadSize * 0.5f;

  const auto kLeft = host_.CenterX(kQuadSize);
  const auto kTop = host_.CenterY(kQuadSize) + 16.f;
  static constexpr auto kZ = 1.f;

  static constexpr auto kDiffuseRed = 0.5f;
  static constexpr auto kSpecularGreen = 0.5f;

  // MAC
  {
    // Draw the control on the left. Expected output is the combination of diffuse red and specular green.
    {
      auto shader = std::make_shared<PassthroughVertexShader>();
      host_.SetVertexShaderProgram(shader);

      host_.Begin(TestHost::PRIMITIVE_QUADS);

      host_.SetDiffuse(kDiffuseRed, kSpecularGreen, 0.0, 1.0);

      host_.SetVertex(kLeft, kTop, kZ);
      host_.SetVertex(kLeft + kHalfSize, kTop, kZ);
      host_.SetVertex(kLeft + kHalfSize, kTop + kQuadSize, kZ);
      host_.SetVertex(kLeft, kTop + kQuadSize, kZ);
      host_.End();
    }

    // Draw using the multi-output shader.
    {
      auto shader = std::make_shared<VertexShaderProgram>();
      shader->SetShader(kMacIndependenceShader, sizeof(kMacIndependenceShader));
      host_.SetVertexShaderProgram(shader);

      host_.SetDiffuse(kDiffuseRed, 0.0, 0.0, 1.0);
      host_.SetSpecular(0.0, kSpecularGreen, 0.0, 1.0);

      // The fail case will double the specular contribution.
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetVertex(kLeft + kHalfSize, kTop, kZ);
      host_.SetVertex(kLeft + kQuadSize, kTop, kZ);
      host_.SetVertex(kLeft + kQuadSize, kTop + kHalfSize, kZ);
      host_.SetVertex(kLeft + kHalfSize, kTop + kHalfSize, kZ);
      host_.End();
    }
  }

  // ILU
  {
    // Draw the control on the left. Expected output is the red component of diffuse for all color channels.
    {
      auto shader = std::make_shared<PassthroughVertexShader>();
      host_.SetVertexShaderProgram(shader);

      host_.Begin(TestHost::PRIMITIVE_QUADS);

      host_.SetDiffuse(kDiffuseRed, kDiffuseRed, kDiffuseRed, 1.0);

      host_.SetVertex(kLeft, kTop + kHalfSize, kZ);
      host_.SetVertex(kLeft + kHalfSize, kTop + kHalfSize, kZ);
      host_.SetVertex(kLeft + kHalfSize, kTop + kQuadSize, kZ);
      host_.SetVertex(kLeft, kTop + kQuadSize, kZ);
      host_.End();
    }
    // Draw using the multi-output shader.
    {
      auto shader = std::make_shared<VertexShaderProgram>();
      shader->SetShader(kMacIndependenceShader, sizeof(kMacIndependenceShader));
      host_.SetVertexShaderProgram(shader);

      host_.SetDiffuse(kDiffuseRed, 0.0, 0.0, 1.0);
      host_.SetFinalCombiner0Just(TestHost::SRC_SPECULAR);

      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetVertex(kLeft + kHalfSize, kTop + kHalfSize, kZ);
      host_.SetVertex(kLeft + kQuadSize, kTop + kHalfSize, kZ);
      host_.SetVertex(kLeft + kQuadSize, kTop + kQuadSize, kZ);
      host_.SetVertex(kLeft + kHalfSize, kTop + kQuadSize, kZ);
      host_.End();

      host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    }
  }

  pb_print("%s\n", kMultioutputTest);
  pb_print("Expect two uniform colored rows\n");
  pb_print("Left side is control\n");
  pb_printat(6, 44, "MAC");
  pb_printat(11, 44, "ILU");
  pb_draw_text_screen();

  host_.SetVertexShaderProgram(nullptr);
  FinishDraw(kMultioutputTest);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_SPECULAR_ENABLE, false);
  Pushbuffer::End();
}
