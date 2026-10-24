#include "z_min_max_control_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/passthrough_vertex_shader.h"
#include "vertex_buffer.h"
#include "xbox_math_d3d.h"
#include "xbox_math_matrix.h"

constexpr uint32_t kSmallSize = 16;
constexpr uint32_t kSmallSpacing = 4;
constexpr uint32_t kStep = kSmallSize + kSmallSpacing;

static constexpr const char kTestName[] = "Ctrl";
static constexpr const char kTestFixedName[] = "CtrlFixed";
static constexpr const char kTestNearPlaneBoundaryPassthroughName[] = "NearPlaneBoundaryPassthrough";

static constexpr uint32_t kBackgroundColor = 0xFF222228;
static constexpr float kZNear = 10.f;
static constexpr float kZFar = 100.f;
static constexpr float kWNear = 10.f;
static constexpr float kWFar = 100.f;

static std::string MakeTestName(const char* prefix, uint32_t mode, bool w_buffered);

/**
 * Initializes the test suite and creates test cases.
 *
 * Ctrl* and CtrlFixed* tests set NV097_SET_ZMIN_MAX_CONTROL to the mode encoded in the test name, set the depth clip
 * range to 10-100 (in z, or in w for WBuf tests) and draw six blocks of small quads whose z (in depth buffer units,
 * 0-150) and w values vary as labeled: top left: z increasing, w = 1; top middle: z increasing, w = 0.01 on the left
 * edge of each quad and 0.0001 on the right; top right: z and w increasing; bottom left: z from 8 (left edge) to 120
 * (right edge) of each quad, w increasing; bottom middle: z and w from 8 to 120 across each quad; bottom right: z
 * increasing, w = 10. Ctrl tests pass screen space coordinates through a passthrough vertex shader. CtrlFixed tests
 * unproject the same screen space points and draw them with the fixed function pipeline using a D3D style orthographic
 * projection with near/far planes at 10/100.
 *
 * Name components: WBuf enables w-buffering; NEARFAR sets CULL_NEAR_FAR_EN_TRUE; ZCLAMP sets ZCLAMP_EN_CLAMP and ZCULL
 * sets ZCLAMP_EN_CULL; IgnW sets CULL_IGNORE_W_TRUE. Quads (or portions of quads) whose depth falls outside the clip
 * range are culled or clamped according to the mode.
 *
 * @tc CtrlFixed_NEARFAR_ZCLAMP
 *  Draws the quad blocks with the fixed function pipeline; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_TRUE |
 *  ZCLAMP_EN_CLAMP | CULL_IGNORE_W_FALSE.
 *
 * @tc CtrlFixed_NEARFAR_ZCLAMP_IgnW
 *  Draws the quad blocks with the fixed function pipeline; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_TRUE |
 *  ZCLAMP_EN_CLAMP | CULL_IGNORE_W_TRUE.
 *
 * @tc CtrlFixed_NEARFAR_ZCULL
 *  Draws the quad blocks with the fixed function pipeline; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_TRUE |
 *  ZCLAMP_EN_CULL | CULL_IGNORE_W_FALSE.
 *
 * @tc CtrlFixed_NEARFAR_ZCULL_IgnW
 *  Draws the quad blocks with the fixed function pipeline; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_TRUE |
 *  ZCLAMP_EN_CULL | CULL_IGNORE_W_TRUE.
 *
 * @tc CtrlFixed_WBuf_NEARFAR_ZCLAMP
 *  Draws the quad blocks with the fixed function pipeline and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_TRUE | ZCLAMP_EN_CLAMP | CULL_IGNORE_W_FALSE.
 *
 * @tc CtrlFixed_WBuf_NEARFAR_ZCLAMP_IgnW
 *  Draws the quad blocks with the fixed function pipeline and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_TRUE | ZCLAMP_EN_CLAMP | CULL_IGNORE_W_TRUE.
 *
 * @tc CtrlFixed_WBuf_NEARFAR_ZCULL
 *  Draws the quad blocks with the fixed function pipeline and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_TRUE | ZCLAMP_EN_CULL | CULL_IGNORE_W_FALSE.
 *
 * @tc CtrlFixed_WBuf_NEARFAR_ZCULL_IgnW
 *  Draws the quad blocks with the fixed function pipeline and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_TRUE | ZCLAMP_EN_CULL | CULL_IGNORE_W_TRUE.
 *
 * @tc CtrlFixed_WBuf_ZCLAMP
 *  Draws the quad blocks with the fixed function pipeline and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_FALSE | ZCLAMP_EN_CLAMP | CULL_IGNORE_W_FALSE.
 *
 * @tc CtrlFixed_WBuf_ZCLAMP_IgnW
 *  Draws the quad blocks with the fixed function pipeline and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_FALSE | ZCLAMP_EN_CLAMP | CULL_IGNORE_W_TRUE.
 *
 * @tc CtrlFixed_WBuf_ZCULL
 *  Draws the quad blocks with the fixed function pipeline and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_FALSE | ZCLAMP_EN_CULL | CULL_IGNORE_W_FALSE.
 *
 * @tc CtrlFixed_WBuf_ZCULL_IgnW
 *  Draws the quad blocks with the fixed function pipeline and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_FALSE | ZCLAMP_EN_CULL | CULL_IGNORE_W_TRUE.
 *
 * @tc CtrlFixed_ZCLAMP
 *  Draws the quad blocks with the fixed function pipeline; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_FALSE |
 *  ZCLAMP_EN_CLAMP | CULL_IGNORE_W_FALSE.
 *
 * @tc CtrlFixed_ZCLAMP_IgnW
 *  Draws the quad blocks with the fixed function pipeline; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_FALSE |
 *  ZCLAMP_EN_CLAMP | CULL_IGNORE_W_TRUE.
 *
 * @tc CtrlFixed_ZCULL
 *  Draws the quad blocks with the fixed function pipeline; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_FALSE |
 *  ZCLAMP_EN_CULL | CULL_IGNORE_W_FALSE.
 *
 * @tc CtrlFixed_ZCULL_IgnW
 *  Draws the quad blocks with the fixed function pipeline; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_FALSE |
 *  ZCLAMP_EN_CULL | CULL_IGNORE_W_TRUE.
 *
 * @tc Ctrl_NEARFAR_ZCLAMP
 *  Draws the quad blocks with the passthrough vertex shader; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_TRUE |
 *  ZCLAMP_EN_CLAMP | CULL_IGNORE_W_FALSE.
 *
 * @tc Ctrl_NEARFAR_ZCLAMP_IgnW
 *  Draws the quad blocks with the passthrough vertex shader; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_TRUE |
 *  ZCLAMP_EN_CLAMP | CULL_IGNORE_W_TRUE.
 *
 * @tc Ctrl_NEARFAR_ZCULL
 *  Draws the quad blocks with the passthrough vertex shader; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_TRUE |
 *  ZCLAMP_EN_CULL | CULL_IGNORE_W_FALSE.
 *
 * @tc Ctrl_NEARFAR_ZCULL_IgnW
 *  Draws the quad blocks with the passthrough vertex shader; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_TRUE |
 *  ZCLAMP_EN_CULL | CULL_IGNORE_W_TRUE.
 *
 * @tc Ctrl_WBuf_NEARFAR_ZCLAMP
 *  Draws the quad blocks with the passthrough vertex shader and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_TRUE | ZCLAMP_EN_CLAMP | CULL_IGNORE_W_FALSE.
 *
 * @tc Ctrl_WBuf_NEARFAR_ZCLAMP_IgnW
 *  Draws the quad blocks with the passthrough vertex shader and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_TRUE | ZCLAMP_EN_CLAMP | CULL_IGNORE_W_TRUE.
 *
 * @tc Ctrl_WBuf_NEARFAR_ZCULL
 *  Draws the quad blocks with the passthrough vertex shader and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_TRUE | ZCLAMP_EN_CULL | CULL_IGNORE_W_FALSE.
 *
 * @tc Ctrl_WBuf_NEARFAR_ZCULL_IgnW
 *  Draws the quad blocks with the passthrough vertex shader and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_TRUE | ZCLAMP_EN_CULL | CULL_IGNORE_W_TRUE.
 *
 * @tc Ctrl_WBuf_ZCLAMP
 *  Draws the quad blocks with the passthrough vertex shader and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_FALSE | ZCLAMP_EN_CLAMP | CULL_IGNORE_W_FALSE.
 *
 * @tc Ctrl_WBuf_ZCLAMP_IgnW
 *  Draws the quad blocks with the passthrough vertex shader and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_FALSE | ZCLAMP_EN_CLAMP | CULL_IGNORE_W_TRUE.
 *
 * @tc Ctrl_WBuf_ZCULL
 *  Draws the quad blocks with the passthrough vertex shader and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_FALSE | ZCLAMP_EN_CULL | CULL_IGNORE_W_FALSE.
 *
 * @tc Ctrl_WBuf_ZCULL_IgnW
 *  Draws the quad blocks with the passthrough vertex shader and w-buffering enabled; NV097_SET_ZMIN_MAX_CONTROL =
 *  CULL_NEAR_FAR_EN_FALSE | ZCLAMP_EN_CULL | CULL_IGNORE_W_TRUE.
 *
 * @tc Ctrl_ZCLAMP
 *  Draws the quad blocks with the passthrough vertex shader; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_FALSE |
 *  ZCLAMP_EN_CLAMP | CULL_IGNORE_W_FALSE.
 *
 * @tc Ctrl_ZCLAMP_IgnW
 *  Draws the quad blocks with the passthrough vertex shader; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_FALSE |
 *  ZCLAMP_EN_CLAMP | CULL_IGNORE_W_TRUE.
 *
 * @tc Ctrl_ZCULL
 *  Draws the quad blocks with the passthrough vertex shader; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_FALSE |
 *  ZCLAMP_EN_CULL | CULL_IGNORE_W_FALSE.
 *
 * @tc Ctrl_ZCULL_IgnW
 *  Draws the quad blocks with the passthrough vertex shader; NV097_SET_ZMIN_MAX_CONTROL = CULL_NEAR_FAR_EN_FALSE |
 *  ZCLAMP_EN_CULL | CULL_IGNORE_W_TRUE.
 *
 * @tc NearPlaneBoundaryPassthrough
 *  Draws three quads through a passthrough vertex shader in screen space with w = 40 and screen z of -0.8, 0 and 0.8
 *  from left to right, on a Z24S8 surface with the default NV097_SET_ZMIN_MAX_CONTROL (CULL_NEAR_FAR, ZCLAMP_CULL) and
 *  a clip range of 0-16777215. A z exactly equal to the clip minimum is inside the range, so only the left quad is
 *  culled. Together with the Vertex shader rounding tests OpRounding_* this shows that Rugby 2005's depth of field quad
 *  (xemu-project/xemu#2388) is culled on hardware because truncation in the vertex shader places it slightly behind the
 *  near plane, not because of how the clip boundary is handled.
 */
