#include "texture_brdf_tests.h"

#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "shaders/perspective_vertex_shader.h"
#include "test_host.h"
#include "texture_format.h"
#include "xbox-swizzle/swizzle.h"
#include "xbox_math_matrix.h"
#include "xbox_math_types.h"

using namespace XboxMath;

static constexpr float kSize = 1.0f;

static constexpr uint32_t kRightSide[] = {3, 7, 6, 2};
static constexpr uint32_t kLeftSide[] = {1, 5, 4, 0};
static constexpr uint32_t kTopSide[] = {4, 5, 6, 7};
static constexpr uint32_t kBottomSide[] = {1, 0, 3, 2};
static constexpr uint32_t kBackSide[] = {2, 6, 5, 1};
static constexpr uint32_t kFrontSide[] = {0, 4, 7, 3};

// clang-format off
static constexpr const uint32_t *kCubeFaces[] = {
  kRightSide,
  kLeftSide,
  kTopSide,
  kBottomSide,
  kBackSide,
  kFrontSide,
};

static constexpr float kCubePoints[][3] = {
  {-kSize, -kSize, -kSize},
  {-kSize, -kSize, kSize},
  {kSize, -kSize, kSize},
  {kSize, -kSize, -kSize},
  {-kSize, kSize, -kSize},
  {-kSize, kSize, kSize},
  {kSize, kSize, kSize},
  {kSize, kSize, -kSize},
};

// clang-format on

static constexpr uint32_t kTextureWidth = 64;
static constexpr uint32_t kTexturePitch = kTextureWidth * 4;
static constexpr uint32_t kTextureHeight = 64;
static constexpr uint32_t kTextureDepth = 64;

static void GenerateSphericalCoordMap(uint8_t *buffer);
static void GenerateBRDFFunc(uint8_t *buffer);
static std::string MakeTestName(bool stage0_blank, bool stage1_blank);

TextureBRDFTests::TextureBRDFTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texture BRDF", config) {
  tests_[MakeTestName(false, false)] = [this]() { Test(false, false); };
  tests_[MakeTestName(false, true)] = [this]() { Test(false, true); };
  tests_[MakeTestName(true, false)] = [this]() { Test(true, false); };
}

void TextureBRDFTests::Initialize() {
  TestSuite::Initialize();

  float depth_buffer_max_value = host_.GetMaxDepthBufferValue();
  auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                                          0.0f, depth_buffer_max_value, M_PI * 0.25f, 1.0f, 200.0f);
  {
    shader->SetUseD3DStyleViewport();
    vector_t camera_position = {0.0f, 0.0f, -7.0f, 1.0f};
    vector_t camera_look_at = {0.0f, 0.0f, 0.0f, 1.0f};
    shader->LookAt(camera_position, camera_look_at);
  }
  host_.SetVertexShaderProgram(shader);

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  // load the BRDF function volume texture into stage2
  {
    GenerateBRDFFunc(host_.GetTextureMemoryForStage(2));
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8), 2);
    auto &stage = host_.GetTextureStage(2);
    stage.SetTextureDimensions(kTextureWidth, kTextureHeight, kTextureDepth);
    stage.SetImageDimensions(kTextureWidth, kTextureHeight, kTextureDepth);
    stage.SetPWrap(TextureStage::WRAP_REPEAT, true);
  }

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, true);
  p = pb_push1(p, NV097_SET_CONTROL0, 0x100001);
  pb_end(p);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX2);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX2, true);
}

