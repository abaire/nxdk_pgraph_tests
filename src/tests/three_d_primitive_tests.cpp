#include "three_d_primitive_tests.h"

#include <pbkit/pbkit.h>

#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "vertex_buffer.h"

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
static constexpr uint32_t kDefaultDMAChannelA = 3;
// From pbkit.c, DMA_COLOR is set to channel 9 by default.
// NV097_SET_CONTEXT_DMA_COLOR == NV20_TCL_PRIMITIVE_3D_SET_OBJECT3
static constexpr uint32_t kDefaultDMAColorChannel = 9;

// clang-format off
static constexpr ThreeDPrimitiveTests::DrawMode kDrawModes[] = {
    ThreeDPrimitiveTests::DRAW_ARRAYS,
    ThreeDPrimitiveTests::DRAW_INLINE_BUFFERS,
    ThreeDPrimitiveTests::DRAW_INLINE_ARRAYS,
    ThreeDPrimitiveTests::DRAW_INLINE_ELEMENTS,
};

static constexpr TestHost::DrawPrimitive kPrimitives[] = {
    TestHost::PRIMITIVE_POINTS,
    TestHost::PRIMITIVE_LINES,
    TestHost::PRIMITIVE_LINE_LOOP,
    TestHost::PRIMITIVE_LINE_STRIP,
    TestHost::PRIMITIVE_TRIANGLES,
    TestHost::PRIMITIVE_TRIANGLE_STRIP,
    TestHost::PRIMITIVE_TRIANGLE_FAN,
    TestHost::PRIMITIVE_QUADS,
    TestHost::PRIMITIVE_QUAD_STRIP,
    TestHost::PRIMITIVE_POLYGON
};
// clang-format on

static constexpr float kLeft = -2.75f;
static constexpr float kRight = 2.75f;
static constexpr float kTop = 1.75f;
static constexpr float kBottom = -1.75f;
static constexpr float kZFront = 1.0f;
static constexpr float kZBack = 5.0f;

ThreeDPrimitiveTests::ThreeDPrimitiveTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "3D primitive", config) {
  for (auto line_smooth : {false, true}) {
    for (auto poly_smooth : {false, true}) {
      for (const auto primitive : kPrimitives) {
        for (const auto draw_mode : kDrawModes) {
          const std::string test_name = MakeTestName(primitive, draw_mode, line_smooth, poly_smooth);
          auto test = [this, primitive, draw_mode, line_smooth, poly_smooth]() {
            this->Test(primitive, draw_mode, line_smooth, poly_smooth);
          };
          tests_[test_name] = test;
        }
      }
    }
  }
}

void ThreeDPrimitiveTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void ThreeDPrimitiveTests::CreateLines() {
  constexpr int kNumLines = 6;
  auto buffer = host_.AllocateVertexBuffer(kNumLines * 2);

  auto vertex = buffer->Lock();
  index_buffer_.clear();
  auto index = 0;

  vertex->SetPosition(kLeft, kTop, kZFront);
  vertex->SetDiffuseGrey(0.75f);
  index_buffer_.push_back(index++);
  ++vertex;
  vertex->SetPosition(kRight, kTop, kZFront);
  vertex->SetDiffuseGrey(1.0f);
  index_buffer_.push_back(index++);
  ++vertex;

  vertex->SetPosition(-2, 1, kZFront);
  vertex->SetDiffuse(1.0f, 0.0f, 0.0f);
  index_buffer_.push_back(index++);
  ++vertex;
  vertex->SetPosition(2, 0, kZBack);
  vertex->SetDiffuse(1.0f, 0.0f, 0.0f);
  index_buffer_.push_back(index++);
  ++vertex;

  vertex->SetPosition(1.5, 0.5, kZBack);
  vertex->SetDiffuse(0.0f, 1.0f, 0.0f);
  index_buffer_.push_back(index++);
  ++vertex;
  vertex->SetPosition(-1.5, 0.75, kZBack);
  vertex->SetDiffuse(0.0f, 1.0f, 0.0f);
  index_buffer_.push_back(index++);
  ++vertex;

  vertex->SetPosition(kRight, 0.25, kZFront);
  vertex->SetDiffuseGrey(1.0f);
  index_buffer_.push_back(index++);
  ++vertex;
  vertex->SetPosition(1.75f, 1.25, kZFront);
  vertex->SetDiffuseGrey(0.15f);
  index_buffer_.push_back(index++);
  ++vertex;

  vertex->SetPosition(kLeft, 1.0f, kZFront);
  vertex->SetDiffuse(0.25f, 0.25f, 1.0f);
  index_buffer_.push_back(index++);
  ++vertex;
  vertex->SetPosition(kLeft, -1.0f, kZFront);
  vertex->SetDiffuse(0.65f, 0.65f, 1.0f);
  index_buffer_.push_back(index++);
  ++vertex;

  vertex->SetPosition(kLeft, kBottom, kZBack);
  vertex->SetDiffuse(0.0f, 1.0f, 1.0f);
  index_buffer_.push_back(index++);
  ++vertex;
  vertex->SetPosition(kRight, kBottom, kZBack);
  vertex->SetDiffuse(0.5f, 0.5f, 1.0f);
  index_buffer_.push_back(index++);
  ++vertex;

  buffer->Unlock();
}

void ThreeDPrimitiveTests::CreateTriangles() {
  constexpr int kNumTriangles = 3;
  auto buffer = host_.AllocateVertexBuffer(kNumTriangles * 3);

  int index = 0;
  Color color_one, color_two, color_three;
  {
    color_one.SetGrey(0.25f);
    color_two.SetGrey(0.55f);
    color_three.SetGrey(0.75f);

    float one[] = {kLeft, kTop, kZFront};
    float two[] = {0.0f, kTop, kZFront};
    float three[] = {kLeft, 0.0f, kZFront};
    buffer->DefineTriangle(index++, one, two, three, color_one, color_two, color_three);
  }

  {
    color_one.SetRGB(0.8f, 0.25f, 0.10f);
    color_two.SetRGB(0.1f, 0.85f, 0.10f);
    color_three.SetRGB(0.1f, 0.25f, 0.90f);

    float one[] = {kRight, kTop, kZFront};
    float two[] = {0.10f, kBottom, kZBack};
    float three[] = {0.25f, 0.0f, kZFront};
    buffer->DefineTriangle(index++, one, two, three, color_one, color_two, color_three);
  }

  {
    color_one.SetRGB(0.8f, 0.25f, 0.90f);
    color_two.SetRGB(0.1f, 0.85f, 0.90f);
    color_three.SetRGB(0.85f, 0.95f, 0.10f);

    float one[] = {-0.4f, kBottom, kZBack};
    float two[] = {-1.4f, -1.4, kZBack};
    float three[] = {0.0f, 0.0f, kZBack};
    buffer->DefineTriangle(index++, one, two, three, color_one, color_two, color_three);
  }

  index_buffer_.clear();
  for (auto i = 0; i < buffer->GetNumVertices(); ++i) {
    index_buffer_.push_back(i);
  }
}

void ThreeDPrimitiveTests::CreateTriangleStrip() {
  constexpr int kNumTriangles = 6;
  auto buffer = host_.AllocateVertexBuffer(3 + (kNumTriangles - 1));

  auto vertex = buffer->Lock();
  index_buffer_.clear();
  auto index = 0;

  auto add_vertex = [&vertex, &index, this](float x, float y, float z, float r, float g, float b) {
    vertex->SetPosition(x, y, z);
    vertex->SetDiffuse(r, g, b);
    this->index_buffer_.push_back(index++);
    ++vertex;
  };

  add_vertex(kLeft, 0.0f, kZFront, 1.0f, 0.0f, 0.0f);
  add_vertex(-2.25f, kTop, kZFront, 0.0f, 1.0f, 0.0f);
  add_vertex(-2.0f, kBottom, kZFront, 0.0f, 0.0f, 1.0f);

  add_vertex(-1.3f, 1.6, 1.15f, 0.25f, 0.0f, 0.8f);

  add_vertex(0.0f, -1.5f, 1.3f, 0.75f, 0.0f, 0.25f);

  add_vertex(0.4f, 1.0f, 1.7f, 0.33f, 0.33f, 0.33f);

  add_vertex(1.4f, -0.6f, kZBack, 0.7f, 0.7f, 0.7f);

  add_vertex(kRight, kTop, kZBack, 0.5f, 0.5f, 0.6f);

  buffer->Unlock();
}