ZMinMaxControlTests::ZMinMaxControlTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "ZMinMaxControl", config) {
  tests_[kTestNearPlaneBoundaryPassthroughName] = [this]() { TestNearPlaneBoundaryPassthrough(); };

  for (auto w_buffered : {false, true}) {
    for (auto cull :
         {NV097_SET_ZMIN_MAX_CONTROL_CULL_NEAR_FAR_EN_FALSE, NV097_SET_ZMIN_MAX_CONTROL_CULL_NEAR_FAR_EN_TRUE}) {
      for (auto z_clamp : {NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CULL, NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CLAMP}) {
        for (auto ignore_w :
             {NV097_SET_ZMIN_MAX_CONTROL_CULL_IGNORE_W_FALSE, NV097_SET_ZMIN_MAX_CONTROL_CULL_IGNORE_W_TRUE}) {
          uint32_t mode = cull | z_clamp | ignore_w;
          {
            std::string name = MakeTestName(kTestName, mode, w_buffered);
            tests_[name] = [this, name, mode, w_buffered]() { Test(name, mode, w_buffered); };
          }
          {
            std::string name = MakeTestName(kTestFixedName, mode, w_buffered);
            tests_[name] = [this, name, mode, w_buffered]() { TestFixed(name, mode, w_buffered); };
          }
        }
      }
    }
  }

  const auto fb_width = host.GetFramebufferWidthF();
  const auto fb_height = host.GetFramebufferHeightF();

  kLeft = floorf(fb_width / 5.0f);
  kRight = fb_width - kLeft;
  kTop = floorf(fb_height / 10.0f);
  kBottom = fb_height - kTop;
  kRegionWidth = (kRight - kLeft) * 0.33f;
  kRegionHeight = (kBottom - kTop) * 0.5f;

  quads_per_row_ = static_cast<uint32_t>(kRegionWidth - kSmallSize) / kStep;
  quads_per_col_ = static_cast<uint32_t>(kRegionHeight - kSmallSize) / kStep;

  col_size_ = static_cast<float>(kSmallSize) + static_cast<float>(quads_per_col_) * kStep;
  num_quads_ = quads_per_row_ * quads_per_col_;
}

