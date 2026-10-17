#include "attribute_carryover_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"
#include "vertex_buffer.h"

// clang-format off
static constexpr uint32_t kShader[] = {
#include "attribute_carryover_tests.vshinc"
};
// clang-format on

static const TestHost::DrawPrimitive kPrimitives[] = {
    TestHost::PRIMITIVE_LINES,
    TestHost::PRIMITIVE_TRIANGLES,
};

static const AttributeCarryoverTests::Attribute kTestAttributes[]{
    AttributeCarryoverTests::ATTR_WEIGHT,        AttributeCarryoverTests::ATTR_NORMAL,
    AttributeCarryoverTests::ATTR_DIFFUSE,       AttributeCarryoverTests::ATTR_SPECULAR,
    AttributeCarryoverTests::ATTR_FOG_COORD,      // Not supported by cg.
    AttributeCarryoverTests::ATTR_POINT_SIZE,     // cg errors indicating not available on hardware.
    AttributeCarryoverTests::ATTR_BACK_DIFFUSE,   // Not supported by cg.
    AttributeCarryoverTests::ATTR_BACK_SPECULAR,  // Not supported by cg.
    AttributeCarryoverTests::ATTR_TEX0,          AttributeCarryoverTests::ATTR_TEX1,
    AttributeCarryoverTests::ATTR_TEX2,          AttributeCarryoverTests::ATTR_TEX3,
};

// All attribute values should have some amount of red to ensure that 1-component attributes (e.g., weight) are not
// simply a zero'd default.
static const AttributeCarryoverTests::TestConfig kTestConfigs[] = {
    {AttributeCarryoverTests::DRAW_ARRAYS, {0.05f, 0.0f, 1.0f, 1.0f}},
    {AttributeCarryoverTests::DRAW_INLINE_BUFFERS, {0.75f, 0.0f, 0.0f, 1.0f}},
    {AttributeCarryoverTests::DRAW_INLINE_ARRAYS, {0.5f, 0.0f, 0.0f, 1.0f}},
    {AttributeCarryoverTests::DRAW_INLINE_ELEMENTS, {0.25f, 0.0f, 0.6f, 1.0f}},
};

static TestHost::VertexAttribute TestAttributeToVertexAttribute(AttributeCarryoverTests::Attribute attribute);

static constexpr char kD3DColorTest[] = "D3DColor-da";
static constexpr char kD3DColorStrideZeroTest[] = "D3DColor-stride0";

// A8R8G8B8 with distinct R and B so that a channel swap is visible.
static constexpr uint32_t kD3DTestColor = 0xFFE08020;
// Color of the leading bleed vertices, never expected in the test primitive.
static constexpr uint32_t kD3DLeadingColor = 0xFF00FF00;
static constexpr uint32_t kD3DColorCount = 3;

static constexpr char kSquashedDrawArraysTest[] = "SquashedDrawArrays";

static void SetRawVertexAttribute(uint32_t index, uint32_t format, uint32_t size, uint32_t stride, const void *data) {
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_VERTEX_DATA_ARRAY_FORMAT + index * 4,
                   MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, format) |
                       MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, size) |
                       MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, stride));
  if (size && data) {
    Pushbuffer::Push(NV097_SET_VERTEX_DATA_ARRAY_OFFSET + index * 4, reinterpret_cast<uintptr_t>(data) & 0x03ffffff);
  }
  Pushbuffer::End();
}

static void ClearRawVertexAttribute(uint32_t index) {
  SetRawVertexAttribute(index, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 0, 0, nullptr);
}