void ThreeDPrimitiveTests::CreateTriangleFan() {
  constexpr int kNumTriangles = 6;
  auto buffer = host_.AllocateVertexBuffer(3 + (kNumTriangles - 1));

  auto vertex = buffer->Lock();
  index_buffer_.clear();
  auto index = 0;

  auto add_vertex = [&vertex, &index, this](float x, float y, float z, float r, float g, float b) {
    vertex->SetPosition(x, y, z);
    vertex->SetDiffuse(r, g, b);
    this->index_buffer_.push_back(index++);
    ++vertex;
  };

  add_vertex(0.0f, -0.75f, kZFront, 1.0f, 1.0f, 1.0f);
  add_vertex(-2.25f, kBottom, kZFront, 0.0f, 0.0f, 1.0f);
  add_vertex(-2.0f, kTop, kZFront, 0.0f, 1.0f, 0.0f);

  add_vertex(-0.6f, 0.65f, 1.15f, 0.25f, 0.0f, 0.8f);

  add_vertex(0.0f, 1.5f, 1.3f, 0.75f, 0.0f, 0.25f);

  add_vertex(0.4f, 1.0f, 1.7f, 0.33f, 0.33f, 0.33f);

  add_vertex(2.4f, 0.6f, kZBack, 0.7f, 0.7f, 0.7f);

  add_vertex(kRight, kBottom, kZBack, 0.5f, 0.5f, 0.6f);

  buffer->Unlock();
}

void ThreeDPrimitiveTests::CreateQuads() {
  constexpr int kNumQuads = 3;
  auto buffer = host_.AllocateVertexBuffer(kNumQuads * 4);

  auto vertex = buffer->Lock();
  index_buffer_.clear();
  auto index = 0;

  auto add_vertex = [&vertex, &index, this](float x, float y, float z, float r, float g, float b) {
    vertex->SetPosition(x, y, z);
    vertex->SetDiffuse(r, g, b);
    this->index_buffer_.push_back(index++);
    ++vertex;
  };

  add_vertex(kLeft, kTop, kZFront, 1.0f, 1.0f, 1.0f);
  add_vertex(-0.6f, 1.6f, 1.7f, 0.5f, 0.5f, 0.6f);
  add_vertex(-1.35f, -0.6f, 1.7f, 0.7f, 0.7f, 0.7f);
  add_vertex(-2.4f, kBottom, 1.7f, 0.33f, 0.33f, 0.33f);

  add_vertex(0.06f, -0.09f, kZFront, 1.0f, 0.0f, 1.0f);
  add_vertex(1.9f, -0.25f, kZFront, 0.1f, 0.3f, 0.6f);
  add_vertex(2.62f, -1.03f, kZFront, 1.0f, 1.0f, 0.0f);
  add_vertex(1.25f, -1.65f, kZFront, 0.0f, 1.0f, 1.0f);

  add_vertex(-1.25f, 1.3f, kZBack, 0.25f, 0.25f, 0.25f);
  add_vertex(0.3f, 1.25f, kZBack, 0.1f, 0.3f, 0.1f);
  add_vertex(2.62f, -0.03f, kZBack, 0.65f, 0.65f, 0.65f);
  add_vertex(-1.0f, -1.0f, kZBack, 0.45f, 0.45f, 0.45f);

  buffer->Unlock();
}

