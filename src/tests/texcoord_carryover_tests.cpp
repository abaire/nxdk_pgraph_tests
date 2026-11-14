#include "texcoord_carryover_tests.h"

#include <pbkit/pbkit.h>
#include <shaders/vertex_shader_program.h>

#include "debug_output.h"
#include "test_host.h"
#include "vertex_buffer.h"

// clang-format off
static constexpr uint32_t kPrimeConstShader[] = {
#include "texcoord_carryover_prime_t0_t3_const.vshinc"
};

static constexpr uint32_t kPrimeAttrShader[] = {
#include "texcoord_carryover_prime_t0_t3_attr.vshinc"
};

// Writes position and colors only, leaving all texture coordinate outputs unwritten.
static constexpr uint32_t kPositionAndColorShader[] = {
#include "passthrough_just_position_and_color.vshinc"
};
// clang-format on

enum class DrawMode { DRAW_ARRAYS, DRAW_INLINE_BUFFERS, DRAW_INLINE_ELEMENTS, DRAW_INLINE_ARRAYS };

struct LabeledDrawMode {
  DrawMode mode;
  const char *name;
};

static constexpr LabeledDrawMode kDrawModes[] = {
    {DrawMode::DRAW_INLINE_BUFFERS, "IB"},
    {DrawMode::DRAW_INLINE_ELEMENTS, "IE"},
    {DrawMode::DRAW_INLINE_ARRAYS, "IA"},
    {DrawMode::DRAW_ARRAYS, "DA"},
};

static constexpr uint32_t kBasicAttributes = TestHost::POSITION | TestHost::DIFFUSE;
static constexpr uint32_t kTexcoordAttributes = kBasicAttributes | TestHost::TEXCOORD0 | TestHost::TEXCOORD3;

static constexpr float kPrimeValue[] = {0.25f, 0.5f, 0.75f, 1.0f};
static constexpr float kOtherPrimeValue[] = {1.0f, 0.25f, 0.0f, 1.0f};

static constexpr float kQuadSize = 64.f;
static constexpr float kRowSpacing = 100.f;
static constexpr float kFirstRowTop = 80.f;
// Text rows are 25 pixels apart, starting at Y = 25 (row 0).
static constexpr int kFirstRowTextRow = 2;
static constexpr int kTextRowsPerRow = 4;

static constexpr uint32_t kNumPoints = 12;
static constexpr float kPointSize = 16.f;
static constexpr float kPointsLeft = 100.f;
static constexpr float kPointsSpacing = 40.f;

static constexpr float kPointColors[kNumPoints][4] = {
    {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.0f, 0.0f, 1.0f},
    {0.0f, 0.5f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.5f, 1.0f}, {1.0f, 0.5f, 0.0f, 1.0f}, {0.5f, 0.5f, 0.5f, 1.0f},
};

/**
 * @tc Stage3Constant
 *   For each draw mode, the left quad is drawn twice (so that every vertex output buffer is covered) with a shader that
 *   writes oT3 = (0.25, 0.5, 0.75, 1.0) for every vertex. The right quad is then drawn with a shader that does not
 *   write oT3. Texture stage 3 is in PASS_THROUGH mode and the combiner outputs TEX3, so each quad shows its texture
 *   coordinate as a color. The right quad matches the left if oT3 carries over from the previous draw.
 *
 * @tc Stage3Reprime
 *   For each draw mode, the left quad is drawn twice with oT3 = (1.0, 0.25, 0.0, 1.0), the middle quad twice with
 *   oT3 = (0.25, 0.5, 0.75, 1.0), then the right quad is drawn with a shader that does not write oT3. Each quad shows
 *   its texture coordinate as a color. The right quad matches the middle quad if oT3 carries over from the most recent
 *   draw.
 *
 * @tc Stage3PerVertex
 *   For each draw mode, draws 12 points on the top row with oT3 taken from a per-vertex attribute (12 distinct colors),
 *   then 12 points in the same columns on the bottom row with a shader that does not write oT3. Each point shows its
 *   texture coordinate as a color, so the color of a bottom point identifies the primed vertex it inherited from, if
 *   any (e.g., the six vertex round robin pattern observed for fog in FogCarryoverTests).
 *
 * @tc Stage0Constant
 *   As Stage3Constant, for oT0 using texture stage 0.
 *
 * @tc Stage0Reprime
 *   As Stage3Reprime, for oT0 using texture stage 0.
 *
 * @tc Stage0PerVertex
 *   As Stage3PerVertex, for oT0 using texture stage 0.
 */
