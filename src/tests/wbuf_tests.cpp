#include "wbuf_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "shaders/passthrough_vertex_shader.h"
#include "shaders/perspective_vertex_shader.h"
#include "texture_generator.h"

static constexpr const char kTestWBuf[] = "WBuf";
static constexpr const char kTestZBuf[] = "ZBuf";
static constexpr const char kPrimLineStrip[] = "LineStrip";
static constexpr const char kPrimFloorQuad[] = "FloorQuad";
static constexpr const char kPrimRoofQuad[] = "RoofQuad";
static constexpr const char kPrimWallQuad[] = "WallQuad";
static constexpr const char kPrimRiddick[] = "Riddick";
static constexpr const char kPrimFloorClip[] = "ClipF";
static constexpr const char kPrimWallClip[] = "ClipW";
static constexpr const char kPrimTrunc0[] = "Trunc0";
static constexpr const char kPrimTruncNeg[] = "TruncNeg";
static constexpr const char kPrimLargeZ[] = "LargeZ";
static constexpr const char kPrimHTri[] = "TriH";
static constexpr const char kPrimVTri[] = "TriV";
static constexpr float kWBufferZBias = 16.0f;
static constexpr float kWBufferZSlope = 65536.0f;
static constexpr float kZBufferZSlope = 16.0f;
static constexpr uint32_t kClipLeft = 150;
static constexpr uint32_t kClipTop = 0;

static const std::vector<int> kVertSampleCoords = {
    159, 0,   159, 32,  159, 64,  159, 96,  159, 128, 159, 160, 159, 192, 159,
    224, 159, 256, 159, 288, 159, 320, 159, 352, 159, 384, 159, 416, 159, 448,
};

static const std::vector<int> kHorizSampleCoords = {
    159, 5,   193, 5,   227, 5,   261, 5,   295, 5,   329, 5,   363, 5,   397,
    5,   431, 5,   465, 5,   499, 5,   533, 5,   567, 5,   601, 5,   635, 5,
};

static const std::vector<int> kRiddickSampleCoords = {
    316, 262, 270, 266, 184, 479, 150, 463,
};

static bool IsWBuf(int depthf) { return depthf & 4; }

static int GetDepthBits(int depthf) { return (depthf & 2) ? 24 : 16; }

static bool IsFloatDepth(int depthf) { return depthf & 1; }

static std::string MakeTestName(const char *prim_name, int depthf, bool zbias, bool zslope, bool vsh,
                                uint32_t clip_left = kClipLeft, uint32_t clip_top = kClipTop) {
  bool wbuf = IsWBuf(depthf);
  int depth_bits = GetDepthBits(depthf);
  bool float_depth = IsFloatDepth(depthf);

  char depth_buf[32] = {0};
  snprintf(depth_buf, sizeof(depth_buf), "%s%d%c", wbuf ? kTestWBuf : kTestZBuf, depth_bits, float_depth ? 'F' : 'D');

  char clip_buf[32] = {0};
  if (clip_left != kClipLeft || clip_top != kClipTop) {
    snprintf(clip_buf, sizeof(clip_buf), "-%03d-%03d", clip_left, clip_top);
  }

  return std::string(depth_buf) + "_" + prim_name + clip_buf + "_V" + (vsh ? "1" : "0") + "_ZB" + (zbias ? "1" : "0") +
         "_ZS" + (zslope ? "1" : "0");
}

