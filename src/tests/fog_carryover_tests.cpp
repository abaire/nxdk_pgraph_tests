#include "fog_carryover_tests.h"

#include <light.h>
#include <models/flat_mesh_grid_model.h>
#include <pbkit/pbkit.h>
#include <shaders/passthrough_vertex_shader.h>

#include "debug_output.h"
#include "test_host.h"

// clang-format off
static constexpr uint32_t kShader[] = {
#include "passthrough_just_position_and_color.vshinc"

};
// clang-format on

// In ABGR format.
static constexpr uint32_t kFogColor = 0xFF0000FF;

static constexpr char kTestName[] = "FogCarryover";
static constexpr vector_t kDiffuse{0.f, 0.f, 1.f, 1.f};

static constexpr uint32_t kFogModes[] = {
    NV097_SET_FOG_MODE_V_LINEAR,  NV097_SET_FOG_MODE_V_EXP,      NV097_SET_FOG_MODE_V_EXP2,
    NV097_SET_FOG_MODE_V_EXP_ABS, NV097_SET_FOG_MODE_V_EXP2_ABS, NV097_SET_FOG_MODE_V_LINEAR_ABS,
};

enum class FogCarryoverTestsDrawMode { DRAW_ARRAYS, DRAW_INLINE_BUFFERS, DRAW_INLINE_ELEMENTS, DRAW_INLINE_ARRAYS };

struct LabeledDrawMode {
  FogCarryoverTestsDrawMode mode;
  const char *name;
};

static constexpr LabeledDrawMode kDrawModes[] = {
    {FogCarryoverTestsDrawMode::DRAW_INLINE_BUFFERS, "IB"},
    {FogCarryoverTestsDrawMode::DRAW_INLINE_ELEMENTS, "IE"},
    {FogCarryoverTestsDrawMode::DRAW_INLINE_ARRAYS, "IA"},
    {FogCarryoverTestsDrawMode::DRAW_ARRAYS, "DA"},
};

static std::string FogModeName(uint32_t fog_mode) {
  switch (fog_mode) {
    case NV097_SET_FOG_MODE_V_LINEAR:
      return "lin";
    case NV097_SET_FOG_MODE_V_EXP:
      return "exp";
    case NV097_SET_FOG_MODE_V_EXP2:
      return "exp2";
    case NV097_SET_FOG_MODE_V_EXP_ABS:
      return "exp_abs";
    case NV097_SET_FOG_MODE_V_EXP2_ABS:
      return "exp2_abs";
    case NV097_SET_FOG_MODE_V_LINEAR_ABS:
      return "lin_abs";
    default:
      ASSERT(!"Unhandled fog mode");
  }
}

//! Returns the bias parameter needed to remove all fog.
static float ZeroBias(uint32_t fog_mode) {
  switch (fog_mode) {
    case NV097_SET_FOG_MODE_V_LINEAR:
    case NV097_SET_FOG_MODE_V_LINEAR_ABS:
      return 2.f;
    case NV097_SET_FOG_MODE_V_EXP:
    case NV097_SET_FOG_MODE_V_EXP_ABS:
      return 1.51f;
    case NV097_SET_FOG_MODE_V_EXP2:
    case NV097_SET_FOG_MODE_V_EXP2_ABS:
      return 1.5f;
    default:
      ASSERT(!"Unhandled fog mode");
  }
}