void ThreeDPrimitiveTests::CreateQuadStrip() {
  constexpr int kNumQuads = 2;
  auto buffer = host_.AllocateVertexBuffer(4 + (kNumQuads - 1) * 2);

  auto vertex = buffer->Lock();
  index_buffer_.clear();
  auto index = 0;

  auto add_vertex = [&vertex, &index, this](float x, float y, float z, float r, float g, float b) {
    vertex->SetPosition(x, y, z);
    vertex->SetDiffuse(r, g, b);
    this->index_buffer_.push_back(index++);
    ++vertex;
  };

  add_vertex(kLeft, kBottom, kZFront, 0.33f, 0.33f, 0.33f);
  add_vertex(kLeft, kTop, kZFront, 1.0f, 1.0f, 1.0f);
  add_vertex(0.0f, -1.35f, kZFront, 0.7f, 0.1f, 0.0f);
  add_vertex(0.0f, 1.0f, kZFront, 0.0f, 0.9f, 0.2f);

  add_vertex(kRight, kBottom, kZFront, 0.33f, 0.33f, 0.33f);
  add_vertex(kRight, kTop, kZFront, 0.0f, 0.7f, 0.9f);

  buffer->Unlock();
}

void ThreeDPrimitiveTests::CreatePolygon() {
  constexpr int kNumVertices = 5;
  auto buffer = host_.AllocateVertexBuffer(kNumVertices);

  auto vertex = buffer->Lock();
  index_buffer_.clear();
  auto index = 0;

  auto add_vertex = [&vertex, &index, this](float x, float y, float z, float r, float g, float b) {
    vertex->SetPosition(x, y, z);
    vertex->SetDiffuse(r, g, b);
    this->index_buffer_.push_back(index++);
    ++vertex;
  };

  add_vertex(kLeft, kBottom, kZFront, 0.33f, 0.33f, 0.33f);
  add_vertex(-1.4f, 1.1f, kZFront, 0.7f, 0.1f, 0.0f);
  add_vertex(-0.3f, kTop, kZFront, 0.1f, 0.7f, 0.5f);
  add_vertex(2.0f, 0.3f, kZFront, 0.0f, 0.9f, 0.2f);
  add_vertex(kRight, -1.5f, kZFront, 0.7f, 0.7f, 0.7f);

  buffer->Unlock();
}

