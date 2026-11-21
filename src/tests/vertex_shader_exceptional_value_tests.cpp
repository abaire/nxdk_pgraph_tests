#include "vertex_shader_exceptional_value_tests.h"

#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/vertex_shader_program.h"
#include "test_host.h"

static const uint32_t kArlShader[] = {
#include "vertex_shader_exceptional/arl.vshinc"
};
static const uint32_t kColorShader[] = {
#include "vertex_shader_exceptional/color.vshinc"
};
static const uint32_t kMacSgeShader[] = {
#include "vertex_shader_exceptional/mac_sge.vshinc"
};
static const uint32_t kRefShader[] = {
#include "vertex_shader_exceptional/ref.vshinc"
};

// Constant slots used by the shaders.
static constexpr uint32_t kSlotA = 96;
static constexpr uint32_t kSlotB = 97;
static constexpr uint32_t kSlotC = 98;
static constexpr uint32_t kSlotExpected = 100;
static constexpr uint32_t kSlotLaneSelect = 102;
static constexpr uint32_t kSlotGreenMinusRed = 103;
static constexpr uint32_t kSlotRed = 104;

static constexpr uint32_t kZero = 0x00000000;
static constexpr uint32_t kOne = 0x3F800000;
static constexpr uint32_t kNegOne = 0xBF800000;
static constexpr uint32_t kNaN = 0x7FFFFFFF;
static constexpr uint32_t kNegZero = 0x80000000;
static constexpr uint32_t kDenormal = 0x00400000;
static constexpr uint32_t kQuarter = 0x3E800000;
static constexpr uint32_t kHalf = 0x3F000000;
static constexpr uint32_t kThreeQuarters = 0x3F400000;

// On-screen layout, in text grid coordinates.
static constexpr uint32_t kHeaderRow = 0;
static constexpr uint32_t kFirstRecordRow = 1;
static constexpr uint32_t kComparatorRow = 15;
static constexpr uint32_t kMaxRecords = kComparatorRow - kFirstRecordRow;
static constexpr uint32_t kCellHw = 22;
static constexpr uint32_t kCellRef = 25;
static constexpr uint32_t kCellArlRead = 22;
static constexpr uint32_t kCellArlHw = 25;
static constexpr uint32_t kArlNoteColumn = 28;

// Number of constants seeded with distinct colors for the ARL test.
static constexpr uint32_t kArlMarkerCount = 16;
static constexpr uint32_t kArlLastConstant = 191;

static void SetConstant(uint32_t slot, uint32_t x, uint32_t y, uint32_t z, uint32_t w) {
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_TRANSFORM_CONSTANT_LOAD, slot);
  Pushbuffer::Push(NV097_SET_TRANSFORM_CONSTANT, x, y, z, w);
  Pushbuffer::End();
}

static void SetConstant(uint32_t slot, const uint32_t (&v)[4]) { SetConstant(slot, v[0], v[1], v[2], v[3]); }

//! Color of the ARL marker in c[index].
static void ArlMarker(uint32_t index, uint32_t (&out)[4]) {
  static constexpr uint32_t kLevels[] = {kQuarter, kHalf, kThreeQuarters, kOne};
  out[0] = kLevels[index & 3];
  out[1] = kLevels[(index >> 2) & 3];
  out[2] = kOne;
  out[3] = kOne;
}

void VshExceptionalValueRenderer::Initialize() {
  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
}

void VshExceptionalValueRenderer::SetShader(const uint32_t *shader, uint32_t size) {
  host_.SetVertexShaderProgram(std::make_shared<VertexShaderProgram>(shader, size));
}

//! Sets the one-hot selector of the result lane that the epilogue shows.
static void SetLaneSelect(uint8_t lane) {
  SetConstant(kSlotLaneSelect, lane == 0 ? kOne : kZero, lane == 1 ? kOne : kZero, lane == 2 ? kOne : kZero,
              lane == 3 ? kOne : kZero);
}

static void SetExpected(uint32_t bits) { SetConstant(kSlotExpected, bits, bits, bits, bits); }

//! Draws a cell quad at the given text grid position, optionally supplying raw bits for v1 and v9 (texcoord 0).
static void DrawCell(TestHost &host, uint32_t row, uint32_t col, const uint32_t *v1, const uint32_t *v9) {
  const float left = static_cast<float>(20 + 10 * col);
  const float right = left + 18.f;
  const float top = static_cast<float>(25 + 25 * row);
  const float bottom = top + 16.f;

  auto vertex = [&host, v1, v9](float x, float y) {
    Pushbuffer::Begin();
    if (v1) {
      Pushbuffer::Push(NV097_SET_VERTEX_DATA4F_M + 16 * NV2A_VERTEX_ATTR_WEIGHT, v1[0], v1[1], v1[2], v1[3]);
    }
    if (v9) {
      Pushbuffer::Push(NV097_SET_TEXCOORD0_4F, v9[0], v9[1], v9[2], v9[3]);
    }
    Pushbuffer::End();
    host.SetVertex(x, y, 0.1f, 1.f);
  };

  host.Begin(TestHost::PRIMITIVE_QUADS);
  vertex(left, top);
  vertex(right, top);
  vertex(right, bottom);
  vertex(left, bottom);
  host.End();
}