/**
 * @tc FogCarryover
 *   The left column triangles are rendered with the fog coordinate explicitly passed through from the vertex by the
 *   shader. The right column triangles do not specify the fog coordinate at all.
 *
 * @tc CarryoverPoints
 *   Renders a series of primitives using various draw modes on the left, setting fog coordinates. Then renders a quad
 *   using the same draw mode on the right without specifying a fog coordinate at all. The fog mode is set to EXP.
 *
 * @tc CarryoverLines
 *   Renders a series of primitives using various draw modes on the left, setting fog coordinates. Then renders a quad
 *   using the same draw mode on the right without specifying a fog coordinate at all. The fog mode is set to EXP.
 *
 * @tc CarryoverLineLoop
 *   Renders a series of primitives using various draw modes on the left, setting fog coordinates. Then renders a quad
 *   using the same draw mode on the right without specifying a fog coordinate at all. The fog mode is set to EXP.
 *
 * @tc CarryoverLineStrip
 *   Renders a series of primitives using various draw modes on the left, setting fog coordinates. Then renders a quad
 *   using the same draw mode on the right without specifying a fog coordinate at all. The fog mode is set to EXP.
 *
 * @tc CarryoverTris
 *   Renders a series of primitives using various draw modes on the left, setting fog coordinates. Then renders a quad
 *   using the same draw mode on the right without specifying a fog coordinate at all. The fog mode is set to EXP.
 *
 * @tc CarryoverTriStrip
 *   Renders a series of primitives using various draw modes on the left, setting fog coordinates. Then renders a quad
 *   using the same draw mode on the right without specifying a fog coordinate at all. The fog mode is set to EXP.
 *
 * @tc CarryoverTriFan
 *   Renders a series of primitives using various draw modes on the left, setting fog coordinates. Then renders a quad
 *   using the same draw mode on the right without specifying a fog coordinate at all. The fog mode is set to EXP.
 *
 * @tc CarryoverQuads
 *   Renders a series of primitives using various draw modes on the left, setting fog coordinates. Then renders a quad
 *   using the same draw mode on the right without specifying a fog coordinate at all. The fog mode is set to EXP.
 *
 * @tc CarryoverQuadStrip
 *   Renders a series of primitives using various draw modes on the left, setting fog coordinates. Then renders a quad
 *   using the same draw mode on the right without specifying a fog coordinate at all. The fog mode is set to EXP.
 *
 * @tc CarryoverPoly
 *   Renders a series of primitives using various draw modes on the left, setting fog coordinates. Then renders a quad
 *   using the same draw mode on the right without specifying a fog coordinate at all. The fog mode is set to EXP.
 *
 */
FogCarryoverTests::FogCarryoverTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Fog carryover", config) {
  tests_[kTestName] = [this]() { Test(); };

  static constexpr TestHost::DrawPrimitive kAllPrimitives[] = {
      TestHost::PRIMITIVE_POINTS,       TestHost::PRIMITIVE_LINES,     TestHost::PRIMITIVE_LINE_LOOP,
      TestHost::PRIMITIVE_LINE_STRIP,   TestHost::PRIMITIVE_TRIANGLES, TestHost::PRIMITIVE_TRIANGLE_STRIP,
      TestHost::PRIMITIVE_TRIANGLE_FAN, TestHost::PRIMITIVE_QUADS,     TestHost::PRIMITIVE_QUAD_STRIP,
      TestHost::PRIMITIVE_POLYGON,
  };
  for (auto primitive : kAllPrimitives) {
    std::string name = "Carryover" + TestHost::GetDrawPrimitiveName(primitive);
    tests_[name] = [this, primitive, name]() { TestPrimitive(name, primitive); };
  }
}

void FogCarryoverTests::Initialize() {
  TestSuite::Initialize();

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_FOG_ENABLE, true);
    // Note: Fog color is ABGR and not ARGB
    Pushbuffer::Push(NV097_SET_FOG_COLOR, kFogColor);
    // Fog gen mode is set to just use the fog coordinate from the shader.
    Pushbuffer::Push(NV097_SET_FOG_GEN_MODE, NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA);
    Pushbuffer::PushF(NV097_SET_FOG_PLANE, 0.f, 0.f, 2.f, 0.f);
    Pushbuffer::End();
  }

  // Just pass through the diffuse color.
  host_.SetCombinerControl(1);
  // Mix between the output pixel color and the fog color using the fog alpha value.
  host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_DIFFUSE, false, false, TestHost::SRC_FOG, false,
                          false);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_DIFFUSE,
                          true, false, /*specular_add_invert_r0*/ false, /* specular_add_invert_v1*/ false,
                          /* specular_clamp */ true);
}

