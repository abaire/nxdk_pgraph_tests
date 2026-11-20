#include "fog_eye_space_w_tests.h"

#include <pbkit/pbkit.h>

#include <string>
#include <utility>

#include "test_host.h"
#include "xbox_math_matrix.h"

static constexpr uint32_t kFogColor = 0xFF64B4D2;
static constexpr vector_t kDiffuse{0.15f, 0.15f, 0.2f, 1.f};

static constexpr float kNearEyeZ = 3.f;
static constexpr float kFarEyeZ = 40.f;

static constexpr float kLinearFogEnd = 60.f;
static constexpr float kExpDensity = 0.03f;
static constexpr float kExp2Density = 0.04f;

static constexpr uint32_t kFogModes[] = {
    NV097_SET_FOG_MODE_V_LINEAR,
    NV097_SET_FOG_MODE_V_EXP,
    NV097_SET_FOG_MODE_V_EXP2,
};

static constexpr uint32_t kFogGenModes[] = {
    NV097_SET_FOG_GEN_MODE_V_PLANAR,
    NV097_SET_FOG_GEN_MODE_V_ABS_PLANAR,
    NV097_SET_FOG_GEN_MODE_V_RADIAL,
};

struct LabeledValue {
  float value;
  const char *label;
  //! MV_ZERO_W only: also zero the xyz columns so eye xyz = 0.
  bool zero_xyz = false;
};

static constexpr LabeledValue kScales[] = {
    {1.f, "k 1"}, {0.5f, "k 0.5"}, {2.f, "k 2"}, {18.f, "k 18"}, {-1.f, "k -1"},
};

// Negative w is avoided for vertex positions since it would be clipped.
static constexpr LabeledValue kVertexWs[] = {
    {1.f, "w 1"},
    {0.5f, "w 0.5"},
    {2.f, "w 2"},
    {18.f, "w 18"},
};

static constexpr LabeledValue kProjectiveFactors[] = {
    {0.f, "a 0"},
    {0.02f, "a 0.02"},
    {0.05f, "a 0.05"},
    {-0.02f, "a -0.02"},
};

// The default model view w column is already (0, 0, 0, 1), so w 1 is the reference.
static constexpr LabeledValue kZeroWs[] = {
    {1.f, "w 1"}, {0.f, "w 0"}, {-0.f, "w -0"}, {1e-6f, "w 1e-6"}, {-1e-6f, "w -1e-6"}, {0.f, "w 0 xyz 0", true},
};

static std::string VariantName(FogEyeSpaceWTests::Variant variant) {
  switch (variant) {
    case FogEyeSpaceWTests::Variant::MV_SCALE:
      return "MVScale";
    case FogEyeSpaceWTests::Variant::VERTEX_W:
      return "VertexW";
    case FogEyeSpaceWTests::Variant::MV_PROJECTIVE:
      return "MVProjective";
    case FogEyeSpaceWTests::Variant::MV_ZERO_W:
      return "MVZeroW";
  }
  return "<UNKNOWN>";
}

static std::string FogGenModeName(uint32_t gen_mode) {
  switch (gen_mode) {
    case NV097_SET_FOG_GEN_MODE_V_PLANAR:
      return "planar";
    case NV097_SET_FOG_GEN_MODE_V_ABS_PLANAR:
      return "abs_planar";
    case NV097_SET_FOG_GEN_MODE_V_RADIAL:
      return "radial";
    default:
      return "<UNKNOWN>";
  }
}

static std::string FogModeName(uint32_t fog_mode) {
  switch (fog_mode) {
    case NV097_SET_FOG_MODE_V_LINEAR:
      return "lin";
    case NV097_SET_FOG_MODE_V_EXP:
      return "exp";
    case NV097_SET_FOG_MODE_V_EXP2:
      return "exp2";
    default:
      return "<UNKNOWN>";
  }
}