void ZMinMaxControlTests::Initialize() { TestSuite::Initialize(); }

void ZMinMaxControlTests::TearDownTest() {
  TestSuite::TearDownTest();
  host_.SetupControl0();
  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_ZMIN_MAX_CONTROL, NV097_SET_ZMIN_MAX_CONTROL_CULL_NEAR_FAR_EN_TRUE |
                                                   NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CULL |
                                                   NV097_SET_ZMIN_MAX_CONTROL_CULL_IGNORE_W_FALSE);
  Pushbuffer::End();
}

static void AddVertex(TestHost& host, const vector_t& position, float r, float g, float b) {
  host.SetDiffuse(r, g, b);
  host.SetVertex(position);
};

void ZMinMaxControlTests::DrawBlock(float x_offset, float y_offset, ZMinMaxDrawMode zw_mode, bool projected) const {
  const float z_min = 0.f;
  const float z_max = kZFar * 1.5f;
  const float w_min = 0.f;
  const float w_max = kWFar * 1.5f;

  float z_inc = (z_max - z_min) / (static_cast<float>(num_quads_) + 1.f);
  float w_inc = (w_max - w_min) / (static_cast<float>(num_quads_) + 1.f);

  float z_left = z_min;
  float w_left = w_min;
  float right_offset_z = z_inc * 0.75f;
  float right_offset_w = w_inc * 0.75f;
  float y = y_offset;

  for (int y_idx = 0; y_idx < quads_per_col_; ++y_idx) {
    float x = x_offset;
    for (int x_idx = 0; x_idx < quads_per_row_; ++x_idx) {
      float z_right = z_left + right_offset_z;
      float w_right = w_left + right_offset_w;

      Color left;
      Color right;

      const float left_z_color = 0.25f + z_left / z_max * 0.75f;
      const float right_z_color = 0.25f + z_right / z_max * 0.75f;
      const float left_w_color = 0.25f + w_left / w_max * 0.75f;
      const float right_w_color = 0.25f + w_right / w_max * 0.75f;

      switch (zw_mode) {
        case M_Z_INC_W_ONE:
          left.SetGrey(left_z_color);
          right.SetGrey(right_z_color);
          w_left = 1.f;
          w_right = 1.f;
          break;

        case M_Z_INC_W_INC: {
          left.SetRGB(left_z_color, 0.25f, left_w_color);
          right.SetRGB(right_z_color, 0.25f, right_w_color);
        } break;

        case M_Z_NF_W_INC:
          left.SetRGB(0.33f, 0.25f, left_w_color);
          right.SetRGB(0.33f, 0.25f, right_w_color);
          z_left = kZNear * 0.8f;
          z_right = kZFar * 1.2f;
          break;

        case M_Z_NF_W_NF:
          left.SetRGB(0.33f, 0.75f, 0.25f);
          right.SetRGB(0.33f, 0.75f, 0.8f);
          z_left = kZNear * 0.8f;
          z_right = kZFar * 1.2f;
          w_left = kWNear * 0.8f;
          w_right = kWFar * 1.2f;
          break;

        case M_Z_INC_W_TEN:
          left.SetRGB(left_z_color, left_z_color, 0.25f);
          right.SetRGB(right_z_color, right_z_color, 0.8f);
          w_left = 10.f;
          w_right = 10.f;
          break;

        case M_Z_INC_W_FRAC:
          left.SetRGB(0.75f, left_z_color, left_z_color);
          right.SetRGB(right_z_color, right_z_color, 0.8f);
          w_left = 0.01f;
          w_right = 0.0001f;
          break;

        case M_Z_INC_W_INV_Z:
          left.SetRGB(0.5f, left_z_color, 0.5f);
          right.SetRGB(0.5f, right_z_color, 0.5f);
          w_left = 1.f / z_left;
          w_right = 1.f / z_right;
      }

      if (!projected) {
        AddVertex(host_, {x, y, z_left, w_left}, left.r, left.g, left.b);
        AddVertex(host_, {x + kSmallSize, y, z_right, w_right}, right.r, right.g, right.b);
        AddVertex(host_, {x + kSmallSize, y + kSmallSize, z_right, w_right}, right.r, right.g, right.b);
        AddVertex(host_, {x, y + kSmallSize, z_left, w_left}, left.r, left.g, left.b);
      } else {
        auto w_adjust = [](vector_t& pt, float w) {
          pt[0] *= w;
          pt[1] *= w;
          pt[2] *= w;
          pt[3] = w;
        };

        vector_t world_pt;
        host_.UnprojectPoint(world_pt, {x, y, z_left, 1.f});
        w_adjust(world_pt, w_left);
        AddVertex(host_, world_pt, left.r, left.g, left.b);

        host_.UnprojectPoint(world_pt, {x + kSmallSize, y, z_right, 1.f});
        //        host_.UnprojectPoint(world_pt, {x + kSmallSize, y, 0.f, 1.f}, z_right);
        w_adjust(world_pt, w_right);
        AddVertex(host_, world_pt, right.r, right.g, right.b);

        host_.UnprojectPoint(world_pt, {x + kSmallSize, y + kSmallSize, z_right, 1.f});
        //        host_.UnprojectPoint(world_pt, {x + kSmallSize, y + kSmallSize, 0.f, 1.f}, z_right);
        w_adjust(world_pt, w_right);
        AddVertex(host_, world_pt, right.r, right.g, right.b);

        host_.UnprojectPoint(world_pt, {x, y + kSmallSize, z_left, 1.f});
        //        host_.UnprojectPoint(world_pt, {x, y + kSmallSize, 0.f, 1.f}, z_left);
        w_adjust(world_pt, w_left);
        AddVertex(host_, world_pt, left.r, left.g, left.b);
      }

      z_left += z_inc;
      w_left += w_inc;
      x += kStep;
    }

    y += kStep;
  }
}