void FogCarryoverTests::Test() {
  host_.PrepareDraw(0xFF201F20);

  static constexpr float kTriWidth = 96.f;
  static constexpr float kTriHeight = 42.f;
  static constexpr float kTriVSpacing = 8.f;
  static constexpr float kTriZ = 0.f;

  const float start_x = (host_.GetFramebufferWidthF() - (kTriWidth * 2.f)) / 3.f;
  const float y_inc = kTriHeight + kTriVSpacing;
  float top = 62.f;
  int text_row = 2;
  static constexpr int kTextRowInc = 2;

  for (auto fog_mode : kFogModes) {
    auto draw_tri = [this, fog_mode](float left, float top) {
      const auto right = left + kTriWidth;
      const auto bottom = top + kTriHeight;

      host_.Begin(TestHost::PRIMITIVE_TRIANGLES);

      host_.SetDiffuse(kDiffuse);
      host_.SetSpecular(kDiffuse);

      float fog_coord = 1.f;
      switch (fog_mode) {
        case NV097_SET_FOG_MODE_V_LINEAR:
        case NV097_SET_FOG_MODE_V_LINEAR_ABS:
          fog_coord = 0.6f;
          break;
        case NV097_SET_FOG_MODE_V_EXP:
        case NV097_SET_FOG_MODE_V_EXP_ABS:
          fog_coord = 0.1f;
          break;
        case NV097_SET_FOG_MODE_V_EXP2:
        case NV097_SET_FOG_MODE_V_EXP2_ABS:
          fog_coord = 0.2f;
          break;
        default:
          break;
      }

      host_.SetFogCoord(fog_coord);

      host_.SetVertex(left, top, kTriZ);
      host_.SetVertex(right, top, kTriZ);
      host_.SetVertex(left + (right - left) * 0.5f, bottom, kTriZ);

      host_.End();
    };

    auto shader = std::make_shared<PassthroughVertexShader>();
    host_.SetVertexShaderProgram(shader);
    shader->PrepareDraw();

    {
      Pushbuffer::Begin();
      Pushbuffer::Push(NV097_SET_FOG_MODE, fog_mode);
      // The last parameter never seems to have an effect.
      Pushbuffer::PushF(NV097_SET_FOG_PARAMS, ZeroBias(fog_mode), -1.f, 0.f);
      Pushbuffer::End();
    }

    // Draw with the fog coordinates explicitly set to put the hardware into a consistent state.
    // This is done repeatedly to fix an apparent issue with parallelism, drawing only once can lead to a situation
    // where one or more of the vertices in the "unset" draw case below still have arbitrary values from previous
    // operations and are non-hermetic, leading to test results that are dependent on previous draws.
    for (uint32_t i = 0; i < 2; ++i) {
      draw_tri(start_x, top);
    }

    shader->SetShader(kShader, sizeof(kShader));
    shader->PrepareDraw();
    shader->Activate();

    draw_tri(start_x * 2.f, top);

    pb_printat(text_row, 4, "%8s", FogModeName(fog_mode).c_str());
    text_row += kTextRowInc;

    top += y_inc;
  }

  pb_printat(0, 0, "%s\n", kTestName);

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestName);
}

static std::shared_ptr<VertexBuffer> CreateLines(TestHost &host_, std::vector<uint32_t> &index_buffer, float left,
                                                 float top, float right, float bottom) {
  const auto half_width = floorf((right - left) * 0.5f);
  const auto half_height = floorf((bottom - top) * 0.5f);
  const auto quarter_width = floorf(half_width * 0.5f);
  const auto quarter_height = floorf(half_height * 0.5f);

  const auto cx = left + half_width;
  const auto cy = top + half_height;

  constexpr int kNumLines = 4;
  auto buffer = host_.AllocateVertexBuffer(kNumLines * 2);

  auto vertex = buffer->Lock();
  index_buffer.clear();
  auto index = 0;

  auto add_vertex = [&vertex, &index_buffer, &index](float x, float y, float z) {
    vertex->SetPosition(x, y, z);
    vertex->SetDiffuse(0.f, 1.f, 0.f, 1.f);
    index_buffer.push_back(index++);
    ++vertex;
  };

  add_vertex(left, top, 0.f);
  add_vertex(cx, top + quarter_height, 0.f);

  add_vertex(cx + quarter_width, top, 1.f);
  add_vertex(right, cy, 1.f);

  add_vertex(right, bottom, 3.f);
  add_vertex(cx, bottom - 4.f, 3.f);

  add_vertex(cx, cy, 10.f);
  add_vertex(left, bottom, 10.f);

  buffer->Unlock();

  return buffer;
}