TexcoordCarryoverTests::TexcoordCarryoverTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texcoord carryover", config) {
  for (uint32_t stage : {3u, 0u}) {
    const std::string prefix = "Stage" + std::to_string(stage);
    std::string name = prefix + "Constant";
    tests_[name] = [this, name, stage]() { TestConstant(name, stage); };
    name = prefix + "Reprime";
    tests_[name] = [this, name, stage]() { TestReprime(name, stage); };
    name = prefix + "PerVertex";
    tests_[name] = [this, name, stage]() { TestPerVertex(name, stage); };
  }
}

void TexcoordCarryoverTests::Initialize() {
  TestSuite::Initialize();

  prime_const_shader_ =
      std::make_shared<PBKitPlusPlus::VertexShaderProgram>(kPrimeConstShader, sizeof(kPrimeConstShader));
  prime_attr_shader_ = std::make_shared<PBKitPlusPlus::VertexShaderProgram>(kPrimeAttrShader, sizeof(kPrimeAttrShader));
  position_and_color_shader_ =
      std::make_shared<PBKitPlusPlus::VertexShaderProgram>(kPositionAndColorShader, sizeof(kPositionAndColorShader));
}

void TexcoordCarryoverTests::TearDownTest() {
  TestSuite::TearDownTest();

  host_.DisableTextureStages();
  host_.SetBlend(true);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_POINT_SIZE, 8);
  Pushbuffer::End();
}

void TexcoordCarryoverTests::ActivateShader(const std::shared_ptr<PBKitPlusPlus::VertexShaderProgram> &shader) {
  host_.SetVertexShaderProgram(shader);
  shader->PrepareDraw();
  shader->Activate();
}

//! Configures the given stage to output its texture coordinate directly as the final color.
void TexcoordCarryoverTests::ConfigureStageOutput(uint32_t stage) {
  ASSERT(stage == 0 || stage == 3);

  for (uint32_t i = 0; i < 4; ++i) {
    host_.SetTextureStageEnabled(i, i == stage);
  }

  if (stage == 0) {
    host_.SetShaderStageProgram(TestHost::STAGE_PASS_THROUGH);
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
  } else {
    host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE,
                                TestHost::STAGE_PASS_THROUGH);
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX3);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);
  }

  // Ensure that the alpha of the texture coordinate does not affect the output color.
  host_.SetBlend(false);
}

static void DrawBuffer(TestHost &host, DrawMode draw_mode, const std::shared_ptr<VertexBuffer> &buffer,
                       const std::vector<uint32_t> &indices, uint32_t attributes, TestHost::DrawPrimitive primitive) {
  host.SetVertexBuffer(buffer);
  switch (draw_mode) {
    case DrawMode::DRAW_ARRAYS:
      host.DrawArrays(attributes, primitive);
      break;
    case DrawMode::DRAW_INLINE_BUFFERS:
      host.DrawInlineBuffer(attributes, primitive);
      break;
    case DrawMode::DRAW_INLINE_ELEMENTS:
      host.DrawInlineElements16(indices, attributes, primitive);
      break;
    case DrawMode::DRAW_INLINE_ARRAYS:
      host.DrawInlineArray(attributes, primitive);
      break;
  }
}

static std::shared_ptr<VertexBuffer> CreateQuad(TestHost &host) {
  auto buffer = host.AllocateVertexBuffer(4);
  auto vertex = buffer->Lock();
  for (auto i = 0; i < 4; ++i, ++vertex) {
    vertex->SetDiffuse(1.f, 1.f, 1.f, 1.f);
  }
  buffer->Unlock();
  return buffer;
}

static void PositionQuad(const std::shared_ptr<VertexBuffer> &buffer, float left, float top, float width,
                         float height) {
  const float xs[] = {left, left + width, left + width, left};
  const float ys[] = {top, top, top + height, top + height};
  auto vertex = buffer->Lock();
  for (auto i = 0; i < 4; ++i, ++vertex) {
    vertex->SetPosition(xs[i], ys[i], 0.f);
  }
  buffer->Unlock();
}