void ThreeDPrimitiveTests::Test(TestHost::DrawPrimitive primitive, DrawMode draw_mode, bool line_smooth,
                                bool poly_smooth) {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

  if (line_smooth || poly_smooth) {
    const uint32_t kAAFramebufferPitch = host_.GetFramebufferWidth() * 4 * 2;
    const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(0));
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kAAFramebufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));
    pb_end(p);
    host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                           host_.GetFramebufferHeight(), false, 0, 0, 0, 0, TestHost::AA_CENTER_CORNER_2);
  }
  host_.PrepareDraw(kBackgroundColor);

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, true);
    p = pb_push1(p, NV097_SET_ALPHA_TEST_ENABLE, true);
    p = pb_push1(p, NV097_SET_SMOOTHING_CONTROL, 0xFFFF0000);  // From MS Dashboard, this enables poly smoothing
    p = pb_push1(p, NV097_SET_LINE_SMOOTH_ENABLE, line_smooth);
    p = pb_push1(p, NV097_SET_POLY_SMOOTH_ENABLE, poly_smooth);
    pb_end(p);
  }
  host_.SetBlend(true);

  switch (primitive) {
    case TestHost::PRIMITIVE_LINES:
    case TestHost::PRIMITIVE_POINTS:
    case TestHost::PRIMITIVE_LINE_LOOP:
    case TestHost::PRIMITIVE_LINE_STRIP:
      CreateLines();
      break;

    case TestHost::PRIMITIVE_TRIANGLES:
      CreateTriangles();
      break;

    case TestHost::PRIMITIVE_TRIANGLE_STRIP:
      CreateTriangleStrip();
      break;

    case TestHost::PRIMITIVE_TRIANGLE_FAN:
      CreateTriangleFan();
      break;

    case TestHost::PRIMITIVE_QUADS:
      CreateQuads();
      break;

    case TestHost::PRIMITIVE_QUAD_STRIP:
      CreateQuadStrip();
      break;

    case TestHost::PRIMITIVE_POLYGON:
      CreatePolygon();
      break;
  }

  uint32_t vertex_elements = host_.POSITION | host_.DIFFUSE;
  switch (draw_mode) {
    case DRAW_ARRAYS:
      host_.DrawArrays(vertex_elements, primitive);
      break;

    case DRAW_INLINE_BUFFERS:
      host_.DrawInlineBuffer(vertex_elements, primitive);
      break;

    case DRAW_INLINE_ELEMENTS:
      host_.DrawInlineElements16(index_buffer_, vertex_elements, primitive);
      break;

    case DRAW_INLINE_ARRAYS:
      host_.DrawInlineArray(vertex_elements, primitive);
      break;
  }

  {
    auto p = pb_begin();
    // From Tenchu: Return from Darkness, this disables poly smoothing regardless of whether the individual flags are
    // enabled.
    p = pb_push1(p, NV097_SET_SMOOTHING_CONTROL, 0xFFFF0001);
    p = pb_push1(p, NV097_SET_LINE_SMOOTH_ENABLE, false);
    p = pb_push1(p, NV097_SET_POLY_SMOOTH_ENABLE, false);
    pb_end(p);
  }

  // Render the antialiased texture to the screen.
  if (line_smooth || poly_smooth) {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
    pb_end(p);
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                    host_.GetFramebufferHeight());

    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    auto& texture_stage = host_.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
    texture_stage.SetImageDimensions(host_.GetFramebufferWidth() * 2, host_.GetFramebufferHeight());
    host_.SetupTextureStages();

    auto shader = std::make_shared<PrecalculatedVertexShader>();
    host_.SetVertexShaderProgram(shader);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(0.0f, 0.0f, 0.1f, 1.0f);

    host_.SetTexCoord0(host_.GetFramebufferWidthF() * 2.0f, 0.0f);
    host_.SetVertex(host_.GetFramebufferWidthF(), 0.0f, 0.1f, 1.0f);

    host_.SetTexCoord0(host_.GetFramebufferWidthF() * 2.0f, host_.GetFramebufferHeightF());
    host_.SetVertex(host_.GetFramebufferWidthF(), host_.GetFramebufferHeightF(), 0.1f, 1.0f);

    host_.SetTexCoord0(0.0f, host_.GetFramebufferHeightF());
    host_.SetVertex(0.0f, host_.GetFramebufferHeightF(), 0.1f, 1.0f);
    host_.End();

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);

    host_.SetVertexShaderProgram(nullptr);
    host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  }

  std::string name = MakeTestName(primitive, draw_mode, line_smooth, poly_smooth);
  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}

std::string ThreeDPrimitiveTests::MakeTestName(TestHost::DrawPrimitive primitive,
                                               ThreeDPrimitiveTests::DrawMode draw_mode, bool line_smooth,
                                               bool poly_smooth) {
  std::string ret = TestHost::GetPrimitiveName(primitive);
  switch (draw_mode) {
    case DRAW_ARRAYS:
      break;

    case DRAW_INLINE_BUFFERS:
      ret += "-inlinebuf";
      break;

    case DRAW_INLINE_ARRAYS:
      ret += "-inlinearrays";
      break;

    case DRAW_INLINE_ELEMENTS:
      ret += "-inlineelements";
      break;
  }

  if (line_smooth) {
    ret += "-ls";
  }
  if (poly_smooth) {
    ret += "-ps";
  }

  return std::move(ret);
}