static std::shared_ptr<VertexBuffer> CreateTriangles(TestHost &host_, std::vector<uint32_t> &index_buffer_, float left,
                                                     float top, float right, float bottom) {
  constexpr int kNumTriangles = 3;
  auto buffer = host_.AllocateVertexBuffer(kNumTriangles * 3);

  const auto half_width = floorf((right - left) * 0.5f);
  const auto quarter_width = floorf(half_width * 0.5f);

  const auto cx = left + half_width;

  int index = 0;
  Color color(0.f, 1.f, 0.f, 1.f);
  {
    float one[] = {left, top, 0.f};
    float two[] = {cx - quarter_width, top, 0.f};
    float three[] = {left, bottom, 0.f};
    buffer->DefineTriangle(index++, one, two, three, color, color, color);
  }

  {
    float one[] = {left + quarter_width, bottom, 0.1f};
    float two[] = {cx, top, 0.1f};
    float three[] = {right - quarter_width, bottom, 0.1f};
    buffer->DefineTriangle(index++, one, two, three, color, color, color);
  }

  {
    float one[] = {right, top, 1.f};
    float two[] = {right, bottom, 0.1f};
    float three[] = {cx + quarter_width, top, 0.f};
    buffer->DefineTriangle(index++, one, two, three, color, color, color);
  }

  index_buffer_.clear();
  for (auto i = 0; i < buffer->GetNumVertices(); ++i) {
    index_buffer_.push_back(i);
  }

  return buffer;
}

static std::shared_ptr<VertexBuffer> CreateTriangleStrip(TestHost &host_, std::vector<uint32_t> &index_buffer_,
                                                         float left, float top, float right, float bottom) {
  constexpr int kNumTriangles = 5;
  auto buffer = host_.AllocateVertexBuffer(3 + (kNumTriangles - 1));

  const auto half_width = floorf((right - left) * 0.5f);
  const auto quarter_width = floorf(half_width * 0.5f);
  const auto half_height = floorf((bottom - top) * 0.5f);
  const auto quarter_height = floorf(half_height * 0.5f);

  const auto cx = left + half_width;
  const auto cy = top + half_height;

  auto vertex = buffer->Lock();
  index_buffer_.clear();
  auto index = 0;

  auto add_vertex = [&vertex, &index, &index_buffer_](float x, float y, float z) {
    vertex->SetPosition(x, y, z);
    vertex->SetDiffuse(0.f, 1.f, 0.f);
    index_buffer_.push_back(index++);
    ++vertex;
  };

  add_vertex(left, bottom, 0.f);
  add_vertex(left, top, 0.f);
  add_vertex(cx, cy, 0.f);

  add_vertex(cx, top, 1.15f);

  add_vertex(right - quarter_width, cy - quarter_height, 1.3f);

  add_vertex(right - quarter_width, top, 1.7f);

  add_vertex(right, cy + quarter_height, 0.1f);

  buffer->Unlock();

  return buffer;
}

static std::shared_ptr<VertexBuffer> CreateTriangleFan(TestHost &host_, std::vector<uint32_t> &index_buffer_,
                                                       float left, float top, float right, float bottom) {
  constexpr int kNumTriangles = 3;
  auto buffer = host_.AllocateVertexBuffer(3 + (kNumTriangles - 1));

  const auto half_width = floorf((right - left) * 0.5f);
  const auto quarter_width = floorf(half_width * 0.5f);
  const auto half_height = floorf((bottom - top) * 0.5f);
  const auto quarter_height = floorf(half_height * 0.5f);

  const auto cx = left + half_width;
  const auto cy = top + half_height;

  auto vertex = buffer->Lock();
  index_buffer_.clear();
  auto index = 0;

  auto add_vertex = [&vertex, &index, &index_buffer_](float x, float y, float z) {
    vertex->SetPosition(x, y, z);
    vertex->SetDiffuse(0.f, 1.f, 0.f);
    index_buffer_.push_back(index++);
    ++vertex;
  };

  add_vertex(cx, cy, 0.f);
  add_vertex(left, cy + quarter_height, 1.f);
  add_vertex(left + quarter_width, top, 2.f);

  add_vertex(cx + quarter_width, cy - quarter_height, 4.f);

  add_vertex(right, bottom, 3.f);

  buffer->Unlock();

  return buffer;
}