//! Returns the column at which text will start at the given X coordinate (columns start at X = 20 + 10 * col).
static int TextColumn(float x) { return static_cast<int>((x - 20.f) / 10.f); }

void TexcoordCarryoverTests::TestConstant(const std::string &name, uint32_t stage) {
  ConfigureStageOutput(stage);

  host_.SetVertexShaderProgram(prime_const_shader_);
  host_.PrepareDraw(0xFF202020);

  const float left_x = floorf(host_.GetFramebufferWidthF() * 0.25f - kQuadSize * 0.5f);
  const float right_x = floorf(host_.GetFramebufferWidthF() * 0.75f - kQuadSize * 0.5f);

  auto prime_buffer = CreateQuad(host_);
  auto test_buffer = CreateQuad(host_);
  const std::vector<uint32_t> indices{0, 1, 2, 3};

  prime_const_shader_->SetUniform4F(0, kPrimeValue);

  int text_row = kFirstRowTextRow;
  float top = kFirstRowTop;
  for (auto &mode : kDrawModes) {
    PositionQuad(prime_buffer, left_x, top, kQuadSize, kQuadSize);
    PositionQuad(test_buffer, right_x, top, kQuadSize, kQuadSize);

    // Prime twice to ensure that every vertex output slot has been written.
    ActivateShader(prime_const_shader_);
    DrawBuffer(host_, mode.mode, prime_buffer, indices, kBasicAttributes, TestHost::PRIMITIVE_QUADS);
    DrawBuffer(host_, mode.mode, prime_buffer, indices, kBasicAttributes, TestHost::PRIMITIVE_QUADS);

    ActivateShader(position_and_color_shader_);
    DrawBuffer(host_, mode.mode, test_buffer, indices, kBasicAttributes, TestHost::PRIMITIVE_QUADS);

    pb_printat(text_row, 1, "%s", mode.name);
    host_.PBKitBusyWait();

    text_row += kTextRowsPerRow;
    top += kRowSpacing;
  }

  pb_printat(0, 0, "%s", name.c_str());
  pb_printat(1, TextColumn(left_x), "primed");
  pb_printat(1, TextColumn(right_x), "oT%u unwritten", stage);
  pb_draw_text_screen();

  FinishDraw(name);
}

void TexcoordCarryoverTests::TestReprime(const std::string &name, uint32_t stage) {
  ConfigureStageOutput(stage);

  host_.SetVertexShaderProgram(prime_const_shader_);
  host_.PrepareDraw(0xFF202020);

  const float fb_width = host_.GetFramebufferWidthF();
  const float xs[] = {
      floorf(fb_width * (1.f / 6.f) - kQuadSize * 0.5f),
      floorf(fb_width * 0.5f - kQuadSize * 0.5f),
      floorf(fb_width * (5.f / 6.f) - kQuadSize * 0.5f),
  };

  auto first_buffer = CreateQuad(host_);
  auto second_buffer = CreateQuad(host_);
  auto test_buffer = CreateQuad(host_);
  const std::vector<uint32_t> indices{0, 1, 2, 3};

  int text_row = kFirstRowTextRow;
  float top = kFirstRowTop;
  for (auto &mode : kDrawModes) {
    PositionQuad(first_buffer, xs[0], top, kQuadSize, kQuadSize);
    PositionQuad(second_buffer, xs[1], top, kQuadSize, kQuadSize);
    PositionQuad(test_buffer, xs[2], top, kQuadSize, kQuadSize);

    prime_const_shader_->SetUniform4F(0, kOtherPrimeValue);
    ActivateShader(prime_const_shader_);
    DrawBuffer(host_, mode.mode, first_buffer, indices, kBasicAttributes, TestHost::PRIMITIVE_QUADS);
    DrawBuffer(host_, mode.mode, first_buffer, indices, kBasicAttributes, TestHost::PRIMITIVE_QUADS);

    prime_const_shader_->SetUniform4F(0, kPrimeValue);
    ActivateShader(prime_const_shader_);
    DrawBuffer(host_, mode.mode, second_buffer, indices, kBasicAttributes, TestHost::PRIMITIVE_QUADS);
    DrawBuffer(host_, mode.mode, second_buffer, indices, kBasicAttributes, TestHost::PRIMITIVE_QUADS);

    ActivateShader(position_and_color_shader_);
    DrawBuffer(host_, mode.mode, test_buffer, indices, kBasicAttributes, TestHost::PRIMITIVE_QUADS);

    pb_printat(text_row, 1, "%s", mode.name);
    host_.PBKitBusyWait();

    text_row += kTextRowsPerRow;
    top += kRowSpacing;
  }

  pb_printat(0, 0, "%s", name.c_str());
  pb_printat(1, TextColumn(xs[0]), "prime 1");
  pb_printat(1, TextColumn(xs[1]), "prime 2");
  pb_printat(1, TextColumn(xs[2]), "oT%u unwritten", stage);
  pb_draw_text_screen();

  FinishDraw(name);
}

