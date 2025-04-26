#include "color_mask_blend_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "shaders/passthrough_vertex_shader.h"
#include "vertex_buffer.h"

typedef struct TestCase {
  uint32_t color_mask;
  uint32_t blend_op;
  uint32_t sfactor;
  uint32_t dfactor;
} TestCase;

static constexpr TestCase kTestCases[] = {
    {NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE | NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE |
         NV097_SET_COLOR_MASK_RED_WRITE_ENABLE,
     NV097_SET_BLEND_EQUATION_V_FUNC_ADD, NV097_SET_BLEND_FUNC_SFACTOR_V_DST_ALPHA,
     NV097_SET_BLEND_FUNC_DFACTOR_V_ZERO},
};

static std::string MakeTestName(const TestCase &test_case) {
  char buf[32] = {0};
  snprintf(buf, 31, "C%08X_O%d_S%d_D%d", test_case.color_mask, test_case.blend_op, test_case.sfactor,
           test_case.dfactor);
  return buf;
}

ColorMaskBlendTests::ColorMaskBlendTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Color mask blend", config) {
  for (auto &test_case : kTestCases) {
    std::string name = MakeTestName(test_case);
    tests_[name] = [this, name, test_case]() {
      Test(test_case.color_mask, test_case.blend_op, test_case.sfactor, test_case.dfactor, name);
    };
  }
}

void ColorMaskBlendTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  CreateGeometry();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BLEND_ENABLE, true);
  Pushbuffer::End();
}

void ColorMaskBlendTests::CreateGeometry() {
  uint32_t num_quads = 4;
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6 * num_quads);

  Color ul{1.0, 0.0, 0.0, 0.5};
  Color ll{0.0, 1.0, 0.0, 0.75};
  Color lr{0.0, 0.0, 1.0, 0.9};
  Color ur{0.5, 0.5, 0.5, 1.0};

  auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  float width = floorf(fb_width / (1.0f + 2.0f * static_cast<float>(num_quads)));
  float height = floorf(fb_height / 4.0f);

  float x = width;
  float y = floorf(fb_height * 0.5f) - (height * 0.5f);
  float z = 10.0f;

  for (auto i = 0; i < num_quads; ++i) {
    buffer->DefineBiTri(i, x, y, x + width, y + height, z, z, z, z, ul, ll, lr, ur);
    x += width * 2.0f;
    z -= 0.5f;

    // Fade alpha for each subsequent quad.
    ul.a *= 0.75;
    ll.a *= 0.75;
    lr.a *= 0.75;
    ur.a *= 0.75;
  }
}

void ColorMaskBlendTests::Test(uint32_t color_mask, uint32_t blend_op, uint32_t sfactor, uint32_t dfactor,
                               const std::string &test_name) {
  host_.PrepareDraw(0x30222222);

  // Draw some quads to modify the alpha value in the buffer
  {
    const float left = 20.0f;
    const float right = static_cast<float>(host_.GetFramebufferWidth()) * 0.5f;
    const float top = 120.0f;
    const float bottom = static_cast<float>(host_.GetFramebufferHeight()) - top;
    const float z = 20.0f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xCCFFFFFF);
    host_.SetVertex(left, top, z, 1.0f);
    host_.SetVertex(right, top, z, 1.0f);
    host_.SetVertex(right, bottom, z, 1.0f);
    host_.SetVertex(left, bottom, z, 1.0f);
    host_.End();

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFF000000);
    host_.SetVertex(left + (right * 0.5f), top + 10.0f, z, 1.0f);
    host_.SetVertex(right - 10.0f, top + 10.0f, z, 1.0f);
    host_.SetVertex(right - 10.0f, bottom - 10.0f, z, 1.0f);
    host_.SetVertex(left + (right * 0.5f), bottom - 10.0f, z, 1.0f);
    host_.End();

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0x227799FF);
    host_.SetVertex(right + 10.0f, top + 10.0f, z, 1.0f);
    host_.SetVertex(right - 10.0f + (right * 0.5f), top + 10.0f, z, 1.0f);
    host_.SetVertex(right - 10.0f + (right * 0.5f), bottom - 10.0f, z, 1.0f);
    host_.SetVertex(right + 10.0f, bottom - 10.0f, z, 1.0f);
    host_.End();
  }

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_COLOR_MASK, color_mask);
  Pushbuffer::Push(NV097_SET_BLEND_EQUATION, blend_op);
  Pushbuffer::Push(NV097_SET_BLEND_FUNC_SFACTOR, sfactor);
  Pushbuffer::Push(NV097_SET_BLEND_FUNC_DFACTOR, dfactor);
  Pushbuffer::End();

  host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE);

  pb_print("C: 0x%08X\n", color_mask);
  pb_print("Op: %d\n", blend_op);
  pb_print("Src: %d\n", sfactor);
  pb_print("Dst: %d\n", dfactor);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, test_name);
}