WBufTests::WBufTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "W buffering", config) {
  for (auto depthf : {0, 1, 2, 3, 4, 5, 6, 7}) {
    for (auto zbias : {false, true}) {
      for (auto zslope : {false, true}) {
        bool vsh = true;

        tests_[MakeTestName(kPrimFloorQuad, depthf, zbias, zslope, vsh)] = [this, depthf, zbias, zslope, vsh]() {
          this->Test(depthf, zbias, zslope, vsh, kPrimFloorQuad, [this]() {
            float tu = 1.0;

            host_.Begin(TestHost::PRIMITIVE_QUADS);
            host_.SetTexCoord0(0.0f, 0.0f);
            host_.SetVertex(53.1875f, -34.3125f, 16809768.0f, 325.8057861328125f);

            host_.SetTexCoord0(tu, 0.0f);
            host_.SetVertex(586.75f, -34.3125f, 16809768.0f, 325.8057861328125f);

            host_.SetTexCoord0(tu, tu);
            host_.SetVertex(1808.6875f, 453.0f, 16572698.0f, 58.379993438720703f);

            host_.SetTexCoord0(0.0f, tu);
            host_.SetVertex(-1168.6875f, 453.0f, 16572698.0f, 58.379993438720703f);

            host_.End();
            return kVertSampleCoords;
          });
        };

        tests_[MakeTestName(kPrimRoofQuad, depthf, zbias, zslope, vsh)] = [this, depthf, zbias, zslope, vsh]() {
          this->Test(depthf, zbias, zslope, vsh, kPrimRoofQuad, [this]() {
            float tu = 1.0;

            host_.Begin(TestHost::PRIMITIVE_QUADS);
            host_.SetTexCoord0(0.0f, tu);
            host_.SetVertex(-1168.6875f, 0.0f, 16572698.0f, 58.379993438720703f);

            host_.SetTexCoord0(tu, tu);
            host_.SetVertex(1808.6875f, 0.0f, 16572698.0f, 58.379993438720703f);

            host_.SetTexCoord0(tu, 0.0f);
            host_.SetVertex(586.75f, 487.3125f, 16809768.0f, 325.8057861328125f);

            host_.SetTexCoord0(0.0f, 0.0f);
            host_.SetVertex(53.1875f, 487.3125f, 16809768.0f, 325.8057861328125f);

            host_.End();
            return kVertSampleCoords;
          });
        };

        tests_[MakeTestName(kPrimWallQuad, depthf, zbias, zslope, vsh)] = [this, depthf, zbias, zslope, vsh]() {
          this->Test(depthf, zbias, zslope, vsh, kPrimWallQuad, [this]() {
            float tu = 1.0;

            host_.Begin(TestHost::PRIMITIVE_QUADS);
            host_.SetTexCoord0(0.0f, 0.0f);
            host_.SetVertex(637.3125f, -26.8125f, 16809768.0f, 325.8057861328125f);

            host_.SetTexCoord0(tu, 0.0f);
            host_.SetVertex(637.3125f, 506.75f, 16809768.0f, 325.8057861328125f);

            host_.SetTexCoord0(tu, tu);
            host_.SetVertex(150.0f, 1728.6875f, 16572698.0f, 58.379993438720703f);

            host_.SetTexCoord0(0.0f, tu);
            host_.SetVertex(150.0f, -1248.6875f, 16572698.0f, 58.379993438720703f);

            host_.End();
            return kHorizSampleCoords;
          });
        };

        tests_[MakeTestName(kPrimLargeZ, depthf, zbias, zslope, vsh)] = [this, depthf, zbias, zslope, vsh]() {
          this->Test(depthf, zbias, zslope, vsh, kPrimLargeZ, [this]() {
            float tu = 1.0;
            float md = 16777046.0f;
            float incr = 1.0f;

            host_.Begin(TestHost::PRIMITIVE_QUADS);
            host_.SetTexCoord0(0.0f, 0.0f);
            host_.SetVertex(0.0f, 0.0f, md, md);

            host_.SetTexCoord0(tu, 0.0f);
            host_.SetVertex(640.0f, 0.0f, md, md);

            host_.SetTexCoord0(tu, tu);
            host_.SetVertex(640.0f, 480.0f, md + incr, md + incr);

            host_.SetTexCoord0(0.0f, tu);
            host_.SetVertex(0.0f, 480.0f, md + incr, md + incr);

            host_.End();
            return kVertSampleCoords;
          });
        };

        tests_[MakeTestName(kPrimLineStrip, depthf, zbias, zslope, vsh)] = [this, depthf, zbias, zslope, vsh]() {
          this->Test(depthf, zbias, zslope, vsh, kPrimLineStrip, [this]() {
            float tu = 1.0;

            host_.Begin(TestHost::PRIMITIVE_LINE_STRIP);
            host_.SetTexCoord0(0.0f, 0.0f);
            host_.SetVertex(321.0f, 0.5f, 16777210.0f, 58.379993438720703f);

            host_.SetTexCoord0(tu, 0.0f);
            host_.SetVertex(159.5f, 0.5f, 16777210.0f, 325.8057861328125f);

            host_.SetTexCoord0(tu, tu);
            host_.SetVertex(159.5f, 448.5f, 16572698.0f, 58.379993438720703f);

            host_.SetTexCoord0(0.0f, tu);
            host_.SetVertex(321.0f, 448.5f, 16572698.0f, 325.8057861328125f);

            host_.End();
            return std::vector<int>{
                320, 0,   267, 0,   214, 0,   159, 0,   159, 56,  159, 112, 159, 168, 159,
                224, 159, 280, 159, 336, 159, 392, 159, 448, 214, 448, 267, 448, 320, 448,
            };
          });
        };

        tests_[MakeTestName(kPrimHTri, depthf, zbias, zslope, vsh)] = [this, depthf, zbias, zslope, vsh]() {
          this->Test(depthf, zbias, zslope, vsh, kPrimHTri, [this]() {
            std::vector<int> sample_coords;
            float tu = 1.0;
            host_.Begin(TestHost::PRIMITIVE_TRIANGLES);

            float yshift = 0.0f;
            int y = 0;
            for (int x = 160; x < 640; x += 20) {
              host_.SetTexCoord0(0.0f, 0.0f);
              host_.SetVertex(x + 0.5f, y + yshift + 0.5f, 325.0f, 325.0f);
              sample_coords.push_back(x);
              sample_coords.push_back(y + ceil(yshift));
              host_.SetTexCoord0(tu, 0.0f);
              host_.SetVertex(x + 10 + 0.5f, y + yshift + 0.5f, 325.0f, 325.0f);
              host_.SetTexCoord0(tu, tu);
              host_.SetVertex(x + 0.5f, y + yshift + 200.0f, 58.0f, 58.0f);
              yshift += 1.0f;
            }

            host_.End();
            return sample_coords;
          });
        };

        tests_[MakeTestName(kPrimVTri, depthf, zbias, zslope, vsh)] = [this, depthf, zbias, zslope, vsh]() {
          this->Test(depthf, zbias, zslope, vsh, kPrimVTri, [this]() {
            std::vector<int> sample_coords;
            float tu = 1.0;
            host_.Begin(TestHost::PRIMITIVE_TRIANGLES);

            float xshift = 0.0f;
            int x = 160;
            for (int y = 0; y < 480; y += 20) {
              host_.SetTexCoord0(0.0f, 0.0f);
              host_.SetVertex(x + xshift + 0.5f, y + 0.5f, 325.0f, 325.0f);
              sample_coords.push_back(x + ceil(xshift));
              sample_coords.push_back(y);
              host_.SetTexCoord0(tu, 0.0f);
              host_.SetVertex(x + xshift + 200.0f, y + 0.5f, 58.0f, 58.0f);
              host_.SetTexCoord0(tu, tu);
              host_.SetVertex(x + xshift + 0.5f, y + 10 + 0.5f, 325.0f, 325.0f);
              xshift += 1.0f;
            }

            host_.End();
            return sample_coords;
          });
        };
      }
    }
  }

  for (auto depthf : {0, 1, 2, 3, 4, 5, 6, 7}) {
    for (auto zbias : {false, true}) {
      for (auto zslope : {false, true}) {
        bool vsh = false;

        tests_[MakeTestName(kPrimFloorQuad, depthf, zbias, zslope, vsh)] = [this, depthf, zbias, zslope, vsh]() {
          this->Test(depthf, zbias, zslope, vsh, kPrimFloorQuad, [this]() {
            float tu = 100.0f;

            host_.Begin(TestHost::PRIMITIVE_QUADS);
            host_.SetTexCoord0(0.0f, 0.0f);
            host_.SetVertex(-15000.0f, 0.0f, 30000.0f, 1.0f);

            host_.SetTexCoord0(tu, 0.0f);
            host_.SetVertex(15000.0f, 0.0f, 30000.0f, 1.0f);

            host_.SetTexCoord0(tu, tu);
            host_.SetVertex(15000.0f, 0.0f, -10.0f, 1.0f);

            host_.SetTexCoord0(0.0f, tu);
            host_.SetVertex(-15000.0f, 0.0f, -10.0f, 1.0f);
            host_.End();
            return kVertSampleCoords;
          });
        };
      }
    }
  }

  {
    int depthf = 6;
    bool zbias = false;
    bool zslope = false;
    bool vsh = true;

    tests_[MakeTestName(kPrimRiddick, depthf, zbias, zslope, vsh)] = [this, depthf, zbias, zslope, vsh]() {
      this->Test(depthf, zbias, zslope, vsh, kPrimRiddick, [this]() {
        float tu = 1.0;

        host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
        host_.SetTexCoord0(0.0f, 0.0f);
        host_.SetVertex(-12630.8125f, 21456.1875f, 2889041.50f, 2026.6815185546875f);

        host_.SetTexCoord0(tu, 0.0f);
        host_.SetVertex(270.4375f, 266.50f, 16736755.0f, 668074.8125f);

        host_.SetTexCoord0(tu, tu);
        host_.SetVertex(317.25f, 262.375f, 16739467.0f, 714052.1875f);
        host_.End();
        return kRiddickSampleCoords;
      });
    };
  }

  for (int index = 0; index < 2; index++) {
    int depthf = 2;
    bool zbias = false;
    bool zslope = false;
    bool vsh = true;

    tests_[MakeTestName(index == 0 ? kPrimTrunc0 : kPrimTruncNeg, depthf, zbias, zslope, vsh)] =
        [this, depthf, zbias, zslope, vsh, index]() {
          this->Test(depthf, zbias, zslope, vsh, index == 0 ? kPrimTrunc0 : kPrimTruncNeg, [this, index]() {
            float tu = 1.0;
            float top = (index == 0) ? (-1.0f / 16.0f + 0.001f) : -1.0f / 16.0f;

            host_.Begin(TestHost::PRIMITIVE_QUADS);
            host_.SetTexCoord0(0.0f, tu);
            host_.SetVertex(-1168.6875f, top, 0.0f, 58.379993438720703f);

            host_.SetTexCoord0(tu, tu);
            host_.SetVertex(1808.6875f, top, 0.0f, 58.379993438720703f);

            host_.SetTexCoord0(tu, 0.0f);
            host_.SetVertex(586.75f, 487.3125f, 500000.0f, 325.8057861328125f);

            host_.SetTexCoord0(0.0f, 0.0f);
            host_.SetVertex(53.1875f, 487.3125f, 500000.0f, 325.8057861328125f);

            host_.End();
            return kVertSampleCoords;
          });
        };
  }

  for (int i = 0; i < 3; i++) {
    int depthf = 6;
    bool zbias = false;
    bool zslope = true;
    bool vsh = true;
    uint32_t clip_left = kClipLeft;
    uint32_t clip_top = kVertSampleCoords[3 + 6 * i];

    tests_[MakeTestName(kPrimFloorClip, depthf, zbias, zslope, vsh, clip_left, clip_top)] =
        [this, depthf, zbias, zslope, vsh, clip_left, clip_top]() {
          this->Test(
              depthf, zbias, zslope, vsh, kPrimFloorClip,
              [this]() {
                float tu = 1.0;

                host_.Begin(TestHost::PRIMITIVE_QUADS);
                host_.SetTexCoord0(0.0f, 0.0f);
                host_.SetVertex(53.1875f, -34.3125f, 16809768.0f, 325.8057861328125f);

                host_.SetTexCoord0(tu, 0.0f);
                host_.SetVertex(586.75f, -34.3125f, 16809768.0f, 325.8057861328125f);

                host_.SetTexCoord0(tu, tu);
                host_.SetVertex(1808.6875f, 453.0f, 16572698.0f, 58.379993438720703f);

                host_.SetTexCoord0(0.0f, tu);
                host_.SetVertex(-1168.6875f, 453.0f, 16572698.0f, 58.379993438720703f);

                host_.End();
                return kVertSampleCoords;
              },
              clip_left, clip_top);
        };
  }

  for (int i = 0; i < 3; i++) {
    int depthf = 6;
    bool zbias = false;
    bool zslope = true;
    bool vsh = true;
    uint32_t clip_left = kHorizSampleCoords[6 * i];
    uint32_t clip_top = kClipTop;

    tests_[MakeTestName(kPrimWallClip, depthf, zbias, zslope, vsh, clip_left, clip_top)] =
        [this, depthf, zbias, zslope, vsh, clip_left, clip_top]() {
          this->Test(
              depthf, zbias, zslope, vsh, kPrimWallClip,
              [this]() {
                float tu = 1.0;

                host_.Begin(TestHost::PRIMITIVE_QUADS);
                host_.SetTexCoord0(0.0f, 0.0f);
                host_.SetVertex(637.3125f, -26.8125f, 16809768.0f, 325.8057861328125f);

                host_.SetTexCoord0(tu, 0.0f);
                host_.SetVertex(637.3125f, 506.75f, 16809768.0f, 325.8057861328125f);

                host_.SetTexCoord0(tu, tu);
                host_.SetVertex(150.0f, 1728.6875f, 16572698.0f, 58.379993438720703f);

                host_.SetTexCoord0(0.0f, tu);
                host_.SetVertex(150.0f, -1248.6875f, 16572698.0f, 58.379993438720703f);

                host_.End();
                return kHorizSampleCoords;
              },
              clip_left, clip_top);
        };
  }
}