void TexcoordCarryoverTests::TestPerVertex(const std::string &name, uint32_t stage) {
  ConfigureStageOutput(stage);

  host_.SetVertexShaderProgram(prime_attr_shader_);
  host_.PrepareDraw(0xFF202020);

  Pushbuffer::Begin();
  // Point size is in 1/8th pixel units.
  Pushbuffer::Push(NV097_SET_POINT_SIZE, static_cast<uint32_t>(kPointSize * 8.f));
  Pushbuffer::End();

  auto prime_buffer = host_.AllocateVertexBuffer(kNumPoints);
  prime_buffer->SetTexCoord0Count(4);
  prime_buffer->SetTexCoord3Count(4);
  {
    auto vertex = prime_buffer->Lock();
    for (auto i = 0u; i < kNumPoints; ++i, ++vertex) {
      vertex->SetDiffuse(1.f, 1.f, 1.f, 1.f);
      vertex->SetTexCoord0(kPointColors[i][0], kPointColors[i][1], kPointColors[i][2], kPointColors[i][3]);
      vertex->SetTexCoord3(kPointColors[i][0], kPointColors[i][1], kPointColors[i][2], kPointColors[i][3]);
    }
    prime_buffer->Unlock();
  }

  auto test_buffer = host_.AllocateVertexBuffer(kNumPoints);
  {
    auto vertex = test_buffer->Lock();
    for (auto i = 0u; i < kNumPoints; ++i, ++vertex) {
      vertex->SetDiffuse(1.f, 1.f, 1.f, 1.f);
    }
    test_buffer->Unlock();
  }

  std::vector<uint32_t> indices;
  for (auto i = 0u; i < kNumPoints; ++i) {
    indices.push_back(i);
  }

  int text_row = kFirstRowTextRow;
  float top = kFirstRowTop;
  for (auto &mode : kDrawModes) {
    const float prime_y = top;
    const float test_y = top + 28.f;

    auto position = [](const std::shared_ptr<VertexBuffer> &buffer, float y) {
      auto vertex = buffer->Lock();
      for (auto i = 0u; i < kNumPoints; ++i, ++vertex) {
        vertex->SetPosition(kPointsLeft + kPointsSpacing * static_cast<float>(i), y, 0.f);
      }
      buffer->Unlock();
    };
    position(prime_buffer, prime_y);
    position(test_buffer, test_y);

    ActivateShader(prime_attr_shader_);
    DrawBuffer(host_, mode.mode, prime_buffer, indices, kTexcoordAttributes, TestHost::PRIMITIVE_POINTS);

    ActivateShader(position_and_color_shader_);
    DrawBuffer(host_, mode.mode, test_buffer, indices, kBasicAttributes, TestHost::PRIMITIVE_POINTS);

    pb_printat(text_row, 1, "%s", mode.name);
    host_.PBKitBusyWait();

    text_row += kTextRowsPerRow;
    top += kRowSpacing;
  }

  pb_printat(0, 0, "%s", name.c_str());
  pb_printat(1, 1, "Top: oT%u primed per vertex. Bottom: unwritten", stage);
  pb_draw_text_screen();

  FinishDraw(name);
}
