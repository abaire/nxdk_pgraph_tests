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

static constexpr uint32_t kBackgroundColor = 0xFF222228;
static constexpr float kZNear = 10.f;
static constexpr float kZFar = 100.f;
static constexpr float kWNear = 10.f;
static constexpr float kWFar = 100.f;

static std::string MakeTestName(const char* prefix, uint32_t mode, bool w_buffered);

ZMinMaxControlTests::ZMinMaxControlTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "ZMinMaxControl", config) {
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

static std::string MakeTestName(const char* prefix, uint32_t mode, bool w_buffered) {
  std::string ret = prefix;
  ret += w_buffered ? "_WBuf" : "";
  ret += (mode & NV097_SET_ZMIN_MAX_CONTROL_CULL_NEAR_FAR_EN_TRUE) ? "_NEARFAR" : "";
  ret += (mode & NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CLAMP) ? "_ZCLAMP" : "_ZCULL";
  ret += (mode & NV097_SET_ZMIN_MAX_CONTROL_CULL_IGNORE_W_TRUE) ? "_IgnW" : "";

  return ret;
}
