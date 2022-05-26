#include "vertex_shader_swizzle_tests.h"

#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "texture_generator.h"

// clang-format off
static constexpr uint32_t kShaderHeader[] = {
    /* mov oPos, v0 */
    0x00000000, 0x0020001b, 0x0836106c, 0x2070f800,
    /* mov oDiffuse, c96 */
    0x00000000, 0x002c001b, 0x0c36106c, 0x2070f818,
};
// clang-format on

static const VertexShaderSwizzleTests::Instruction kTestMov00[] = {
    {"mov", "xy", "xw", {/* mov oDiffuse.xy, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x2070c819}},
    {"mov", "xy", "zxy", {/* mov oDiffuse.xy, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x2070c819}},
    {"mov", "xy", "xzw", {/* mov oDiffuse.xy, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x2070c819}},
    {"mov", "xy", "wzy", {/* mov oDiffuse.xy, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x2070c819}},
    {"mov", "xy", "wz", {/* mov oDiffuse.xy, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x2070c819}},
    {"mov", "xy", "zx", {/* mov oDiffuse.xy, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x2070c819}},
    {"mov", "xy", "yx", {/* mov oDiffuse.xy, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x2070c819}},
    {"mov", "xy", "yzw", {/* mov oDiffuse.xy, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x2070c819}},
    {"mov", "x", "xw", {/* mov oDiffuse.x, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20708819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov01[] = {
    {"mov", "x", "zxy", {/* mov oDiffuse.x, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20708819}},
    {"mov", "x", "xzw", {/* mov oDiffuse.x, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20708819}},
    {"mov", "x", "wzy", {/* mov oDiffuse.x, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20708819}},
    {"mov", "x", "wz", {/* mov oDiffuse.x, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20708819}},
    {"mov", "x", "zx", {/* mov oDiffuse.x, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20708819}},
    {"mov", "x", "yx", {/* mov oDiffuse.x, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20708819}},
    {"mov", "x", "yzw", {/* mov oDiffuse.x, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20708819}},
    {"mov", "xw", "xw", {/* mov oDiffuse.xw, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20709819}},
    {"mov", "xw", "zxy", {/* mov oDiffuse.xw, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20709819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov02[] = {
    {"mov", "xw", "xzw", {/* mov oDiffuse.xw, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20709819}},
    {"mov", "xw", "wzy", {/* mov oDiffuse.xw, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20709819}},
    {"mov", "xw", "wz", {/* mov oDiffuse.xw, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20709819}},
    {"mov", "xw", "zx", {/* mov oDiffuse.xw, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20709819}},
    {"mov", "xw", "yx", {/* mov oDiffuse.xw, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20709819}},
    {"mov", "xw", "yzw", {/* mov oDiffuse.xw, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20709819}},
    {"mov", "xz", "xw", {/* mov oDiffuse.xz, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x2070a819}},
    {"mov", "xz", "zxy", {/* mov oDiffuse.xz, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x2070a819}},
    {"mov", "xz", "xzw", {/* mov oDiffuse.xz, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x2070a819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov03[] = {
    {"mov", "xz", "wzy", {/* mov oDiffuse.xz, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x2070a819}},
    {"mov", "xz", "wz", {/* mov oDiffuse.xz, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x2070a819}},
    {"mov", "xz", "zx", {/* mov oDiffuse.xz, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x2070a819}},
    {"mov", "xz", "yx", {/* mov oDiffuse.xz, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x2070a819}},
    {"mov", "xz", "yzw", {/* mov oDiffuse.xz, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x2070a819}},
    {"mov", "yw", "xw", {/* mov oDiffuse.yw, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20705819}},
    {"mov", "yw", "zxy", {/* mov oDiffuse.yw, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20705819}},
    {"mov", "yw", "xzw", {/* mov oDiffuse.yw, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20705819}},
    {"mov", "yw", "wzy", {/* mov oDiffuse.yw, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20705819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov04[] = {
    {"mov", "yw", "wz", {/* mov oDiffuse.yw, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20705819}},
    {"mov", "yw", "zx", {/* mov oDiffuse.yw, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20705819}},
    {"mov", "yw", "yx", {/* mov oDiffuse.yw, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20705819}},
    {"mov", "yw", "yzw", {/* mov oDiffuse.yw, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20705819}},
    {"mov", "y", "xw", {/* mov oDiffuse.y, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20704819}},
    {"mov", "y", "zxy", {/* mov oDiffuse.y, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20704819}},
    {"mov", "y", "xzw", {/* mov oDiffuse.y, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20704819}},
    {"mov", "y", "wzy", {/* mov oDiffuse.y, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20704819}},
    {"mov", "y", "wz", {/* mov oDiffuse.y, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20704819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov05[] = {
    {"mov", "y", "zx", {/* mov oDiffuse.y, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20704819}},
    {"mov", "y", "yx", {/* mov oDiffuse.y, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20704819}},
    {"mov", "y", "yzw", {/* mov oDiffuse.y, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20704819}},
    {"mov", "yz", "xw", {/* mov oDiffuse.yz, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20706819}},
    {"mov", "yz", "zxy", {/* mov oDiffuse.yz, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20706819}},
    {"mov", "yz", "xzw", {/* mov oDiffuse.yz, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20706819}},
    {"mov", "yz", "wzy", {/* mov oDiffuse.yz, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20706819}},
    {"mov", "yz", "wz", {/* mov oDiffuse.yz, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20706819}},
    {"mov", "yz", "zx", {/* mov oDiffuse.yz, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20706819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov06[] = {
    {"mov", "yz", "yx", {/* mov oDiffuse.yz, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20706819}},
    {"mov", "yz", "yzw", {/* mov oDiffuse.yz, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20706819}},
    {"mov", "z", "xw", {/* mov oDiffuse.z, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20702819}},
    {"mov", "z", "zxy", {/* mov oDiffuse.z, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20702819}},
    {"mov", "z", "xzw", {/* mov oDiffuse.z, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20702819}},
    {"mov", "z", "wzy", {/* mov oDiffuse.z, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20702819}},
    {"mov", "z", "wz", {/* mov oDiffuse.z, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20702819}},
    {"mov", "z", "zx", {/* mov oDiffuse.z, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20702819}},
    {"mov", "z", "yx", {/* mov oDiffuse.z, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20702819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov07[] = {
    {"mov", "z", "yzw", {/* mov oDiffuse.z, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20702819}},
    {"mov", "zw", "xw", {/* mov oDiffuse.zw, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20703819}},
    {"mov", "zw", "zxy", {/* mov oDiffuse.zw, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20703819}},
    {"mov", "zw", "xzw", {/* mov oDiffuse.zw, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20703819}},
    {"mov", "zw", "wzy", {/* mov oDiffuse.zw, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20703819}},
    {"mov", "zw", "wz", {/* mov oDiffuse.zw, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20703819}},
    {"mov", "zw", "zx", {/* mov oDiffuse.zw, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20703819}},
    {"mov", "zw", "yx", {/* mov oDiffuse.zw, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20703819}},
    {"mov", "zw", "yzw", {/* mov oDiffuse.zw, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20703819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov08[] = {
    {"mov", "xyz", "xw", {/* mov oDiffuse.xyz, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x2070e819}},
    {"mov", "xyz", "zxy", {/* mov oDiffuse.xyz, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x2070e819}},
    {"mov", "xyz", "xzw", {/* mov oDiffuse.xyz, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x2070e819}},
    {"mov", "xyz", "wzy", {/* mov oDiffuse.xyz, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x2070e819}},
    {"mov", "xyz", "wz", {/* mov oDiffuse.xyz, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x2070e819}},
    {"mov", "xyz", "zx", {/* mov oDiffuse.xyz, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x2070e819}},
    {"mov", "xyz", "yx", {/* mov oDiffuse.xyz, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x2070e819}},
    {"mov", "xyz", "yzw", {/* mov oDiffuse.xyz, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x2070e819}},
    {"mov", "xyzw", "xw", {/* mov oDiffuse, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x2070f819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov09[] = {
    {"mov", "xyzw", "zxy", {/* mov oDiffuse, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x2070f819}},
    {"mov", "xyzw", "xzw", {/* mov oDiffuse, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x2070f819}},
    {"mov", "xyzw", "wzy", {/* mov oDiffuse, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x2070f819}},
    {"mov", "xyzw", "wz", {/* mov oDiffuse, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x2070f819}},
    {"mov", "xyzw", "zx", {/* mov oDiffuse, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x2070f819}},
    {"mov", "xyzw", "yx", {/* mov oDiffuse, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x2070f819}},
    {"mov", "xyzw", "yzw", {/* mov oDiffuse, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x2070f819}},
    {"mov", "w", "xw", {/* mov oDiffuse.w, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20701819}},
    {"mov", "w", "zxy", {/* mov oDiffuse.w, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20701819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov10[] = {
    {"mov", "w", "xzw", {/* mov oDiffuse.w, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20701819}},
    {"mov", "w", "wzy", {/* mov oDiffuse.w, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20701819}},
    {"mov", "w", "wz", {/* mov oDiffuse.w, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20701819}},
    {"mov", "w", "zx", {/* mov oDiffuse.w, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20701819}},
    {"mov", "w", "yx", {/* mov oDiffuse.w, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20701819}},
    {"mov", "w", "yzw", {/* mov oDiffuse.w, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20701819}},
    {"mov", "yzw", "xw", {/* mov oDiffuse.yzw, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20707819}},
    {"mov", "yzw", "zxy", {/* mov oDiffuse.yzw, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20707819}},
    {"mov", "yzw", "xzw", {/* mov oDiffuse.yzw, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20707819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov11[] = {
    {"mov", "yzw", "wzy", {/* mov oDiffuse.yzw, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20707819}},
    {"mov", "yzw", "wz", {/* mov oDiffuse.yzw, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20707819}},
    {"mov", "yzw", "zx", {/* mov oDiffuse.yzw, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20707819}},
    {"mov", "yzw", "yx", {/* mov oDiffuse.yzw, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20707819}},
    {"mov", "yzw", "yzw", {/* mov oDiffuse.yzw, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20707819}},
};

static const VertexShaderSwizzleTests::Instruction *kTests[] = {kTestMov00, kTestMov01, kTestMov02, kTestMov03,
                                                                kTestMov04, kTestMov05, kTestMov06, kTestMov07,
                                                                kTestMov08, kTestMov09, kTestMov10, kTestMov11};
static const uint32_t kTestSizes[] = {
    sizeof(kTestMov00) / sizeof(kTestMov00[0]), sizeof(kTestMov01) / sizeof(kTestMov01[0]),
    sizeof(kTestMov02) / sizeof(kTestMov02[0]), sizeof(kTestMov03) / sizeof(kTestMov03[0]),
    sizeof(kTestMov04) / sizeof(kTestMov04[0]), sizeof(kTestMov05) / sizeof(kTestMov05[0]),
    sizeof(kTestMov06) / sizeof(kTestMov06[0]), sizeof(kTestMov07) / sizeof(kTestMov07[0]),
    sizeof(kTestMov08) / sizeof(kTestMov08[0]), sizeof(kTestMov09) / sizeof(kTestMov09[0]),
    sizeof(kTestMov10) / sizeof(kTestMov10[0]), sizeof(kTestMov11) / sizeof(kTestMov11[0]),
};

static constexpr uint32_t kCheckerboardA = 0xFF202020;
static constexpr uint32_t kCheckerboardB = 0xFF000000;

VertexShaderSwizzleTests::VertexShaderSwizzleTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Vertex shader swizzle tests") {
  char buf[32] = {0};
  uint32_t index = 0;
  for (auto &group : kTests) {
    snprintf(buf, sizeof(buf), "group%02d", index);
    std::string name = buf;
    tests_[name] = [this, name, index]() { Test(name, index); };
    ++index;
  }
}

void VertexShaderSwizzleTests::Initialize() {
  TestSuite::Initialize();

  host_.SetCombinerControl(1);
  host_.SetAlphaBlendEnabled();

  // Allocate enough space for the shader header and the final instruction.
  shader_code_size_ = sizeof(kShaderHeader) + 16;
  shader_code_ = new uint32_t[shader_code_size_];
  memcpy(shader_code_, kShaderHeader, sizeof(kShaderHeader));
}

void VertexShaderSwizzleTests::Deinitialize() {
  TestSuite::Deinitialize();
  delete[] shader_code_;
}

void VertexShaderSwizzleTests::Test(const std::string &name, uint32_t index) {
  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.PrepareDraw(0xFE312F31);

  DrawCheckerboardBackground();

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  constexpr float kWidth = 96.0f;
  constexpr float kHeight = 96.0f;
  constexpr float z = 0.0f;

  const float kStartHorizontal = (host_.GetFramebufferWidthF() - kWidth * 3) * 0.25f;
  const float kStartVertical = (host_.GetFramebufferHeightF() - kHeight * 3) * 0.25f + 10.0f;
  const float kEndHorizontal = host_.GetFramebufferWidthF() - kWidth;
  float left = kStartHorizontal;
  float top = kStartVertical;

  const Instruction *instructions = kTests[index];
  uint32_t count = kTestSizes[index];
  for (uint32_t i = 0; i < count; ++i) {
    memcpy(shader_code_ + sizeof(kShaderHeader) / 4, instructions[i].instruction, sizeof(instructions[i].instruction));
    shader->SetShaderOverride(shader_code_, shader_code_size_);
    // Set the default color for unset components.
    shader->SetUniformF(0, 0.25f, 0.25f, 0.25f, 0.25f);
    shader->Activate();
    shader->PrepareDraw();
    host_.SetVertexShaderProgram(shader);

    const float right = left + kWidth;
    const float bottom = top + kHeight;

    constexpr float kRed = 0.25f;
    constexpr float kGreen = 0.5f;
    constexpr float kBlue = 0.75f;
    constexpr float kAlpha = 1.0f;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    // Set RGBA to a distinct pattern.
    host_.SetDiffuse(kRed, kGreen, kBlue, kAlpha);
    host_.SetVertex(left, top, z, 1.0f);
    host_.SetVertex(right, top, z, 1.0f);
    host_.SetVertex(right, bottom, z, 1.0f);
    host_.SetVertex(left, bottom, z, 1.0f);
    host_.End();

    left += kWidth + kStartHorizontal;
    if (left >= kEndHorizontal) {
      left = kStartHorizontal;
      top += kStartVertical + kHeight;
    }
  }

  pb_print("%s", name.c_str());

  constexpr int32_t kTextLeft = 10;
  constexpr int32_t kTextStrideHorizontal = 18;
  constexpr int32_t kTextStrideVertical = 6;
  constexpr int32_t kTextRight = 50;
  int32_t row = 0;
  int32_t col = kTextLeft;
  for (auto i = 0; i < count; ++i) {
    pb_printat(row, col, (char *)"%s_%s", instructions[i].mask, instructions[i].swizzle);
    col += kTextStrideHorizontal;
    if (col > kTextRight) {
      col = kTextLeft;
      row += kTextStrideVertical;
    }
  }

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}

void VertexShaderSwizzleTests::DrawCheckerboardBackground() const {
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  constexpr uint32_t kTextureSize = 256;
  constexpr uint32_t kCheckerSize = 24;

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
  host_.SetupTextureStages();

  GenerateSwizzledRGBACheckerboard(host_.GetTextureMemory(), 0, 0, kTextureSize, kTextureSize, kTextureSize * 4,
                                   kCheckerboardA, kCheckerboardB, kCheckerSize);

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetTexCoord0(0.0f, 0.0f);
  host_.SetVertex(0.0f, 0.0f, 0.1f, 1.0f);
  host_.SetTexCoord0(1.0f, 0.0f);
  host_.SetVertex(host_.GetFramebufferWidthF(), 0.0f, 0.1f, 1.0f);
  host_.SetTexCoord0(1.0f, 1.0f);
  host_.SetVertex(host_.GetFramebufferWidthF(), host_.GetFramebufferHeightF(), 0.1f, 1.0f);
  host_.SetTexCoord0(0.0f, 1.0f);
  host_.SetVertex(0.0f, host_.GetFramebufferHeightF(), 0.1f, 1.0f);
  host_.End();

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
}