static std::shared_ptr<VertexBuffer> CreateQuads(TestHost &host_, std::vector<uint32_t> &index_buffer_, float left,
                                                 float top, float right, float bottom) {
  const auto half_width = floorf((right - left) * 0.5f);
  const auto quarter_width = floorf(half_width * 0.5f);
  const auto half_height = floorf((bottom - top) * 0.5f);
  const auto quarter_height = floorf(half_height * 0.5f);

  const auto cx = left + half_width;
  const auto cy = top + half_height;

  constexpr int kNumQuads = 3;
  auto buffer = host_.AllocateVertexBuffer(kNumQuads * 4);

  auto vertex = buffer->Lock();
  index_buffer_.clear();
  auto index = 0;

  auto add_vertex = [&vertex, &index, &index_buffer_](float x, float y, float z, float r, float g, float b) {
    vertex->SetPosition(x, y, z);
    vertex->SetDiffuse(r, g, b);
    index_buffer_.push_back(index++);
    ++vertex;
  };

  add_vertex(left, top, 0.f, 1.0f, 1.0f, 1.0f);
  add_vertex(cx - quarter_width, top, 1.7f, 0.5f, 0.5f, 0.6f);
  add_vertex(cx - quarter_width, cy, 1.7f, 0.7f, 0.7f, 0.7f);
  add_vertex(left, cy, 1.7f, 0.33f, 0.33f, 0.33f);

  add_vertex(cx, top, 0.f, 1.0f, 0.0f, 1.0f);
  add_vertex(right, top, 0.f, 0.1f, 0.3f, 0.6f);
  add_vertex(right, cy, 0.f, 1.0f, 1.0f, 0.0f);
  add_vertex(cx, cy, 0.f, 0.0f, 1.0f, 1.0f);

  add_vertex(left, cy + quarter_height, 0.1f, 0.25f, 0.25f, 0.25f);
  add_vertex(right, cy + quarter_height, 0.1f, 0.1f, 0.3f, 0.1f);
  add_vertex(right, bottom, 0.1f, 0.65f, 0.65f, 0.65f);
  add_vertex(left, bottom, 0.1f, 0.45f, 0.45f, 0.45f);

  buffer->Unlock();

  return buffer;
}

static std::shared_ptr<VertexBuffer> CreateQuadStrip(TestHost &host_, std::vector<uint32_t> &index_buffer_, float left,
                                                     float top, float right, float bottom) {
  const auto half_width = floorf((right - left) * 0.5f);
  const auto cx = left + half_width;
  const auto half_height = floorf((bottom - top) * 0.5f);
  const auto quarter_height = floorf(half_height * 0.5f);

  constexpr int kNumQuads = 2;
  auto buffer = host_.AllocateVertexBuffer(4 + (kNumQuads - 1) * 2);

  auto vertex = buffer->Lock();
  index_buffer_.clear();
  auto index = 0;

  auto add_vertex = [&vertex, &index, &index_buffer_](float x, float y, float z) {
    vertex->SetPosition(x, y, z);
    vertex->SetDiffuse(0.f, 1.f, 0.f);
    index_buffer_.push_back(index++);
    ++vertex;
  };

  add_vertex(left, bottom, 10.f);
  add_vertex(left, top, 0.f);
  add_vertex(cx, bottom - quarter_height, 20.f);
  add_vertex(cx, top + quarter_height, 20.f);

  add_vertex(right, bottom, 0.5f);
  add_vertex(right, top, 5.f);

  buffer->Unlock();

  return buffer;
}

static std::shared_ptr<VertexBuffer> CreatePolygon(TestHost &host_, std::vector<uint32_t> &index_buffer_, float left,
                                                   float top, float right, float bottom) {
  const auto half_width = floorf((right - left) * 0.5f);
  const auto quarter_width = floorf(half_width * 0.5f);
  const auto half_height = floorf((bottom - top) * 0.5f);
  const auto quarter_height = floorf(half_height * 0.5f);

  const auto cx = left + half_width;
  const auto cy = top + half_height;

  constexpr int kNumVertices = 5;
  auto buffer = host_.AllocateVertexBuffer(kNumVertices);

  auto vertex = buffer->Lock();
  index_buffer_.clear();
  auto index = 0;

  auto add_vertex = [&vertex, &index, &index_buffer_](float x, float y, float z, float r, float g, float b) {
    vertex->SetPosition(x, y, z);
    vertex->SetDiffuse(r, g, b);
    index_buffer_.push_back(index++);
    ++vertex;
  };

  add_vertex(cx - quarter_width, bottom, 0.f, 0.33f, 0.7f, 0.33f);
  add_vertex(left, cy, 0.f, 0.7f, 0.1f, 0.0f);
  add_vertex(left, top, 0.f, 0.1f, 0.7f, 0.5f);
  add_vertex(right, top + quarter_height, 0.f, 0.0f, 0.9f, 0.6f);
  add_vertex(right - quarter_width, bottom - quarter_height, 0.f, 0.4f, 0.25f, 0.9f);

  buffer->Unlock();

  return buffer;
}