static void DrawRawArrays(TestHost::DrawPrimitive primitive, uint32_t count) {
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_BREAK_VERTEX_BUFFER_CACHE, 0);
  Pushbuffer::Push(NV097_SET_BEGIN_END, primitive);
  Pushbuffer::Push(NV097_DRAW_ARRAYS,
                   MASK(NV097_DRAW_ARRAYS_COUNT, count - 1) | MASK(NV097_DRAW_ARRAYS_START_INDEX, 0));
  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  Pushbuffer::End();
}

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc D3DColor-da
 *  Renders a triangle whose diffuse color comes from a UB_D3D vertex array (green for the first two vertices,
 *  0xFFE08020 for the last), then renders a test triangle on the right that only provides positions. The left triangle
 *  should be a green to orange gradient. The test triangle should be solid orange (the carried value retains the
 *  shader-visible BGRA order); blue indicates the raw memory byte order was carried instead.
 *
 * @tc D3DColor-stride0
 *  As D3DColor-da, but the UB_D3D diffuse array has a stride of 0 so element 0 (0xFFE08020) is used for every vertex.
 *  Both triangles should be solid orange; any green indicates the stride was not treated as 0.
 *
 * @tc SquashedDrawArrays
 *  Renders two triangles from a single vertex buffer using back-to-back BEGIN/DRAW_ARRAYS/END sequences with no other
 *  methods between them. The first draw (left) uses vertices 3-5 and ends on a red vertex, the second (middle) uses
 *  vertices 0-2 and ends on a blue vertex. A test triangle (right) that only provides positions is then rendered with
 *  its diffuse color carried over. It should be solid blue, the last vertex submitted; red indicates that the highest
 *  vertex index was carried instead.
 *
 * @tc L-bd0.1_0.0_1.0_1.0-da
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from back diffuse using draw
 *  mode draw arrays.
 *
 * @tc L-bd0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from back diffuse using draw
 *  mode inline elements.
 *
 * @tc L-bd0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from back diffuse using draw
 *  mode inline arrays.
 *
 * @tc L-bd0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from back diffuse using draw
 *  mode inline buffers.
 *
 * @tc L-bs0.1_0.0_1.0_1.0-da
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from back specular using draw
 *  mode draw arrays.
 *
 * @tc L-bs0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from back specular using draw
 *  mode inline elements.
 *
 * @tc L-bs0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from back specular using using
 *  draw mode inline arrays.
 *
 * @tc L-bs0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from back specular using draw
 *  mode inline buffers.
 *
 * @tc L-d0.1_0.0_1.0_1.0-da
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from diffuse using draw mode
 *  draw arrays.
 *
 * @tc L-d0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from diffuse using draw mode
 *  inline elements.
 *
 * @tc L-d0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from diffuse using draw mode
 *  inline arrays.
 *
 * @tc L-d0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from diffuse using draw mode
 *  inline buffers.
 *
 * @tc L-fc0.1_0.0_1.0_1.0-da
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from fog using draw mode draw
 *  arrays.
 *
 * @tc L-fc0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from fog using draw mode
 *  inline elements.
 *
 * @tc L-fc0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from fog using draw mode
 *  inline arrays.
 *
 * @tc L-fc0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from fog using draw mode
 *  inline buffers.
 *
 * @tc L-n0.1_0.0_1.0_1.0-da
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from normal using draw mode
 *  draw arrays.
 *
 * @tc L-n0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from normal using draw mode
 *  inline elements.
 *
 * @tc L-n0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from normal using draw mode
 *  inline arrays.
 *
 * @tc L-n0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from normal using draw mode
 *  inline buffers.
 *
 * @tc L-ps0.1_0.0_1.0_1.0-da
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from point size using draw
 *  mode draw arrays.
 *
 * @tc L-ps0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from point size using draw
 *  mode inline elements.
 *
 * @tc L-ps0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from point size using draw
 *  mode inline arrays.
 *
 * @tc L-ps0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from point size using draw
 *  mode inline buffers.
 *  WARNING: This test is non-hermetic as SET_POINT_SIZE does not set the v6 input to the vertex shader!
 *
 * @tc L-s0.1_0.0_1.0_1.0-da
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from specular using draw mode
 *  draw arrays.
 *
 * @tc L-s0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from specular using draw mode
 *  inline elements.
 *
 * @tc L-s0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from specular using draw mode
 *  inline arrays.
 *
 * @tc L-s0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from specular using draw mode
 *  inline buffers.
 *
 * @tc L-t00.1_0.0_1.0_1.0-da
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex0 using draw mode draw
 *  arrays.
 *
 * @tc L-t00.2_0.0_0.6_1.0-ie
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex0 using draw mode
 *  inline elements.
 *
 * @tc L-t00.5_0.0_0.0_1.0-ia
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex0 using draw mode
 *  inline arrays.
 *
 * @tc L-t00.8_0.0_0.0_1.0-ib
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex0 using draw mode
 *  inline buffers.
 *
 * @tc L-t10.1_0.0_1.0_1.0-da
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex1 using draw mode draw
 *  arrays.
 *
 * @tc L-t10.2_0.0_0.6_1.0-ie
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex1 using draw mode
 *  inline elements.
 *
 * @tc L-t10.5_0.0_0.0_1.0-ia
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex1 using draw mode
 *  inline arrays.
 *
 * @tc L-t10.8_0.0_0.0_1.0-ib
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex1 using draw mode
 *  inline buffers.
 *
 * @tc L-t20.1_0.0_1.0_1.0-da
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex2 using draw mode draw
 *  arrays.
 *
 * @tc L-t20.2_0.0_0.6_1.0-ie
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex2 using draw mode
 *  inline elements.
 *
 * @tc L-t20.5_0.0_0.0_1.0-ia
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex2 using draw mode
 *  inline arrays.
 *
 * @tc L-t20.8_0.0_0.0_1.0-ib
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex2 using draw mode
 *  inline buffers.
 *
 * @tc L-t30.1_0.0_1.0_1.0-da
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex3 using draw mode draw
 *  arrays.
 *
 * @tc L-t30.2_0.0_0.6_1.0-ie
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex3 using draw mode
 *  inline elements.
 *
 * @tc L-t30.5_0.0_0.0_1.0-ia
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex3 using draw mode
 *  inline arrays.
 *
 * @tc L-t30.8_0.0_0.0_1.0-ib
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from tex3 using draw mode
 *  inline buffers.
 *
 * @tc L-w0.1_0.0_1.0_1.0-da
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from weight using draw mode
 *  draw arrays.
 *
 * @tc L-w0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from weight using draw mode
 *  inline elements.
 *
 * @tc L-w0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from weight using draw mode
 *  inline arrays.
 *
 * @tc L-w0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified line, then renders a test line that takes its diffuse color from weight using draw mode
 *  inline buffers.
 *
 * @tc T-bd0.1_0.0_1.0_1.0-da
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from back diffuse using draw
 *  mode draw arrays.
 *
 * @tc T-bd0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from back diffuse using draw
 *  mode inline elements.
 *
 * @tc T-bd0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from back diffuse using draw
 *  mode inline arrays.
 *
 * @tc T-bd0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from back diffuse using draw
 *  mode inline buffers.
 *
 * @tc T-bs0.1_0.0_1.0_1.0-da
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from back specular using draw
 *  mode draw arrays.
 *
 * @tc T-bs0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from back specular using draw
 *  mode inline elements.
 *
 * @tc T-bs0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from back specular using using
 *  draw mode inline arrays.
 *
 * @tc T-bs0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from back specular using draw
 *  mode inline buffers.
 *
 * @tc T-d0.1_0.0_1.0_1.0-da
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from diffuse using draw mode
 *  draw arrays.
 *
 * @tc T-d0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from diffuse using draw mode
 *  inline elements.
 *
 * @tc T-d0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from diffuse using draw mode
 *  inline arrays.
 *
 * @tc T-d0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from diffuse using draw mode
 *  inline buffers.
 *
 * @tc T-fc0.1_0.0_1.0_1.0-da
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from fog using draw mode draw
 *  arrays.
 *
 * @tc T-fc0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from fog using draw mode
 *  inline elements.
 *
 * @tc T-fc0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from fog using draw mode
 *  inline arrays.
 *
 * @tc T-fc0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from fog using draw mode
 *  inline buffers.
 *
 * @tc T-n0.1_0.0_1.0_1.0-da
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from normal using draw mode
 *  draw arrays.
 *
 * @tc T-n0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from normal using draw mode
 *  inline elements.
 *
 * @tc T-n0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from normal using draw mode
 *  inline arrays.
 *
 * @tc T-n0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from normal using draw mode
 *  inline buffers.
 *
 * @tc T-ps0.1_0.0_1.0_1.0-da
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from point size using draw
 *  mode draw arrays.
 *
 * @tc T-ps0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from point size using draw
 *  mode inline elements.
 *
 * @tc T-ps0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from point size using draw
 *  mode inline arrays.
 *
 * @tc T-ps0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from point size using draw
 *  mode inline buffers.
 *  WARNING: This test is non-hermetic as SET_POINT_SIZE does not set the v6 input to the vertex shader!
 *
 * @tc T-s0.1_0.0_1.0_1.0-da
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from specular using draw mode
 *  draw arrays.
 *
 * @tc T-s0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from specular using draw mode
 *  inline elements.
 *
 * @tc T-s0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from specular using draw mode
 *  inline arrays.
 *
 * @tc T-s0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from specular using draw mode
 *  inline buffers.
 *
 * @tc T-t00.1_0.0_1.0_1.0-da
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex0 using draw mode draw
 *  arrays.
 *
 * @tc T-t00.2_0.0_0.6_1.0-ie
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex0 using draw mode
 *  inline elements.
 *
 * @tc T-t00.5_0.0_0.0_1.0-ia
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex0 using draw mode
 *  inline arrays.
 *
 * @tc T-t00.8_0.0_0.0_1.0-ib
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex0 using draw mode
 *  inline buffers.
 *
 * @tc T-t10.1_0.0_1.0_1.0-da
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex1 using draw mode draw
 *  arrays.
 *
 * @tc T-t10.2_0.0_0.6_1.0-ie
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex1 using draw mode
 *  inline elements.
 *
 * @tc T-t10.5_0.0_0.0_1.0-ia
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex1 using draw mode
 *  inline arrays.
 *
 * @tc T-t10.8_0.0_0.0_1.0-ib
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex1 using draw mode
 *  inline buffers.
 *
 * @tc T-t20.1_0.0_1.0_1.0-da
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex2 using draw mode draw
 *  arrays.
 *
 * @tc T-t20.2_0.0_0.6_1.0-ie
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex2 using draw mode
 *  inline elements.
 *
 * @tc T-t20.5_0.0_0.0_1.0-ia
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex2 using draw mode
 *  inline arrays.
 *
 * @tc T-t20.8_0.0_0.0_1.0-ib
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex2 using draw mode
 *  inline buffers.
 *
 * @tc T-t30.1_0.0_1.0_1.0-da
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex3 using draw mode draw
 *  arrays.
 *
 * @tc T-t30.2_0.0_0.6_1.0-ie
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex3 using draw mode
 *  inline elements.
 *
 * @tc T-t30.5_0.0_0.0_1.0-ia
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex3 using draw mode
 *  inline arrays.
 *
 * @tc T-t30.8_0.0_0.0_1.0-ib
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from tex3 using draw mode
 *  inline buffers.
 *
 * @tc T-w0.1_0.0_1.0_1.0-da
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from weight using draw mode
 *  draw arrays.
 *
 * @tc T-w0.2_0.0_0.6_1.0-ie
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from weight using draw mode
 *  inline elements.
 *
 * @tc T-w0.5_0.0_0.0_1.0-ia
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from weight using draw mode
 *  inline arrays.
 *
 * @tc T-w0.8_0.0_0.0_1.0-ib
 *  Renders a fully specified tri, then renders a test tri that takes its diffuse color from weight using draw mode
 *  inline buffers.
 *
 */