static void SetVerdictColors() {
  SetConstant(kSlotGreenMinusRed, kNegOne, kOne, kZero, kZero);
  SetConstant(kSlotRed, kOne, kZero, kZero, kOne);
}

void VshExceptionalValueRenderer::DrawComparatorRow() {
  // Each cell checks the SGE result against 1.0, so green means SGE returned 1.
  SetShader(kMacSgeShader, sizeof(kMacSgeShader));
  SetVerdictColors();
  SetLaneSelect(0);
  SetExpected(kOne);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);

  struct Comparison {
    uint32_t a;
    uint32_t b;
    uint32_t col;
  };
  static constexpr Comparison kComparisons[] = {
      {kNaN, kNaN, 13},
      {kNegZero, kZero, 22},
      {kZero, kDenormal, 31},
  };
  for (auto &cmp : kComparisons) {
    SetConstant(kSlotA, cmp.a, cmp.a, cmp.a, cmp.a);
    SetConstant(kSlotB, cmp.b, cmp.b, cmp.b, cmp.b);
    DrawCell(host_, kComparatorRow, cmp.col, nullptr, nullptr);
  }

  pb_printat(kComparatorRow, 0, "SGE");
  pb_printat(kComparatorRow, 5, "NaN,NaN");
  pb_printat(kComparatorRow, 16, "-0,+0");
  pb_printat(kComparatorRow, 25, "+0,den");
}

void VshExceptionalValueRenderer::Draw(const VshExcTestCase &test_case) {
  ASSERT(test_case.count <= kMaxRecords && "Too many records for one test");
  host_.PrepareDraw(0xFF303030);

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  SetVerdictColors();

  for (uint32_t i = 0; i < test_case.count; ++i) {
    const VshExcRecord &record = test_case.records[i];
    const uint32_t row = kFirstRecordRow + i;

    SetConstant(kSlotA, record.a);
    SetConstant(kSlotB, record.b);
    SetConstant(kSlotC, record.c);
    SetLaneSelect(record.lane);

    SetShader(test_case.shader, test_case.shader_size);
    SetExpected(record.hw);
    DrawCell(host_, row, kCellHw, record.a, nullptr);

    SetShader(kRefShader, sizeof(kRefShader));
    SetExpected(record.hw);
    DrawCell(host_, row, kCellRef, nullptr, nullptr);

    pb_printat(row, 0, "%s", record.label);
  }

  DrawComparatorRow();

  pb_printat(kHeaderRow, 0, "%s", test_case.test_name);
  pb_printat(kHeaderRow, kCellHw, "hw");
  pb_printat(kHeaderRow, kCellRef, "rf");
  pb_draw_text_screen();
}

void VshExceptionalValueRenderer::Draw(const VshExcArlTestCase &test_case) {
  ASSERT(test_case.count <= kMaxRecords && "Too many records for one test");
  host_.PrepareDraw(0xFF303030);

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  auto arl_program = std::make_shared<VertexShaderProgram>(kArlShader, sizeof(kArlShader));
  auto color_program = std::make_shared<VertexShaderProgram>(kColorShader, sizeof(kColorShader));

  // c[0] to c[15] are normally used by the fixed function pipeline and are restored at the end of the test.
  for (uint32_t index = 0; index < kArlMarkerCount; ++index) {
    uint32_t marker[4];
    ArlMarker(index, marker);
    SetConstant(index, marker);
  }
  SetConstant(kArlLastConstant, kOne, kZero, kOne, kOne);

  for (uint32_t i = 0; i < test_case.count; ++i) {
    const VshExcArlRecord &record = test_case.records[i];
    const uint32_t row = kFirstRecordRow + i;

    host_.SetVertexShaderProgram(arl_program);
    DrawCell(host_, row, kCellArlRead, nullptr, record.a);

    host_.SetVertexShaderProgram(color_program);
    if (record.reads_zero) {
      SetConstant(kSlotA, kZero, kZero, kZero, kZero);
      pb_printat(row, kArlNoteColumn, "zero");
    } else {
      uint32_t marker[4];
      ArlMarker(record.index, marker);
      SetConstant(kSlotA, marker);
      pb_printat(row, kArlNoteColumn, "c[%u]", record.index);
    }
    DrawCell(host_, row, kCellArlHw, nullptr, nullptr);

    pb_printat(row, 0, "%s", record.label);
  }

  pb_printat(kHeaderRow, 0, "%s", test_case.test_name);
  pb_printat(kHeaderRow, kCellArlRead, "rd");
  pb_printat(kHeaderRow, kCellArlHw, "hw");
  pb_printat(kHeaderRow, kArlNoteColumn, "hw reads");
  pb_draw_text_screen();

  host_.SetVertexShaderProgram(nullptr);
  host_.SetDefaultViewportAndFixedFunctionMatrices();
}

void VertexShaderExceptionalMACTests::Initialize() {
  TestSuite::Initialize();
  renderer_.Initialize();
}

void VertexShaderExceptionalMOVTests::Initialize() {
  TestSuite::Initialize();
  renderer_.Initialize();
}

void VertexShaderExceptionalILUTests::Initialize() {
  TestSuite::Initialize();
  renderer_.Initialize();
}

void VertexShaderExceptionalPathsTests::Initialize() {
  TestSuite::Initialize();
  renderer_.Initialize();
}