void FogCarryoverTests::TestPrimitive(const std::string &name, TestHost::DrawPrimitive primitive) {
  auto draw_shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(draw_shader);
  draw_shader->PrepareDraw();

  auto carryover_shader = std::make_shared<PassthroughVertexShader>();
  carryover_shader->SetShader(kShader, sizeof(kShader));
  carryover_shader->PrepareDraw();

  host_.PrepareDraw(0xFFAAAAAA);

  static constexpr float kFogCoord = 0.6f;

  static constexpr float kTestSize = 64.f;
  static constexpr float kLeft = 8.f;
  static constexpr float kTop = 64.f;
  const auto kCenterX = floorf(host_.GetFramebufferWidthF() * 0.5f);
  const auto kRight = kCenterX - kLeft;

  // Build up a vertex buffer that will render a test quad
  std::shared_ptr<VertexBuffer> quad_buffer = host_.AllocateVertexBuffer(4);
  std::vector<uint32_t> quad_indices;
  {
    auto vertex = quad_buffer->Lock();
    auto index = 0;

    const float left = floorf(kCenterX + (kCenterX - kTestSize) * 0.5f);

    vertex->SetPosition(left, kTop, 0.f);
    vertex->SetDiffuse(0.f, 0.f, 1.f);
    quad_indices.push_back(index++);
    ++vertex;
    vertex->SetPosition(left + kTestSize, kTop, 0.f);
    vertex->SetDiffuse(0.f, 0.f, 1.f);
    quad_indices.push_back(index++);
    ++vertex;
    vertex->SetPosition(left + kTestSize, kTop + kTestSize, 0.f);
    vertex->SetDiffuse(0.f, 0.f, 1.f);
    quad_indices.push_back(index++);
    ++vertex;
    vertex->SetPosition(left, kTop + kTestSize, 0.f);
    vertex->SetDiffuse(0.f, 0.f, 1.f);
    quad_indices.push_back(index);

    quad_buffer->Unlock();
  }

  std::shared_ptr<VertexBuffer> draw_buffer;
  std::vector<uint32_t> index_buffer;
  switch (primitive) {
    case TestHost::PRIMITIVE_POINTS:
    case TestHost::PRIMITIVE_LINES:
    case TestHost::PRIMITIVE_LINE_LOOP:
    case TestHost::PRIMITIVE_LINE_STRIP:
      draw_buffer = CreateLines(host_, index_buffer, kLeft, kTop, kRight, kTop + kTestSize);
      break;
    case TestHost::PRIMITIVE_TRIANGLES:
      draw_buffer = CreateTriangles(host_, index_buffer, kLeft, kTop, kRight, kTop + kTestSize);
      break;
    case TestHost::PRIMITIVE_TRIANGLE_STRIP:
      draw_buffer = CreateTriangleStrip(host_, index_buffer, kLeft, kTop, kRight, kTop + kTestSize);
      break;
    case TestHost::PRIMITIVE_TRIANGLE_FAN:
      draw_buffer = CreateTriangleFan(host_, index_buffer, kLeft, kTop, kRight, kTop + kTestSize);
      break;
    case TestHost::PRIMITIVE_QUADS:
      draw_buffer = CreateQuads(host_, index_buffer, kLeft, kTop, kRight, kTop + kTestSize);
      break;
    case TestHost::PRIMITIVE_QUAD_STRIP:
      draw_buffer = CreateQuadStrip(host_, index_buffer, kLeft, kTop, kRight, kTop + kTestSize);
      break;
    case TestHost::PRIMITIVE_POLYGON:
      draw_buffer = CreatePolygon(host_, index_buffer, kLeft, kTop, kRight, kTop + kTestSize);
      break;
  }

  // Set the fog coord on the buffer.
  {
    auto vertex = draw_buffer->Lock();
    for (auto idx = 0; idx < draw_buffer->GetNumVertices(); ++idx, ++vertex) {
      vertex->SetFogCoord(kFogCoord);
    }
    draw_buffer->Unlock();
  }

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_FOG_MODE, NV097_SET_FOG_MODE_V_EXP);
  // The last parameter never seems to have an effect.
  Pushbuffer::PushF(NV097_SET_FOG_PARAMS, ZeroBias(NV097_SET_FOG_MODE_V_EXP), -1.f, 0.f);
  Pushbuffer::End();

  auto activate_carryover_shader = [this, &draw_shader, &carryover_shader](bool activate) {
    if (activate) {
      host_.SetVertexShaderProgram(carryover_shader);
      carryover_shader->PrepareDraw();
      carryover_shader->Activate();
    } else {
      host_.SetVertexShaderProgram(draw_shader);
      draw_shader->PrepareDraw();
      draw_shader->Activate();
    }
  };

  auto draw = [this, primitive, &draw_buffer, &index_buffer, &quad_buffer, &quad_indices,
               &activate_carryover_shader](FogCarryoverTestsDrawMode draw_mode) {
    static constexpr uint32_t kQuadAttributes = TestHost::POSITION | TestHost::DIFFUSE;
    static constexpr uint32_t kAttributes = kQuadAttributes | TestHost::FOG_COORD;
    activate_carryover_shader(false);
    host_.SetVertexBuffer(draw_buffer);
    switch (draw_mode) {
      case FogCarryoverTestsDrawMode::DRAW_ARRAYS:
        host_.DrawArrays(kAttributes, primitive);
        activate_carryover_shader(true);
        host_.SetVertexBuffer(quad_buffer);
        host_.DrawArrays(kQuadAttributes, TestHost::PRIMITIVE_QUADS);
        break;

      case FogCarryoverTestsDrawMode::DRAW_INLINE_BUFFERS:
        host_.DrawInlineBuffer(kAttributes, primitive);
        activate_carryover_shader(true);
        host_.SetVertexBuffer(quad_buffer);
        host_.DrawInlineBuffer(kQuadAttributes, TestHost::PRIMITIVE_QUADS);
        break;

      case FogCarryoverTestsDrawMode::DRAW_INLINE_ELEMENTS:
        host_.DrawInlineElements16(index_buffer, kAttributes, primitive);
        activate_carryover_shader(true);
        host_.SetVertexBuffer(quad_buffer);
        host_.DrawInlineElements16(quad_indices, kQuadAttributes, TestHost::PRIMITIVE_QUADS);
        break;

      case FogCarryoverTestsDrawMode::DRAW_INLINE_ARRAYS:
        host_.DrawInlineArray(kAttributes, primitive);
        activate_carryover_shader(true);
        host_.SetVertexBuffer(quad_buffer);
        host_.DrawInlineArray(kQuadAttributes, TestHost::PRIMITIVE_QUADS);
        break;
    }
  };

  static constexpr float kTestVerticalSpacing = kTestSize + 40.f;
  static constexpr int kTextColumn = 30;
  int text_row = 1;

  auto shift_geometry_vertically = [](std::shared_ptr<VertexBuffer> &buffer, float delta_y) {
    auto vertex = buffer->Lock();
    for (auto idx = 0; idx < buffer->GetNumVertices(); ++idx, ++vertex) {
      vertex->pos[1] += delta_y;
    }
    buffer->Unlock();
  };

  for (auto &mode : kDrawModes) {
    draw(mode.mode);
    pb_printat(text_row, kTextColumn, "%s", mode.name);

    // Allow the draw to fully complete before modifying the buffers, since DMA may still be reading from them.
    host_.PBKitBusyWait();

    shift_geometry_vertically(draw_buffer, kTestVerticalSpacing);
    shift_geometry_vertically(quad_buffer, kTestVerticalSpacing);
    text_row += 4;
  }

  pb_printat(0, 0, "%s", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