void ZMinMaxControlTests::Test(const std::string& name, uint32_t mode, bool w_buffered) {
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.PrepareDraw(kBackgroundColor);

  if (w_buffered) {
    host_.SetDepthClip(kWNear, kWFar);
  } else {
    host_.SetDepthClip(kZNear, kZFar);
  }

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_ZMIN_MAX_CONTROL, mode);
  Pushbuffer::End();

  const float zscale = host_.GetMaxDepthBufferValue();
  host_.SetupControl0(false, w_buffered);
  TestHost::SetViewportScale(320.f, -240.f, zscale, w_buffered ? 1.f : 0.f);

  host_.Begin(TestHost::PRIMITIVE_QUADS);

  float x_offset = 90.f;
  float y_offset = kTop + kSmallSpacing + (kRegionHeight - col_size_) * 0.5f;

  DrawBlock(x_offset, y_offset, M_Z_INC_W_ONE);
  DrawBlock(x_offset, y_offset + kRegionHeight, M_Z_NF_W_INC);

  x_offset += kRegionWidth + 90.f;

  DrawBlock(x_offset, y_offset, M_Z_INC_W_FRAC);
  DrawBlock(x_offset, y_offset + kRegionHeight, M_Z_NF_W_NF);

  x_offset += kRegionWidth + 90.f;

  DrawBlock(x_offset, y_offset, M_Z_INC_W_INC);
  DrawBlock(x_offset, y_offset + kRegionHeight, M_Z_INC_W_TEN);

  host_.End();

  pb_print("%s\n", name.c_str());
  pb_printat(2, 1, (char*)"Z=Inc");
  pb_printat(2, 21, (char*)"Z=Inc");
  pb_printat(2, 44, (char*)"Z=Inc");
  pb_printat(3, 1, (char*)"W=1");
  pb_printat(3, 21, (char*)"W=Frac");
  pb_printat(3, 44, (char*)"W=Inc");

  pb_printat(11, 1, (char*)"Z=N->F");
  pb_printat(11, 21, (char*)"Z=N->F");
  pb_printat(11, 44, (char*)"Z=Inc");
  pb_printat(12, 1, (char*)"W=Inc");
  pb_printat(12, 21, (char*)"W=N->F");
  pb_printat(12, 44, (char*)"W=10");

  pb_draw_text_screen();
  FinishDraw(name);
}