AttributeCarryoverTests::AttributeCarryoverTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Attrib carryover", config) {
  for (auto primitive : kPrimitives) {
    for (auto attr : kTestAttributes) {
      for (auto config : kTestConfigs) {
        std::string name = MakeTestName(primitive, attr, config);
        tests_[name] = [this, primitive, attr, config]() { Test(primitive, attr, config); };
      }
    }
  }

  tests_[kD3DColorTest] = [this]() { TestD3DColor(false); };
  tests_[kD3DColorStrideZeroTest] = [this]() { TestD3DColor(true); };
  tests_[kSquashedDrawArraysTest] = [this]() { TestSquashedDrawArrays(); };
}

void AttributeCarryoverTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PassthroughVertexShader>();
  shader->SetShader(kShader, sizeof(kShader));

  // Keep in sync with attribute_carryover_tests.vsh

  auto index = 1;
  // #one_constant
  shader->SetUniformF(index++, 1.f, 1.f, 1.f, 1.f);
  // #unsupported_color
  shader->SetUniformF(index++, 0.11f, 0.f, 0.11f, 1.f);

  // #weight_normal_diffuse_specular
  shader->SetUniformF(index++, 0.f, 1.f, 2.f, 3.f);
  // #fog_pts_backdiffuse_backspecular
  shader->SetUniformF(index++, 4.f, 5.f, 6.f, 7.f);
  // #tex0_tex1_tex2_tex3
  shader->SetUniformF(index++, 8.f, 9.f, 10.f, 11.f);

  host_.SetVertexShaderProgram(shader);

  d3d_colors_ = static_cast<uint32_t *>(MmAllocateContiguousMemoryEx(kD3DColorCount * sizeof(uint32_t), 0, MAXRAM,
                                                                     0x1000, PAGE_WRITECOMBINE | PAGE_READWRITE));
}

