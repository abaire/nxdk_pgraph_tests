#include "vertex_shader_edge_case_tests.h"

#include <shaders/vertex_shader_program.h>

#include <cmath>

#include "test_host.h"

static const uint32_t kDp4TestVsh[] = {
#include "dp4_test.vshinc"

};

static constexpr char kDP4WithXComponentInputsTests[] = "DP4WithXComponentIn";

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc DP4WithXComponentIn
 *   Renders a pair of quads using a shader that runs dp4 on the weight, diffuse, and specular inputs. Weight has all 4
 *   components initialized, diffuse has 3, and specular has 2. The contents of the second vector for each attribute is
 *   printed on screen, as are the results of a dot product operation on the CPU, assuming missing W param is always set
 *   to 1.0 and any others are set to 0.0.
 */
VertexShaderEdgeCaseTests::VertexShaderEdgeCaseTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Vertex shader edge case", config) {
  tests_[kDP4WithXComponentInputsTests] = [this]() { TestDP4WithThreeComponentInputs(); };
}

void VertexShaderEdgeCaseTests::Initialize() { TestSuite::Initialize(); }

void VertexShaderEdgeCaseTests::TestDP4WithThreeComponentInputs() {
  static constexpr float kWeightVector[] = {0.9, 0.5, 0.25, 0.1};
  static constexpr float kDiffVector[] = {0.5, 0.25, 0.1, 0.4};
  static constexpr float kSpecVector[] = {0.4, 0.2, 0.2, 0.2};

  static constexpr float kWeights[4][4] = {
      {1.f, 0.f, 0.f, 0.f}, {0.f, 1.f, 0.f, 0.f}, {0.f, 0.f, 1.f, 0.f}, {0.f, 0.f, 0.f, 1.f}};

  static constexpr float kDiffuse[4][3] = {
      {0.9f, 0.04f, 0.06f},
      {0.f, 1.f, 0.f},
      {0.f, 0.f, 1.f},
      {0.f, 0.f, 0.f},
  };

  static constexpr float kSpecular[4][2] = {
      {1.f, 0.f},
      {0.f, 1.f},
      {0.8f, 0.4f},
      {0.1f, 0.2f},
  };

  static constexpr float kOneVector[] = {1.0, 1.0, 1.0, 1.0};

  {
    auto shader = std::make_shared<VertexShaderProgram>();
    shader->SetShader(kDp4TestVsh, sizeof(kDp4TestVsh));

    // Keep constants aligned with dp4_test.vsh
    // #weight_vector
    shader->SetUniformF(96 - VertexShaderProgram::kShaderUserConstantOffset, kWeightVector[0], kWeightVector[1],
                        kWeightVector[2], kWeightVector[3]);
    // #diffuse_vector
    shader->SetUniformF(97 - VertexShaderProgram::kShaderUserConstantOffset, kDiffVector[0], kDiffVector[1],
                        kDiffVector[2], kDiffVector[3]);
    // #specular_vector
    shader->SetUniformF(98 - VertexShaderProgram::kShaderUserConstantOffset, kSpecVector[0], kSpecVector[1],
                        kSpecVector[2], kSpecVector[3]);
    // #one_constant
    shader->SetUniformF(99 - VertexShaderProgram::kShaderUserConstantOffset, kOneVector[0], kOneVector[1],
                        kOneVector[2], kOneVector[3]);

    host_.SetVertexShaderProgram(shader);
  }

  host_.PrepareDraw(0xFF252525);

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  static constexpr float kQuadSize = 256.f;
  std::shared_ptr<VertexBuffer> quad_buffer = host_.AllocateVertexBuffer(4);
  quad_buffer->SetWeightCount(4);
  quad_buffer->SetDiffuseCount(3);
  quad_buffer->SetSpecularCount(2);

  const float kMargin = floorf((host_.GetFramebufferWidthF() - (2.f * kQuadSize)) / 3.f);
  const float kLeft1 = kMargin;
  const float kLeft2 = kMargin + kQuadSize + kMargin;
  const float top = host_.GetFramebufferHeightF() - kQuadSize - 4.f;

  pb_print("%s\n", kDP4WithXComponentInputsTests);
  pb_print(" Weight (4): %.2f, %.2f, %.2f, %.2f\n", kWeightVector[0], kWeightVector[1], kWeightVector[2],
           kWeightVector[3]);
  pb_print(" Diff   (3): %.2f, %.2f, %.2f, %.2f\n", kDiffVector[0], kDiffVector[1], kDiffVector[2], kDiffVector[3]);
  pb_print(" Spec   (2): %.2f, %.2f, %.2f, %.2f\n", kSpecVector[0], kSpecVector[1], kSpecVector[2], kSpecVector[3]);

  static constexpr float kDefaultValLeft = 0.25f;
  static constexpr float kDefaultValRight = 0.85f;
  pb_print("\n");

  auto draw_quad = [&](float left, float default_val) {
    host_.SetWeight(default_val, default_val, default_val, default_val);
    host_.SetDiffuse(default_val, default_val, default_val, default_val);
    host_.SetSpecular(default_val, default_val, default_val, default_val);

    auto vertex = quad_buffer->Lock();

    vertex->SetPosition(left, top, 0.f);
    vertex->SetWeight(kWeights[0], 4);
    vertex->SetDiffuse(kDiffuse[0], 3);
    vertex->SetSpecular(kSpecular[0], 2);

    ++vertex;
    vertex->SetPosition(left + kQuadSize, top, 0.f);
    vertex->SetWeight(kWeights[1], 4);
    vertex->SetDiffuse(kDiffuse[1], 3);
    vertex->SetSpecular(kSpecular[1], 2);

    ++vertex;
    vertex->SetPosition(left + kQuadSize, top + kQuadSize, 0.f);
    vertex->SetWeight(kWeights[2], 4);
    vertex->SetDiffuse(kDiffuse[2], 3);
    vertex->SetSpecular(kSpecular[2], 2);

    ++vertex;
    vertex->SetPosition(left, top + kQuadSize, 0.f);
    vertex->SetWeight(kWeights[3], 4);
    vertex->SetDiffuse(kDiffuse[3], 3);
    vertex->SetSpecular(kSpecular[3], 2);

    quad_buffer->Unlock();

    host_.SetVertexBuffer(quad_buffer);
    host_.DrawInlineArray(TestHost::POSITION | TestHost::WEIGHT | TestHost::DIFFUSE | TestHost::SPECULAR,
                          TestHost::PRIMITIVE_QUADS);
  };

  draw_quad(kLeft1, kDefaultValLeft);
  draw_quad(kLeft2, kDefaultValRight);

  auto dot = [](const float *vals, int num_components, const float *vec_b) {
    float ret = vals[0] * vec_b[0];

    for (auto i = 1; i < num_components; ++i) {
      ret += vals[i] * vec_b[i];
    }

    if (num_components < 4) {
      ret += vec_b[3];
    }

    return ret;
  };

  pb_print("%.2f, %.2f, %.2f               %.2f, %.2f, %.2f\n", dot(kWeights[0], 4, kWeightVector),
           dot(kDiffuse[0], 3, kDiffVector), dot(kSpecular[0], 2, kSpecVector), dot(kWeights[1], 4, kWeightVector),
           dot(kDiffuse[1], 3, kDiffVector), dot(kSpecular[1], 2, kSpecVector));
  pb_print("%.2f, %.2f, %.2f               %.2f, %.2f, %.2f\n", dot(kWeights[3], 4, kWeightVector),
           dot(kDiffuse[3], 3, kDiffVector), dot(kSpecular[3], 2, kSpecVector), dot(kWeights[2], 4, kWeightVector),
           dot(kDiffuse[2], 3, kDiffVector), dot(kSpecular[2], 2, kSpecVector));
  pb_print("Quad Defaults: %.2f                %.2f\n", kDefaultValLeft, kDefaultValRight);

  pb_draw_text_screen();

  FinishDraw(kDP4WithXComponentInputsTests);

  host_.SetVertexShaderProgram(nullptr);
}