void ZMinMaxControlTests::TestFixed(const std::string& name, uint32_t mode, bool w_buffered) {
  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  host_.PrepareDraw(kBackgroundColor);

  TestHost::SetViewportScale(0.f, 0.f, 0.f, 0.f);

  matrix4_t matrix;
  XboxMath::MatrixSetIdentity(matrix);
  XboxMath::MatrixTranslate(matrix, {0.f, 0.f, -3.f, 1.f});
  host_.SetFixedFunctionModelViewMatrix(matrix);

  const auto near_plane = w_buffered ? kWNear : kZNear;
  const auto far_plane = w_buffered ? kWFar : kZFar;
  TestHost::BuildD3DOrthographicProjectionMatrix(matrix, -320.f, 320.f, 240.f, -240.f, near_plane, far_plane);
  matrix4_t viewport;
  const auto fb_width = host_.GetFramebufferWidthF();
  const auto fb_height = host_.GetFramebufferHeightF();
  XboxMath::CreateD3DStandardViewport16Bit(viewport, fb_width, fb_height);
  matrix4_t proj_viewport;
  MatrixMultMatrix(matrix, viewport, proj_viewport);
  host_.SetFixedFunctionProjectionMatrix(proj_viewport);

  host_.SetupControl0(false, w_buffered);
  if (w_buffered) {
    host_.SetDepthClip(kWNear, kWFar);
  } else {
    host_.SetDepthClip(kZNear, kZFar);
  }

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_ZMIN_MAX_CONTROL, mode);
  Pushbuffer::End();

  const float zscale = host_.GetMaxDepthBufferValue();
  host_.SetupControl0(false, w_buffered);
  TestHost::SetViewportScale(320.f, -240.f, zscale, w_buffered ? 1.f : 0.f);

  host_.Begin(TestHost::PRIMITIVE_QUADS);

  float x_offset = 90.f;
  float y_offset = kTop + kSmallSpacing + (kRegionHeight - col_size_) * 0.5f;

  DrawBlock(x_offset, y_offset, M_Z_INC_W_ONE, true);
  DrawBlock(x_offset, y_offset + kRegionHeight, M_Z_NF_W_INC, true);

  x_offset += kRegionWidth + 90.f;

  DrawBlock(x_offset, y_offset, M_Z_INC_W_FRAC, true);
  DrawBlock(x_offset, y_offset + kRegionHeight, M_Z_NF_W_NF, true);

  x_offset += kRegionWidth + 90.f;

  DrawBlock(x_offset, y_offset, M_Z_INC_W_INC, true);
  DrawBlock(x_offset, y_offset + kRegionHeight, M_Z_INC_W_TEN, true);

  host_.End();

  pb_print("%s\n", name.c_str());
  pb_printat(2, 1, (char*)"Z=Inc");
  pb_printat(2, 21, (char*)"Z=Inc");
  pb_printat(2, 44, (char*)"Z=Inc");
  pb_printat(3, 1, (char*)"W=1");
  pb_printat(3, 21, (char*)"W=Frac");
  pb_printat(3, 44, (char*)"W=Inc");

  pb_printat(11, 1, (char*)"Z=N->F");
  pb_printat(11, 21, (char*)"Z=N->F");
  pb_printat(11, 44, (char*)"Z=Inc");
  pb_printat(12, 1, (char*)"W=Inc");
  pb_printat(12, 21, (char*)"W=N->F");
  pb_printat(12, 44, (char*)"W=10");

  pb_draw_text_screen();
  FinishDraw(name);
}