void AttributeCarryoverTests::Deinitialize() {
  bleed_buffer_.reset();
  test_buffer_.reset();
  if (d3d_colors_) {
    MmFreeContiguousMemory(d3d_colors_);
    d3d_colors_ = nullptr;
  }
  TestSuite::Deinitialize();
}

static void CreateLinePair(TestHost &host, std::shared_ptr<VertexBuffer> &bleed_buffer,
                           std::shared_ptr<VertexBuffer> &test_buffer, std::vector<uint32_t> &index_buffer) {
  auto fb_width = static_cast<float>(host.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host.GetFramebufferHeight());

  const float left = floorf(fb_width / 5.0f);
  const float right = left + (fb_width - left * 2.0f);
  const float top = floorf(fb_height / 6.0f);
  const float bottom = top + (fb_height - top * 2.0f);
  const float z = 3.0f;

  {
    bleed_buffer = host.AllocateVertexBuffer(2);

    auto vertex = bleed_buffer->Lock();
    vertex->SetPosition(left, top, z);

    // Flag the vertex used in inline-element mode to all green (and a reddish weight/fog/pts).
    vertex->SetWeight(0.66f);
    vertex->SetFogCoord(0.5f);
    vertex->SetPointSize(0.7f);
    vertex->SetNormal(0.0f, 1.0f, 0.0f);
    vertex->SetDiffuse(0.0f, 1.0f, 0.0f, 1.0f);
    vertex->SetSpecular(0.0f, 1.0f, 0.0f, 1.0f);
    vertex->SetBackDiffuse(0.0f, 1.0f, 0.0f, 1.0f);
    vertex->SetBackSpecular(0.0f, 1.0f, 0.0f, 1.0f);
    vertex->SetTexCoord0(0.0f, 1.0f);
    vertex->SetTexCoord1(0.0f, 1.0f);
    vertex->SetTexCoord2(0.0f, 1.0f);
    vertex->SetTexCoord3(0.0f, 1.0f);
    ++vertex;

    // The last vertex will have its attributes set by the test code, set a default diffuse to magenta so it's clear
    // that the original diffuse value is not passed through.
    vertex->SetPosition(right, top, z);
    vertex->SetDiffuse(1.0f, 0.0f, 1.0f, 1.0f);
    ++vertex;
    bleed_buffer->Unlock();
  }

  // Create a pair of test vertices which only provide a position.
  {
    test_buffer = host.AllocateVertexBuffer(2);
    auto vertex = test_buffer->Lock();
    vertex->SetPosition(left, bottom, z);
    ++vertex;
    vertex->SetPosition(right, bottom, z);
    ++vertex;
    test_buffer->Unlock();
  }

  index_buffer.clear();
  index_buffer.push_back(1);  // Intentionally specify the vertices out of order.
  index_buffer.push_back(0);
}