void TextureBRDFTests::Test(bool stage0_blank, bool stage1_blank) {
  // Load the spherical coordinate transform cube map into stage0
  {
    if (stage0_blank) {
      memset(host_.GetTextureMemoryForStage(0), 0, kTexturePitch * kTextureHeight);
    } else {
      GenerateSphericalCoordMap(host_.GetTextureMemoryForStage(0));
    }
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R16B16), 0);
    auto &stage = host_.GetTextureStage(0);
    stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    stage.SetCubemapEnable();
  }

  // Load the spherical coordinate transform cube map into stage1
  {
    if (stage1_blank) {
      memset(host_.GetTextureMemoryForStage(1), 0, kTexturePitch * kTextureHeight);
    } else {
      GenerateSphericalCoordMap(host_.GetTextureMemoryForStage(1));
    }
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R16B16), 1);
    auto &stage = host_.GetTextureStage(1);
    stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    stage.SetCubemapEnable();
  }

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, true);
  host_.SetTextureStageEnabled(2, true);
  host_.SetShaderStageProgram(TestHost::STAGE_CUBE_MAP, TestHost::STAGE_CUBE_MAP, TestHost::STAGE_BRDF);
  host_.SetupTextureStages();

  host_.PrepareDraw(0xFE121212);

  auto shader = std::static_pointer_cast<PerspectiveVertexShader>(host_.GetShaderProgram());

  auto draw = [this, &shader](float x, float y, float z, float r_x, float r_y, float r_z) {
    matrix4_t matrix;
    vector_t eye{0.0f, 0.0f, -7.0f, 1.0f};
    vector_t at{0.0f, 0.0f, 0.0f, 1.0f};
    vector_t up{0.0f, 1.0f, 0.0f, 1.0f};
    TestHost::BuildD3DModelViewMatrix(matrix, eye, at, up);

    auto &model_matrix = shader->GetModelMatrix();
    MatrixSetIdentity(model_matrix);
    vector_t rotation = {r_x, r_y, r_z};
    MatrixRotate(model_matrix, rotation);
    vector_t translation = {x, y, z};
    MatrixTranslate(model_matrix, translation);

    matrix4_t mv_matrix;
    MatrixMultMatrix(matrix, shader->GetModelMatrix(), mv_matrix);
    host_.SetFixedFunctionModelViewMatrix(mv_matrix);

    shader->PrepareDraw();

    host_.Begin(TestHost::PRIMITIVE_QUADS);

    for (auto face : kCubeFaces) {
      for (auto i = 0; i < 4; ++i) {
        uint32_t index = face[i];
        float lightz = 1.0f;

        for (auto vindex : kFrontSide) {
          if (index == vindex) {
            lightz = -1.0f;
          }
        }

        const float *vertex = kCubePoints[index];
        host_.SetTexCoord0(vertex[0], vertex[1], vertex[2], 1.0f);
        host_.SetTexCoord1(0.1f, 0.05f, lightz, 1.0f);
        host_.SetVertex(vertex[0], vertex[1], vertex[2], 1.0f);
      }
    }

    host_.End();
  };

  const float z = 2.0f;
  draw(-1.5f, 0.0f, z, M_PI * 0.25f, M_PI * 0.25f, 0.0f);
  draw(1.5f, 0.0f, z, M_PI * 1.25f, M_PI * 0.25f, 0.0f);

  std::string test_name = MakeTestName(stage0_blank, stage1_blank);
  pb_print("%s\n", "BRDF");
  pb_print("eye tex blank: %d\n", stage0_blank);
  pb_print("light tex blank: %d\n", stage1_blank);
  pb_draw_text_screen();

  FinishDraw(test_name);
}

static void GenerateSphericalCoordMap(uint8_t *buffer) {
  static constexpr uint32_t kSliceSize = kTexturePitch * kTextureHeight;
  float dx = 2.0f / kTextureWidth;
  float dy = 2.0f / kTextureHeight;

  uint8_t *temp_buffer = new uint8_t[kSliceSize];

  for (int face = 0; face < 6; face++) {
    uint8_t *dest = temp_buffer;

    for (int j = 0; j < kTextureHeight; j++) {
      float y = (0.5f + j) * dy - 1.0f;

      for (int i = 0; i < kTextureWidth; i++) {
        float x = (0.5f + i) * dx - 1.0f;
        float vx, vy, vz;

        switch (face) {
          case 0:  // +X
            vx = 1.0f;
            vy = -y;
            vz = -x;
            break;
          case 1:  // -X
            vx = -1.0f;
            vy = -y;
            vz = x;
            break;
          case 2:  // +Y
            vx = x;
            vy = 1.0f;
            vz = y;
            break;
          case 3:  // -Y
            vx = x;
            vy = -1.0f;
            vz = -y;
            break;
          case 4:  // +Z
            vx = x;
            vy = -y;
            vz = 1.0f;
            break;
          case 5:  // -Z
            vx = -x;
            vy = -y;
            vz = -1.0f;
            break;
        }

        float theta = acosf(vz / sqrtf(vx * vx + vy * vy + vz * vz));
        float phi = atan2f(vy, vx);
        if (phi < 0.0f) {
          // The result range of atan2f is [-pi, pi]
          phi += 2.0f * M_PI;
        }

        unsigned int itheta = (theta / M_PI) * 65535.0f + 0.5f;
        unsigned int iphi = phi / (2.0f * M_PI) * 65535.0f + 0.5f;

        *dest++ = iphi & 0xFF;
        *dest++ = iphi >> 8;
        *dest++ = itheta & 0xFF;
        *dest++ = itheta >> 8;
      }
    }

    swizzle_rect(temp_buffer, kTextureWidth, kTextureHeight, buffer, kTexturePitch, 4);
    buffer += kSliceSize;
  }

  delete[] temp_buffer;
}

static void GenerateBRDFFunc(uint8_t *buffer) {
  uint8_t *temp_buffer = new uint8_t[kTexturePitch * kTextureHeight * kTextureDepth];
  uint8_t *dest = temp_buffer;

  for (int z = 0; z < kTextureDepth; z++) {
    for (int y = 0; y < kTextureHeight; y++) {
      for (int x = 0; x < kTextureWidth; x++) {
        *dest++ = z * 255 / (kTextureDepth - 1);
        *dest++ = y * 255 / (kTextureHeight - 1);
        *dest++ = x * 255 / (kTextureWidth - 1);
        *dest++ = 255;
      }
    }
  }

  swizzle_box(temp_buffer, kTextureWidth, kTextureHeight, kTextureDepth, buffer, kTexturePitch,
              kTexturePitch * kTextureHeight, 4);
  delete[] temp_buffer;
}

static std::string MakeTestName(bool stage0_blank, bool stage1_blank) {
  std::string test_name = "BRDF";
  test_name += stage0_blank ? "_e1" : "_e0";
  test_name += stage1_blank ? "_l1" : "_l0";
  return test_name;
}