/**
 * Initializes the test suite and creates test cases.
 *
 * The hardware dehomogenizes the eye-space position before fog generation, so in the MVScale and VertexW tests every
 * strip matches the reference (first) strip.
 *
 * @tc MVScale-planar-lin
 *  Linear planar fog. The model view matrix is scaled by k (eye w = k) for each strip; k = 1 is the reference. All
 * strips are identical to the reference.
 *
 * @tc MVScale-planar-exp
 *  Exponential planar fog. The model view matrix is scaled by k (eye w = k) for each strip; k = 1 is the reference. All
 * strips are identical to the reference.
 *
 * @tc MVScale-planar-exp2
 *  Exponential squared planar fog. The model view matrix is scaled by k (eye w = k) for each strip; k = 1 is the
 * reference. All strips are identical to the reference.
 *
 * @tc MVScale-abs_planar-lin
 *  Linear absolute planar fog. The model view matrix is scaled by k (eye w = k) for each strip; k = 1 is the reference.
 * All strips are identical to the reference.
 *
 * @tc MVScale-abs_planar-exp
 *  Exponential absolute planar fog. The model view matrix is scaled by k (eye w = k) for each strip; k = 1 is the
 * reference. All strips are identical to the reference.
 *
 * @tc MVScale-abs_planar-exp2
 *  Exponential squared absolute planar fog. The model view matrix is scaled by k (eye w = k) for each strip; k = 1 is
 * the reference. All strips are identical to the reference.
 *
 * @tc MVScale-radial-lin
 *  Linear radial fog. The model view matrix is scaled by k (eye w = k) for each strip; k = 1 is the reference. All
 * strips are identical to the reference.
 *
 * @tc MVScale-radial-exp
 *  Exponential radial fog. The model view matrix is scaled by k (eye w = k) for each strip; k = 1 is the reference. All
 * strips are identical to the reference.
 *
 * @tc MVScale-radial-exp2
 *  Exponential squared radial fog. The model view matrix is scaled by k (eye w = k) for each strip; k = 1 is the
 * reference. All strips are identical to the reference.
 *
 * @tc VertexW-planar-lin
 *  Linear planar fog. Vertices are submitted as w * (x, y, z, 1) with default matrices; w = 1 is the reference. All
 * strips are identical to the reference.
 *
 * @tc VertexW-planar-exp
 *  Exponential planar fog. Vertices are submitted as w * (x, y, z, 1) with default matrices; w = 1 is the reference.
 * All strips are identical to the reference.
 *
 * @tc VertexW-planar-exp2
 *  Exponential squared planar fog. Vertices are submitted as w * (x, y, z, 1) with default matrices; w = 1 is the
 * reference. All strips are identical to the reference.
 *
 * @tc VertexW-abs_planar-lin
 *  Linear absolute planar fog. Vertices are submitted as w * (x, y, z, 1) with default matrices; w = 1 is the
 * reference. All strips are identical to the reference.
 *
 * @tc VertexW-abs_planar-exp
 *  Exponential absolute planar fog. Vertices are submitted as w * (x, y, z, 1) with default matrices; w = 1 is the
 * reference. All strips are identical to the reference.
 *
 * @tc VertexW-abs_planar-exp2
 *  Exponential squared absolute planar fog. Vertices are submitted as w * (x, y, z, 1) with default matrices; w = 1 is
 * the reference. All strips are identical to the reference.
 *
 * @tc VertexW-radial-lin
 *  Linear radial fog. Vertices are submitted as w * (x, y, z, 1) with default matrices; w = 1 is the reference. All
 * strips are identical to the reference.
 *
 * @tc VertexW-radial-exp
 *  Exponential radial fog. Vertices are submitted as w * (x, y, z, 1) with default matrices; w = 1 is the reference.
 * All strips are identical to the reference.
 *
 * @tc VertexW-radial-exp2
 *  Exponential squared radial fog. Vertices are submitted as w * (x, y, z, 1) with default matrices; w = 1 is the
 * reference. All strips are identical to the reference.
 *
 * @tc MVProjective-planar-lin
 *  Linear planar fog. The model view w column is modified so eye w = 1 + a * eye_x while eye xyz are unchanged; a = 0
 * is the reference. The fog distance is divided by eye w, so positive a reduces fog right of the screen center (where
 * eye_x > 0) and slightly increases it left of center; negative a does the opposite.
 *
 * @tc MVProjective-planar-exp
 *  Exponential planar fog. The model view w column is modified so eye w = 1 + a * eye_x while eye xyz are unchanged; a
 * = 0 is the reference. The fog distance is divided by eye w, so positive a reduces fog right of the screen center
 * (where eye_x > 0) and slightly increases it left of center; negative a does the opposite.
 *
 * @tc MVProjective-planar-exp2
 *  Exponential squared planar fog. The model view w column is modified so eye w = 1 + a * eye_x while eye xyz are
 * unchanged; a = 0 is the reference. The fog distance is divided by eye w, so positive a reduces fog right of the
 * screen center (where eye_x > 0) and slightly increases it left of center; negative a does the opposite.
 *
 * @tc MVProjective-abs_planar-lin
 *  Linear absolute planar fog. The model view w column is modified so eye w = 1 + a * eye_x while eye xyz are
 * unchanged; a = 0 is the reference. The fog distance is divided by eye w, so positive a reduces fog right of the
 * screen center (where eye_x > 0) and slightly increases it left of center; negative a does the opposite.
 *
 * @tc MVProjective-abs_planar-exp
 *  Exponential absolute planar fog. The model view w column is modified so eye w = 1 + a * eye_x while eye xyz are
 * unchanged; a = 0 is the reference. The fog distance is divided by eye w, so positive a reduces fog right of the
 * screen center (where eye_x > 0) and slightly increases it left of center; negative a does the opposite.
 *
 * @tc MVProjective-abs_planar-exp2
 *  Exponential squared absolute planar fog. The model view w column is modified so eye w = 1 + a * eye_x while eye xyz
 * are unchanged; a = 0 is the reference. The fog distance is divided by eye w, so positive a reduces fog right of the
 * screen center (where eye_x > 0) and slightly increases it left of center; negative a does the opposite.
 *
 * @tc MVProjective-radial-lin
 *  Linear radial fog. The model view w column is modified so eye w = 1 + a * eye_x while eye xyz are unchanged; a = 0
 * is the reference. The fog distance is divided by eye w, so positive a reduces fog right of the screen center (where
 * eye_x > 0) and slightly increases it left of center; negative a does the opposite.
 *
 * @tc MVProjective-radial-exp
 *  Exponential radial fog. The model view w column is modified so eye w = 1 + a * eye_x while eye xyz are unchanged; a
 * = 0 is the reference. The fog distance is divided by eye w, so positive a reduces fog right of the screen center
 * (where eye_x > 0) and slightly increases it left of center; negative a does the opposite.
 *
 * @tc MVProjective-radial-exp2
 *  Exponential squared radial fog. The model view w column is modified so eye w = 1 + a * eye_x while eye xyz are
 * unchanged; a = 0 is the reference. The fog distance is divided by eye w, so positive a reduces fog right of the
 * screen center (where eye_x > 0) and slightly increases it left of center; negative a does the opposite.
 *
 * @tc MVZeroW-planar-lin
 *  Linear planar fog. The model view w column is replaced with (0, 0, 0, w) so eye xyz are unchanged and eye w is 1
 * (reference), +0, -0, 1e-6, or -1e-6. The final strip zeroes the entire matrix so eye position is (0, 0, 0, 0). The w
 * 1e-6 strip is entirely fogged. The w 0, w -0, w -1e-6, and w 0 xyz 0 strips are entirely unfogged (w 0 behaves like a
 * large negative distance).
 *
 * @tc MVZeroW-planar-exp
 *  Exponential planar fog. The model view w column is replaced with (0, 0, 0, w) so eye xyz are unchanged and eye w is
 * 1 (reference), +0, -0, 1e-6, or -1e-6. The final strip zeroes the entire matrix so eye position is (0, 0, 0, 0). The
 * w 1e-6 strip is entirely fogged. The w 0, w -0, w -1e-6, and w 0 xyz 0 strips are entirely unfogged (w 0 behaves like
 * a large negative distance).
 *
 * @tc MVZeroW-planar-exp2
 *  Exponential squared planar fog. The model view w column is replaced with (0, 0, 0, w) so eye xyz are unchanged and
 * eye w is 1 (reference), +0, -0, 1e-6, or -1e-6. The final strip zeroes the entire matrix so eye position is (0, 0, 0,
 * 0). The w 0, w -0, w 1e-6, and w -1e-6 strips are entirely fogged. The w 0 xyz 0 strip is entirely unfogged.
 *
 * @tc MVZeroW-abs_planar-lin
 *  Linear absolute planar fog. The model view w column is replaced with (0, 0, 0, w) so eye xyz are unchanged and eye w
 * is 1 (reference), +0, -0, 1e-6, or -1e-6. The final strip zeroes the entire matrix so eye position is (0, 0, 0, 0).
 * The w 1e-6 strip is entirely fogged. The w 0, w -0, w -1e-6, and w 0 xyz 0 strips are entirely unfogged (w 0 behaves
 * like a large negative distance). Despite the absolute value gen mode, the sign of w is not discarded.
 *
 * @tc MVZeroW-abs_planar-exp
 *  Exponential absolute planar fog. The model view w column is replaced with (0, 0, 0, w) so eye xyz are unchanged and
 * eye w is 1 (reference), +0, -0, 1e-6, or -1e-6. The final strip zeroes the entire matrix so eye position is (0, 0, 0,
 * 0). The w 1e-6 strip is entirely fogged. The w 0, w -0, w -1e-6, and w 0 xyz 0 strips are entirely unfogged (w 0
 * behaves like a large negative distance). Despite the absolute value gen mode, the sign of w is not discarded.
 *
 * @tc MVZeroW-abs_planar-exp2
 *  Exponential squared absolute planar fog. The model view w column is replaced with (0, 0, 0, w) so eye xyz are
 * unchanged and eye w is 1 (reference), +0, -0, 1e-6, or -1e-6. The final strip zeroes the entire matrix so eye
 * position is (0, 0, 0, 0). The w 0, w -0, w 1e-6, and w -1e-6 strips are entirely fogged. The w 0 xyz 0 strip is
 * entirely unfogged.
 *
 * @tc MVZeroW-radial-lin
 *  Linear radial fog. The model view w column is replaced with (0, 0, 0, w) so eye xyz are unchanged and eye w is 1
 * (reference), +0, -0, 1e-6, or -1e-6. The final strip zeroes the entire matrix so eye position is (0, 0, 0, 0). The w
 * 0, w -0, and w 0 xyz 0 strips are entirely unfogged. The w 1e-6 and w -1e-6 strips are entirely fogged (the radial
 * distance is positive regardless of the sign of w).
 *
 * @tc MVZeroW-radial-exp
 *  Exponential radial fog. The model view w column is replaced with (0, 0, 0, w) so eye xyz are unchanged and eye w is
 * 1 (reference), +0, -0, 1e-6, or -1e-6. The final strip zeroes the entire matrix so eye position is (0, 0, 0, 0). The
 * w 0, w -0, and w 0 xyz 0 strips are entirely unfogged. The w 1e-6 and w -1e-6 strips are entirely fogged (the radial
 * distance is positive regardless of the sign of w).
 *
 * @tc MVZeroW-radial-exp2
 *  Exponential squared radial fog. The model view w column is replaced with (0, 0, 0, w) so eye xyz are unchanged and
 * eye w is 1 (reference), +0, -0, 1e-6, or -1e-6. The final strip zeroes the entire matrix so eye position is (0, 0, 0,
 * 0). The w 0, w -0, and w 0 xyz 0 strips are entirely unfogged. The w 1e-6 and w -1e-6 strips are entirely fogged (the
 * radial distance is positive regardless of the sign of w).
 */