static void CreateTrianglePair(TestHost &host, std::shared_ptr<VertexBuffer> &bleed_buffer,
                               std::shared_ptr<VertexBuffer> &test_buffer, std::vector<uint32_t> &index_buffer) {
  auto fb_width = static_cast<float>(host.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host.GetFramebufferHeight());

  const float left = floorf(fb_width / 5.0f);
  const float right = left + (fb_width - left * 2.0f);
  const float top = floorf(fb_height / 6.0f);
  const float bottom = top + (fb_height - top * 2.0f);
  const float mid_width = left + (right - left) * 0.5f;
  const float z = 3.0f;

  {
    bleed_buffer = host.AllocateVertexBuffer(3);
    float one_pos[3] = {left, top, z};
    float two_pos[3] = {mid_width, top, z};
    float three_pos[3] = {left + (mid_width - left) * 0.5f, bottom, z};
    bleed_buffer->DefineTriangle(0, one_pos, two_pos, three_pos);
    auto vertex = bleed_buffer->Lock();

    vertex[0].SetWeight(1.f);
    vertex[0].SetFogCoord(1.f);
    vertex[0].SetPointSize(1.f);
    vertex[0].SetNormal(1.0f, 1.0f, 1.0f);
    vertex[0].SetDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[0].SetSpecular(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[0].SetBackDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[0].SetBackSpecular(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[0].SetTexCoord0(1.0f, 1.0f);
    vertex[0].SetTexCoord1(1.0f, 1.0f);
    vertex[0].SetTexCoord2(1.0f, 1.0f);
    vertex[0].SetTexCoord3(1.0f, 1.0f);

    // Flag the vertex used in inline-element mode to all green (and a reddish weight).
    vertex[1].SetWeight(0.66f);
    vertex[1].SetFogCoord(0.5f);
    vertex[1].SetPointSize(0.7f);
    vertex[1].SetNormal(0.0f, 1.0f, 0.0f);
    vertex[1].SetDiffuse(0.0f, 1.0f, 0.0f, 1.0f);
    vertex[1].SetSpecular(0.0f, 1.0f, 0.0f, 1.0f);
    vertex[1].SetBackDiffuse(0.0f, 1.0f, 0.0f, 1.0f);
    vertex[1].SetBackSpecular(0.0f, 1.0f, 0.0f, 1.0f);
    vertex[1].SetTexCoord0(0.0f, 1.0f);
    vertex[1].SetTexCoord1(0.0f, 1.0f);
    vertex[1].SetTexCoord2(0.0f, 1.0f);
    vertex[1].SetTexCoord3(0.0f, 1.0f);

    // The last vertex will have its attributes set by the test code, set a default diffuse to magenta so it's clear
    // that the original diffuse value is not passed through.
    vertex[2].SetDiffuse(1.0f, 0.0f, 1.0f, 1.0f);
    bleed_buffer->Unlock();
  }

  {
    test_buffer = host.AllocateVertexBuffer(3);
    float one_pos[3] = {mid_width, top, z};
    float two_pos[3] = {right, top, z};
    float three_pos[3] = {mid_width + (right - mid_width) * 0.5f, bottom, z};
    test_buffer->DefineTriangle(0, one_pos, two_pos, three_pos);
  }

  index_buffer.clear();
  index_buffer.push_back(2);  // Intentionally specify the vertices out of order.
  index_buffer.push_back(0);
  index_buffer.push_back(1);
}

void AttributeCarryoverTests::CreateGeometry(TestHost::DrawPrimitive primitive) {
  bleed_buffer_.reset();
  test_buffer_.reset();

  if (primitive == TestHost::PRIMITIVE_LINES) {
    CreateLinePair(host_, bleed_buffer_, test_buffer_, index_buffer_);
  } else {
    CreateTrianglePair(host_, bleed_buffer_, test_buffer_, index_buffer_);
  }
}

void AttributeCarryoverTests::Test(TestHost::DrawPrimitive primitive, Attribute test_attribute,
                                   const TestConfig &config) {
  auto shader = host_.GetShaderProgram();

  static constexpr uint32_t kBackgroundColor = 0xFF444444;
  CreateGeometry(primitive);
  host_.PrepareDraw(kBackgroundColor);

  auto draw = [this, config, primitive](uint32_t additional_fields) {
    uint32_t attributes = host_.POSITION | additional_fields;
    switch (config.draw_mode) {
      case DRAW_ARRAYS:
        host_.DrawArrays(attributes, primitive);
        break;

      case DRAW_INLINE_BUFFERS:
        host_.DrawInlineBuffer(attributes, primitive);
        break;

      case DRAW_INLINE_ELEMENTS:
        host_.DrawInlineElements16(index_buffer_, attributes, primitive);
        break;

      case DRAW_INLINE_ARRAYS:
        host_.DrawInlineArray(attributes, primitive);
        break;
    }
  };

  // TODO: Set the attribute under test.
  auto vertex = bleed_buffer_->Lock();
  switch (primitive) {
    case TestHost::PRIMITIVE_LINES:
      // Set the second vertex in the line.
      ++vertex;
      break;

    case TestHost::PRIMITIVE_TRIANGLES:
      // Set the last vertex in the triangle.
      vertex += 2;
      break;

    default:
      ASSERT(!"TODO: Implement additional primitives.");
      break;
  }

  switch (test_attribute) {
    case ATTR_WEIGHT:
      vertex->SetWeight(config.attribute_value[0]);
      break;
    case ATTR_NORMAL:
      vertex->SetNormal(config.attribute_value);
      break;
    case ATTR_DIFFUSE:
      vertex->SetDiffuse(config.attribute_value);
      break;
    case ATTR_SPECULAR:
      vertex->SetSpecular(config.attribute_value);
      break;
    case ATTR_FOG_COORD:
      vertex->SetFogCoord(config.attribute_value[0]);
      break;
    case ATTR_POINT_SIZE:
      vertex->SetPointSize(config.attribute_value[0]);
      break;
    case ATTR_BACK_DIFFUSE:
      vertex->SetBackDiffuse(config.attribute_value);
      break;
    case ATTR_BACK_SPECULAR:
      vertex->SetBackSpecular(config.attribute_value);
      break;
    case ATTR_TEX0:
      vertex->SetTexCoord0(config.attribute_value);
      break;
    case ATTR_TEX1:
      vertex->SetTexCoord1(config.attribute_value);
      break;
    case ATTR_TEX2:
      vertex->SetTexCoord2(config.attribute_value);
      break;
    case ATTR_TEX3:
      vertex->SetTexCoord3(config.attribute_value);
      break;
  }
  bleed_buffer_->Unlock();

  // The bleed buffer should always render its diffuse color, regardless of the attribute under test.
  shader->SetUniformF(0, ATTR_DIFFUSE);
  host_.SetVertexBuffer(bleed_buffer_);
  draw(host_.DIFFUSE | TestAttributeToVertexAttribute(test_attribute));

  // The test buffer should render the attribute under test as its diffuse color.
  shader->SetUniformF(0, test_attribute);
  host_.SetVertexBuffer(test_buffer_);
  draw(0);

  std::string name = MakeTestName(primitive, test_attribute, config);
  pb_print("%s", name.c_str());
  pb_draw_text_screen();

  FinishDraw(name);
}

void AttributeCarryoverTests::TestD3DColor(bool zero_stride) {
  auto shader = host_.GetShaderProgram();

  static constexpr uint32_t kBackgroundColor = 0xFF444444;
  CreateGeometry(TestHost::PRIMITIVE_TRIANGLES);

  // Both draws render iDiffuse. This is set before PrepareDraw because the raw draws below do not upload shader
  // constants.
  shader->SetUniformF(0, ATTR_DIFFUSE);
  host_.PrepareDraw(kBackgroundColor);

  if (zero_stride) {
    d3d_colors_[0] = kD3DTestColor;
    d3d_colors_[1] = kD3DLeadingColor;
    d3d_colors_[2] = kD3DLeadingColor;
  } else {
    d3d_colors_[0] = kD3DLeadingColor;
    d3d_colors_[1] = kD3DLeadingColor;
    d3d_colors_[2] = kD3DTestColor;
  }

  // Start from a known state where only position and diffuse are bound.
  for (uint32_t i = 0; i < 16; ++i) {
    ClearRawVertexAttribute(i);
  }

  // Bleed triangle: position from the float vertex buffer, diffuse from the UB_D3D array.
  {
    auto vertex = bleed_buffer_->Lock();
    SetRawVertexAttribute(NV2A_VERTEX_ATTR_POSITION, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 3, sizeof(Vertex),
                          vertex[0].pos);
    bleed_buffer_->Unlock();
  }
  SetRawVertexAttribute(NV2A_VERTEX_ATTR_DIFFUSE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_D3D, 4,
                        zero_stride ? 0 : sizeof(uint32_t), d3d_colors_);
  DrawRawArrays(TestHost::PRIMITIVE_TRIANGLES, 3);

  // Test triangle: diffuse is disabled so the shader reads the carried value.
  ClearRawVertexAttribute(NV2A_VERTEX_ATTR_DIFFUSE);
  {
    auto vertex = test_buffer_->Lock();
    SetRawVertexAttribute(NV2A_VERTEX_ATTR_POSITION, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 3, sizeof(Vertex),
                          vertex[0].pos);
    test_buffer_->Unlock();
  }
  DrawRawArrays(TestHost::PRIMITIVE_TRIANGLES, 3);

  ClearRawVertexAttribute(NV2A_VERTEX_ATTR_POSITION);

  const char *name = zero_stride ? kD3DColorStrideZeroTest : kD3DColorTest;
  pb_print("%s", name);
  pb_draw_text_screen();

  FinishDraw(name);
}

void AttributeCarryoverTests::TestSquashedDrawArrays() {
  auto shader = host_.GetShaderProgram();

  static constexpr uint32_t kBackgroundColor = 0xFF444444;

  const auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  const auto fb_height = static_cast<float>(host_.GetFramebufferHeight());
  const float column_width = floorf(fb_width / 4.0f);
  const float top = floorf(fb_height / 6.0f);
  const float bottom = fb_height - top;
  const float z = 3.0f;

  auto make_triangle = [=](float left, float *one, float *two, float *three) {
    const float right = left + column_width * 0.9f;
    one[0] = left;
    one[1] = top;
    one[2] = z;
    two[0] = right;
    two[1] = top;
    two[2] = z;
    three[0] = left + (right - left) * 0.5f;
    three[1] = bottom;
    three[2] = z;
  };

  const Color kLeadingColor{0.0f, 1.0f, 0.0f, 1.0f};
  const Color kExpectedColor{0.0f, 0.4f, 1.0f, 1.0f};
  const Color kHighestIndexColor{1.0f, 0.2f, 0.0f, 1.0f};

  // Vertices 0-2 are drawn second (middle), vertices 3-5 are drawn first (left).
  bleed_buffer_ = host_.AllocateVertexBuffer(6);
  {
    float one[3], two[3], three[3];
    make_triangle(column_width * 1.5f, one, two, three);
    bleed_buffer_->DefineTriangle(0, one, two, three, kLeadingColor, kLeadingColor, kExpectedColor);
    make_triangle(column_width * 0.5f, one, two, three);
    bleed_buffer_->DefineTriangle(1, one, two, three, kLeadingColor, kLeadingColor, kHighestIndexColor);
  }

  test_buffer_ = host_.AllocateVertexBuffer(3);
  {
    float one[3], two[3], three[3];
    make_triangle(column_width * 2.5f, one, two, three);
    test_buffer_->DefineTriangle(0, one, two, three);
  }

  // Both draws render iDiffuse. This is set before PrepareDraw because the raw draws below do not upload shader
  // constants.
  shader->SetUniformF(0, ATTR_DIFFUSE);
  host_.PrepareDraw(kBackgroundColor);

  for (uint32_t i = 0; i < 16; ++i) {
    ClearRawVertexAttribute(i);
  }

  {
    auto vertex = bleed_buffer_->Lock();
    SetRawVertexAttribute(NV2A_VERTEX_ATTR_POSITION, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 3, sizeof(Vertex),
                          vertex[0].pos);
    SetRawVertexAttribute(NV2A_VERTEX_ATTR_DIFFUSE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, sizeof(Vertex),
                          vertex[0].diffuse);
    bleed_buffer_->Unlock();
  }

  // The two draws must be adjacent in the pushbuffer with no intervening methods.
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_BREAK_VERTEX_BUFFER_CACHE, 0);
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
  Pushbuffer::Push(NV097_DRAW_ARRAYS, MASK(NV097_DRAW_ARRAYS_COUNT, 2) | MASK(NV097_DRAW_ARRAYS_START_INDEX, 3));
  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
  Pushbuffer::Push(NV097_DRAW_ARRAYS, MASK(NV097_DRAW_ARRAYS_COUNT, 2) | MASK(NV097_DRAW_ARRAYS_START_INDEX, 0));
  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  Pushbuffer::End();

  // Test triangle: diffuse is disabled so the shader reads the carried value.
  ClearRawVertexAttribute(NV2A_VERTEX_ATTR_DIFFUSE);
  {
    auto vertex = test_buffer_->Lock();
    SetRawVertexAttribute(NV2A_VERTEX_ATTR_POSITION, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 3, sizeof(Vertex),
                          vertex[0].pos);
    test_buffer_->Unlock();
  }
  DrawRawArrays(TestHost::PRIMITIVE_TRIANGLES, 3);

  ClearRawVertexAttribute(NV2A_VERTEX_ATTR_POSITION);

  pb_print("%s", kSquashedDrawArraysTest);
  pb_draw_text_screen();

  FinishDraw(kSquashedDrawArraysTest);
}

std::string AttributeCarryoverTests::MakeTestName(TestHost::DrawPrimitive primitive, Attribute test_attribute,
                                                  const TestConfig &config) {
  std::string ret;

  switch (primitive) {
    case TestHost::PRIMITIVE_TRIANGLES:
      ret = "T";
      break;

    case TestHost::PRIMITIVE_LINES:
      ret = "L";
      break;

    default:
      ASSERT(!"Unsupported primitive");
  }

  switch (test_attribute) {
    case ATTR_WEIGHT:
      ret += "-w";
      break;
    case ATTR_NORMAL:
      ret += "-n";
      break;
    case ATTR_DIFFUSE:
      ret += "-d";
      break;
    case ATTR_SPECULAR:
      ret += "-s";
      break;
    case ATTR_FOG_COORD:
      ret += "-fc";
      break;
    case ATTR_POINT_SIZE:
      ret += "-ps";
      break;
    case ATTR_BACK_DIFFUSE:
      ret += "-bd";
      break;
    case ATTR_BACK_SPECULAR:
      ret += "-bs";
      break;
    case ATTR_TEX0:
      ret += "-t0";
      break;
    case ATTR_TEX1:
      ret += "-t1";
      break;
    case ATTR_TEX2:
      ret += "-t2";
      break;
    case ATTR_TEX3:
      ret += "-t3";
      break;
  }

  char buf[32] = {0};
  snprintf(buf, 31, "%.1f_%.1f_%.1f_%.1f", config.attribute_value[0], config.attribute_value[1],
           config.attribute_value[2], config.attribute_value[3]);
  ret += buf;

  ret += "-";
  ret += DrawModeLabel(config.draw_mode);

  return ret;
}

std::string AttributeCarryoverTests::DrawModeLabel(DrawMode mode) {
  switch (mode) {
    case DRAW_ARRAYS:
      return "da";
    case DRAW_INLINE_BUFFERS:
      return "ib";
    case DRAW_INLINE_ARRAYS:
      return "ia";
    case DRAW_INLINE_ELEMENTS:
      return "ie";
  }
}

static TestHost::VertexAttribute TestAttributeToVertexAttribute(AttributeCarryoverTests::Attribute attribute) {
  switch (attribute) {
    case AttributeCarryoverTests::ATTR_WEIGHT:
      return TestHost::WEIGHT;
    case AttributeCarryoverTests::ATTR_NORMAL:
      return TestHost::NORMAL;
    case AttributeCarryoverTests::ATTR_DIFFUSE:
      return TestHost::DIFFUSE;
    case AttributeCarryoverTests::ATTR_SPECULAR:
      return TestHost::SPECULAR;
    case AttributeCarryoverTests::ATTR_FOG_COORD:
      return TestHost::FOG_COORD;
    case AttributeCarryoverTests::ATTR_POINT_SIZE:
      return TestHost::POINT_SIZE;
    case AttributeCarryoverTests::ATTR_BACK_DIFFUSE:
      return TestHost::BACK_DIFFUSE;
    case AttributeCarryoverTests::ATTR_BACK_SPECULAR:
      return TestHost::BACK_SPECULAR;
    case AttributeCarryoverTests::ATTR_TEX0:
      return TestHost::TEXCOORD0;
    case AttributeCarryoverTests::ATTR_TEX1:
      return TestHost::TEXCOORD1;
    case AttributeCarryoverTests::ATTR_TEX2:
      return TestHost::TEXCOORD2;
    case AttributeCarryoverTests::ATTR_TEX3:
      return TestHost::TEXCOORD3;
  }
}