void ZMinMaxControlTests::TestNearPlaneBoundaryPassthrough() {
  static constexpr float kMaxDepth = 16777215.f;
  static constexpr float kScreenZs[] = {-0.8f, 0.f, 0.8f};
  static constexpr float kW = 40.f;
  static constexpr uint32_t kColors[] = {0xFFCC3333, 0xFF33CC33, 0xFF3333CC};
  static constexpr float kQuadSize = 120.f;

  host_.SetVertexShaderProgram(std::make_shared<PassthroughVertexShader>());
  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());
  host_.PrepareDraw(kBackgroundColor);
  host_.SetDepthClip(0.f, kMaxDepth);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_ZMIN_MAX_CONTROL, NV097_SET_ZMIN_MAX_CONTROL_CULL_NEAR_FAR_EN_TRUE |
                                                   NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CULL |
                                                   NV097_SET_ZMIN_MAX_CONTROL_CULL_IGNORE_W_FALSE);
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_ALWAYS);
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, false);
  Pushbuffer::End();

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  for (uint32_t i = 0; i < 3; ++i) {
    const float left = 80.f + static_cast<float>(i) * 180.f;
    const float right = left + kQuadSize;
    const float top = 180.f;
    const float bottom = top + kQuadSize;
    host_.SetDiffuse(kColors[i]);
    host_.SetVertex(left, top, kScreenZs[i], kW);
    host_.SetVertex(right, top, kScreenZs[i], kW);
    host_.SetVertex(right, bottom, kScreenZs[i], kW);
    host_.SetVertex(left, bottom, kScreenZs[i], kW);
  }
  host_.End();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  Pushbuffer::End();

  pb_print("%s\n", kTestNearPlaneBoundaryPassthroughName);
  pb_printat(2, 0, "screen z (w = 40, clip min = 0)");
  pb_printat(13, 11, "-0.8");
  pb_printat(13, 29, "0.0");
  pb_printat(13, 47, "0.8");
  pb_draw_text_screen();

  FinishDraw(kTestNearPlaneBoundaryPassthroughName);
}

static std::string MakeTestName(const char* prefix, uint32_t mode, bool w_buffered) {
  std::string ret = prefix;
  ret += w_buffered ? "_WBuf" : "";
  ret += (mode & NV097_SET_ZMIN_MAX_CONTROL_CULL_NEAR_FAR_EN_TRUE) ? "_NEARFAR" : "";
  ret += (mode & NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CLAMP) ? "_ZCLAMP" : "_ZCULL";
  ret += (mode & NV097_SET_ZMIN_MAX_CONTROL_CULL_IGNORE_W_TRUE) ? "_IgnW" : "";

  return ret;
}