void WBufTests::Initialize() { TestSuite::Initialize(); }

void WBufTests::Deinitialize() { TestSuite::Deinitialize(); }

template <typename Func>
void WBufTests::Test(int depthf, bool zbias, bool zslope, bool vsh, const char *prim_name, Func draw_prim,
                     uint32_t clip_left, uint32_t clip_top) {
  float zbias_value = 0.0f;
  float zslope_value = 0.0f;
  bool wbuf = IsWBuf(depthf);
  int depth_bits = GetDepthBits(depthf);
  bool float_depth = IsFloatDepth(depthf);

  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, depth_bits == 16 ? TestHost::SZF_Z16 : TestHost::SZF_Z24S8,
                         host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

  if (vsh) {
    /*
    float depth_buffer_max_value = host_.GetMaxDepthBufferValue();
    auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                                            0.0f, depth_buffer_max_value, M_PI * 0.25f, 1.0f, 200.0f);
    shader->SetUseD3DStyleViewport();
    vector_t camera_position = {0.0f, 50.0f, -7.0f, 1.0f};
    vector_t camera_look_at = {0.0f, 47.4f, 0.0f, 1.0f};
    shader->LookAt(camera_position, camera_look_at);
    */

    auto shader = std::make_shared<PassthroughVertexShader>();
    host_.SetVertexShaderProgram(shader);
  } else {
    host_.SetVertexShaderProgram(nullptr);
    host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
    vector_t eye{0.0f, 50.0f, -7.0f, 1.0f};
    vector_t at{0.0f, 47.0f, 0.0f, 1.0f};
    vector_t up{0.0f, 1.0f, 0.0f, 1.0f};
    matrix4_t matrix;
    host_.BuildD3DModelViewMatrix(matrix, eye, at, up);
    host_.SetFixedFunctionModelViewMatrix(matrix);
  }

  host_.SetWindowClip(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(), clip_left, clip_top);
  host_.PrepareDraw(0xFF251135);

  auto p = pb_begin();
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_CULL_FACE_ENABLE, false);
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, true);
  p = pb_push1(p, NV097_SET_DEPTH_MASK, true);
  p = pb_push1(p, NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LEQUAL);
  p = pb_push1(p, NV097_SET_COMPRESS_ZBUFFER_EN, false);
  p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_MASK, false);
  p = pb_push1(p, NV097_SET_ZMIN_MAX_CONTROL, NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CLAMP);
  p = pb_push1f(p, NV097_SET_CLIP_MIN, 0.0f);
  p = pb_push1f(p, NV097_SET_CLIP_MAX, 16777215.0f * 16.0f);
  p = pb_push1(p, NV097_SET_CONTROL0,
               0x100000 | (wbuf ? NV097_SET_CONTROL0_Z_PERSPECTIVE_ENABLE : 0) |
                   (float_depth ? NV097_SET_CONTROL0_Z_FORMAT : 0));
  if (zbias) {
    zbias_value = kWBufferZBias;
  }
  if (zslope) {
    zslope_value = wbuf ? kWBufferZSlope : kZBufferZSlope;
  }
  p = pb_push1f(p, NV097_SET_POLYGON_OFFSET_BIAS, zbias_value);
  p = pb_push1f(p, NV097_SET_POLYGON_OFFSET_SCALE_FACTOR, zslope_value);
  p = pb_push1(p, NV097_SET_POLY_OFFSET_FILL_ENABLE, (zbias || zslope) ? 1 : 0);
  pb_end(p);

  // Generate a distinct texture.
  constexpr uint32_t kTextureSize = 256;
  {
    memset(host_.GetTextureMemory(), 0, host_.GetTextureMemorySize());
    GenerateSwizzledRGBACheckerboard(host_.GetTextureMemory(), 0, 0, kTextureSize, kTextureSize, kTextureSize * 4);

    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
    texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
    texture_stage.SetUWrap(TextureStage::WRAP_REPEAT);
    texture_stage.SetVWrap(TextureStage::WRAP_REPEAT);
    host_.SetupTextureStages();

    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
  }

  auto sample_coords = draw_prim();

  while (pb_busy()) {
    /* Wait for completion... */
  }

  {
    unsigned int depth_pitch = pb_depth_stencil_pitch();
    unsigned int depth_size = depth_pitch * host_.GetFramebufferHeight();
    auto depth_buf = std::make_unique<uint8_t[]>(depth_size);
    memcpy(depth_buf.get(), pb_agp_access(pb_depth_stencil_buffer()), depth_size);

    auto shader = std::make_shared<PassthroughVertexShader>();
    host_.SetVertexShaderProgram(shader);
    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
    host_.SetWindowClip(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
    p = pb_push1(p, NV097_SET_DEPTH_MASK, false);
    p = pb_push1f(p, NV097_SET_POLYGON_OFFSET_BIAS, 0.0f);
    p = pb_push1f(p, NV097_SET_POLYGON_OFFSET_SCALE_FACTOR, 0.0f);
    p = pb_push1(p, NV097_SET_POLY_OFFSET_FILL_ENABLE, 0);
    pb_end(p);

    const int bpp = (host_.GetDepthBufferFormat() == NV097_SET_SURFACE_FORMAT_ZETA_Z16) ? 2 : 4;
    const int leftx = (bpp == 4) ? 122 : 102;

    for (int n = 0, y = 32; n < 15; n++, y += 25) {
      if ((2 * n + 1) >= sample_coords.size()) {
        break;
      }
      int x2 = sample_coords[2 * n];
      int y2 = sample_coords[2 * n + 1];

      if (bpp == 2) {
        uint16_t val = *(uint16_t *)&depth_buf.get()[y2 * depth_pitch + x2 * bpp];
        pb_print("Z=0x%04x\n", val);
      } else {
        uint32_t val = *(uint32_t *)&depth_buf.get()[y2 * depth_pitch + x2 * bpp];
        pb_print("Z=0x%06x\n", val >> 8);
      }
      host_.Begin(TestHost::PRIMITIVE_LINES);
      host_.SetDiffuse(1.0f, 1.0f, 0.0f);
      host_.SetVertex(leftx + 0.5f, y + 0.5f, 0.0f, 1.0f);
      float ex = 0.5f / std::max(abs(x2 - leftx), abs(y2 - y));
      host_.SetVertex(x2 + 0.5f + ex * (x2 - leftx), y2 + 0.5f + ex * (y2 - y), 0.0f, 1.0f);
      host_.End();
    }

    if (clip_top != kClipTop) {
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetDiffuse(1.0f, 0.0f, 0.0f);
      host_.SetVertex(clip_left, clip_top, 0.0f, 1.0f);
      host_.SetVertex(host_.GetFramebufferWidth(), clip_top, 0.0f, 1.0f);
      host_.SetVertex(host_.GetFramebufferWidth(), clip_top + 1, 0.0f, 1.0f);
      host_.SetVertex(clip_left, clip_top + 1, 0.0f, 1.0f);
      host_.End();
    }

    if (clip_left != kClipLeft) {
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetDiffuse(1.0f, 0.0f, 0.0f);
      host_.SetVertex(clip_left, clip_top, 0.0f, 1.0f);
      host_.SetVertex(clip_left + 1, clip_top, 0.0f, 1.0f);
      host_.SetVertex(clip_left + 1, host_.GetFramebufferHeight(), 0.0f, 1.0f);
      host_.SetVertex(clip_left, host_.GetFramebufferHeight(), 0.0f, 1.0f);
      host_.End();
    }
  }

  pb_printat(10, 20, "Depth value test");
  pb_printat(11, 20, (char *)prim_name);
  pb_print(" %s%d%c", wbuf ? kTestWBuf : kTestZBuf, depth_bits, float_depth ? 'F' : 'D');
  pb_printat(12, 20, "VSH: %s", vsh ? "Prog" : "FF");
  pb_printat(13, 20, "");
  pb_print("ZBias: %.1f", zbias_value);
  pb_printat(14, 20, "");
  pb_print("ZSlope: %.1f", zslope_value);
  pb_draw_text_screen();

  std::string name = MakeTestName(prim_name, depthf, zbias, zslope, vsh, clip_left, clip_top);
  FinishDraw(name, true);
}