FogEyeSpaceWTests::FogEyeSpaceWTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Fog eye space W", config) {
  for (auto variant : {Variant::MV_SCALE, Variant::VERTEX_W, Variant::MV_PROJECTIVE, Variant::MV_ZERO_W}) {
    for (auto gen_mode : kFogGenModes) {
      for (auto fog_mode : kFogModes) {
        std::string name = VariantName(variant) + "-" + FogGenModeName(gen_mode) + "-" + FogModeName(fog_mode);
        tests_[name] = [this, name, variant, gen_mode, fog_mode]() { Test(name, variant, gen_mode, fog_mode); };
      }
    }
  }
}

void FogEyeSpaceWTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_LIGHTING_ENABLE, false);
  Pushbuffer::Push(NV097_SET_SPECULAR_ENABLE, false);
  Pushbuffer::Push(NV097_SET_FOG_ENABLE, true);
  Pushbuffer::Push(NV097_SET_FOG_COLOR, kFogColor);
  Pushbuffer::PushF(NV097_SET_FOG_PLANE, 0.f, 0.f, 1.f, 0.f);
  Pushbuffer::End();

  host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_DIFFUSE, false, false, TestHost::SRC_FOG, false,
                          false);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_DIFFUSE,
                          true, false);
}

static void SetupFogParams(uint32_t fog_gen_mode, uint32_t fog_mode) {
  static constexpr float kLn256 = 5.5452f;
  static constexpr float kSqrtLn256 = 2.3548f;

  float bias = 0.f;
  float multiplier = 0.f;
  switch (fog_mode) {
    case NV097_SET_FOG_MODE_V_LINEAR:
      multiplier = -1.f / kLinearFogEnd;
      bias = 1.f + -kLinearFogEnd * multiplier;
      break;
    case NV097_SET_FOG_MODE_V_EXP:
      bias = 1.5f;
      multiplier = -kExpDensity / (2.f * kLn256);
      break;
    case NV097_SET_FOG_MODE_V_EXP2:
      bias = 1.5f;
      multiplier = -kExp2Density / (2.f * kSqrtLn256);
      break;
    default:
      break;
  }

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_FOG_GEN_MODE, fog_gen_mode);
  Pushbuffer::Push(NV097_SET_FOG_MODE, fog_mode);
  Pushbuffer::PushF(NV097_SET_FOG_PARAMS, bias, multiplier, 0.f);
  Pushbuffer::End();
}

void FogEyeSpaceWTests::Test(const std::string &name, Variant variant, uint32_t fog_gen_mode, uint32_t fog_mode) {
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  SetupFogParams(fog_gen_mode, fog_mode);

  host_.PrepareDraw(0xFF232623);

  const LabeledValue *values;
  uint32_t num_values;
  switch (variant) {
    case Variant::MV_SCALE:
      values = kScales;
      num_values = sizeof(kScales) / sizeof(kScales[0]);
      break;
    case Variant::VERTEX_W:
      values = kVertexWs;
      num_values = sizeof(kVertexWs) / sizeof(kVertexWs[0]);
      break;
    case Variant::MV_PROJECTIVE:
      values = kProjectiveFactors;
      num_values = sizeof(kProjectiveFactors) / sizeof(kProjectiveFactors[0]);
      break;
    case Variant::MV_ZERO_W:
      values = kZeroWs;
      num_values = sizeof(kZeroWs) / sizeof(kZeroWs[0]);
      break;
  }

  matrix4_t default_model_view;
  TestHost::BuildDefaultXDKModelViewMatrix(default_model_view);
  const float eye_to_world_z = -default_model_view[3][2];

  // Strips occupy two text rows each, starting at text row 2, between columns 11 and 60.
  static constexpr float kLeft = 130.f;
  static constexpr float kRight = 620.f;
  static constexpr float kStripHeight = 45.f;
  static constexpr uint32_t kSegments = 16;
  static constexpr float kSegmentWidth = (kRight - kLeft) / static_cast<float>(kSegments);

  for (uint32_t i = 0; i < num_values; ++i) {
    const auto &entry = values[i];
    const int text_row = 2 + static_cast<int>(i) * 2;
    const float top = 25.f + 25.f * static_cast<float>(text_row);
    const float bottom = top + kStripHeight;

    float vertex_scale = 1.f;
    matrix4_t model_view;
    MatrixCopyMatrix(model_view, default_model_view);
    switch (variant) {
      case Variant::MV_SCALE:
        for (auto &row : model_view) {
          for (auto &col : row) {
            col *= entry.value;
          }
        }
        break;
      case Variant::VERTEX_W:
        vertex_scale = entry.value;
        break;
      case Variant::MV_PROJECTIVE:
        // eye_w = sum_i(v_i * M[i][3]), eye_x = sum_i(v_i * M[i][0]) so this yields eye_w = 1 + a * eye_x.
        for (auto &row : model_view) {
          row[3] += entry.value * row[0];
        }
        break;
      case Variant::MV_ZERO_W:
        for (auto &row : model_view) {
          if (entry.zero_xyz) {
            row[0] = row[1] = row[2] = 0.f;
          }
          row[3] = 0.f;
        }
        model_view[3][3] = entry.value;
        break;
    }

    // The composite matrix is intentionally left at the default so the screen position is unaffected. The inverse
    // model view is also left stale since lighting and texgen are disabled.
    Pushbuffer::Begin();
    Pushbuffer::PushTransposedMatrix(NV097_SET_MODEL_VIEW_MATRIX, model_view[0]);
    Pushbuffer::End();

    auto emit_vertex = [this, vertex_scale, eye_to_world_z](float x, float y, float eye_z) {
      const float world_z = eye_z + eye_to_world_z;
      const vector_t screen_point{x, y, world_z, 1.f};
      vector_t world_point{0.f, 0.f, 0.f, 1.f};
      host_.UnprojectPoint(world_point, screen_point, world_z);
      host_.SetVertex(world_point[0] * vertex_scale, world_point[1] * vertex_scale, world_point[2] * vertex_scale,
                      vertex_scale);
    };

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(kDiffuse);
    for (uint32_t s = 0; s < kSegments; ++s) {
      const float x0 = kLeft + kSegmentWidth * static_cast<float>(s);
      const float x1 = x0 + kSegmentWidth;
      const float z0 = kNearEyeZ + (kFarEyeZ - kNearEyeZ) * (x0 - kLeft) / (kRight - kLeft);
      const float z1 = kNearEyeZ + (kFarEyeZ - kNearEyeZ) * (x1 - kLeft) / (kRight - kLeft);
      emit_vertex(x0, top, z0);
      emit_vertex(x1, top, z1);
      emit_vertex(x1, bottom, z1);
      emit_vertex(x0, bottom, z0);
    }
    host_.End();

    pb_printat(text_row, 0, "%s", entry.label);
  }

  pb_printat(0, 0, "%s", name.c_str());
  pb_draw_text_screen();

  FinishDraw(name);
}
