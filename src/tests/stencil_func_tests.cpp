#include "stencil_func_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"
#include "vertex_buffer.h"

//! Value used for comparisons in the stencil buffer.
static constexpr uint32_t kStencilFuncRef = 0x7F;

static std::string MakeTestName(uint32_t stencil_func) {
  switch (stencil_func) {
    case NV097_SET_STENCIL_FUNC_V_NEVER:
      return "Never";
    case NV097_SET_STENCIL_FUNC_V_LESS:
      return "Less";
    case NV097_SET_STENCIL_FUNC_V_EQUAL:
      return "Equal";
    case NV097_SET_STENCIL_FUNC_V_LEQUAL:
      return "LessThanOrEqual";
    case NV097_SET_STENCIL_FUNC_V_GREATER:
      return "Greater";
    case NV097_SET_STENCIL_FUNC_V_NOTEQUAL:
      return "NotEqual";
    case NV097_SET_STENCIL_FUNC_V_GEQUAL:
      return "GreaterThanOrEqual";
    case NV097_SET_STENCIL_FUNC_V_ALWAYS:
      return "Always";

    default:
      ASSERT(!"Invalid stencil_func");
  }
}

/**
 * @tc Always
 *   Tests NV097_SET_STENCIL_FUNC_V_ALWAYS - the entire test region should be
 *   green.
 *
 * @tc Equal
 *   Tests NV097_SET_STENCIL_FUNC_V_EQUAL - only the 0x7F patch should be green.
 *
 * @tc Greater
 *   Tests NV097_SET_STENCIL_FUNC_V_GREATER - all patches less than 0x7F and the
 *   test area (stencil = 0) should be green.
 *
 * @tc GreaterThanOrEqual
 *   Tests NV097_SET_STENCIL_FUNC_V_GEQUAL - the test area (stencil = 0) and all
 *   patches less than 0x7F and the 0x7F patch should be green.
 *
 * @tc Less
 *   Tests NV097_SET_STENCIL_FUNC_V_LESS - all patches greater than 0x7F should
 *   be green.
 *
 * @tc LessThanOrEqual
 *   Tests NV097_SET_STENCIL_FUNC_V_LEQUAL - all patches greater than 0x7F and
 *   the 0x7F patch should be green.
 *
 * @tc Never
 *   Tests NV097_SET_STENCIL_FUNC_V_NEVER - all patches should be blue.
 *
 * @tc NotEqual
 *   Tests NV097_SET_STENCIL_FUNC_V_NOTEQUAL - the 0x7F patch should be blue,
 *   all other patches and the test area (stencil = 0) should be green.
 *
 */
StencilFuncTests::StencilFuncTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Stencil func", config) {
  for (auto func : {

           NV097_SET_STENCIL_FUNC_V_NEVER,
           NV097_SET_STENCIL_FUNC_V_LESS,
           NV097_SET_STENCIL_FUNC_V_EQUAL,
           NV097_SET_STENCIL_FUNC_V_LEQUAL,
           NV097_SET_STENCIL_FUNC_V_GREATER,
           NV097_SET_STENCIL_FUNC_V_NOTEQUAL,
           NV097_SET_STENCIL_FUNC_V_GEQUAL,
           NV097_SET_STENCIL_FUNC_V_ALWAYS,

       }) {
    auto name = MakeTestName(func);
    tests_[name] = [this, name, func]() { Test(name, func); };
  }
}

void StencilFuncTests::Initialize() {
  TestSuite::Initialize();
  SetDefaultTextureFormat();

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.SetSurfaceFormat(host_.GetColorBufferFormat(),
                         static_cast<TestHost::SurfaceZetaFormat>(NV097_SET_SURFACE_FORMAT_ZETA_Z24S8),
                         host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
    Pushbuffer::Push(NV097_SET_DEPTH_MASK, false);
    Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_ALWAYS);
    // If the stencil comparison fails, leave the value in the stencil buffer alone.
    Pushbuffer::Push(NV097_SET_STENCIL_OP_FAIL, NV097_SET_STENCIL_OP_V_KEEP);
    // If the stencil comparison passes but the depth comparison fails, leave the stencil buffer alone.
    Pushbuffer::Push(NV097_SET_STENCIL_OP_ZFAIL, NV097_SET_STENCIL_OP_V_KEEP);
    // If the stencil comparison passes and the depth comparison passes, leave the stencil buffer alone.
    Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_KEEP);

    // Set the reference value used by the NV097_SET_STENCIL_FUNC test.
    Pushbuffer::Push(NV097_SET_STENCIL_FUNC_REF, kStencilFuncRef);

    Pushbuffer::End();
  }
}

void StencilFuncTests::Test(const std::string &name, uint32_t stencil_func) {
  constexpr int kQuadSize = 400;

  host_.SetupControl0(true);

  // Clear the depth value to maximum and the stencil value to 0.
  host_.PrepareDraw(0xFF000000, 0x0000FFFF, 0);

  // Disable color/zeta limit errors
  auto crash_register = reinterpret_cast<uint32_t *>(PGRAPH_REGISTER_BASE + 0x880);
  auto crash_register_pre_test = *crash_register;
  *crash_register = crash_register_pre_test & (~0x800);

  {
    Pushbuffer::Begin();
    // Stencil testing must be enabled to allow anything to be written to the stencil buffer.
    Pushbuffer::Push(NV097_SET_STENCIL_TEST_ENABLE, true);
    Pushbuffer::End();
  }

  const auto kLeft = (host_.GetFramebufferWidthF() - kQuadSize) * 0.5f;
  const auto kTop = (host_.GetFramebufferHeightF() - kQuadSize) * 0.5f;

  auto left = kLeft;
  auto top = kTop;

  constexpr auto kWidth = kQuadSize / 5;
  constexpr auto kHeight = kWidth;

  left += 5.f;
  top += 4.f;

  auto draw_quad = [this, &left, &top]() {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetVertex(left, top, 0.1f, 1.0f);
    host_.SetVertex(left + kWidth, top, 0.1f, 1.0f);
    host_.SetVertex(left + kWidth, top + kHeight, 0.1f, 1.0f);
    host_.SetVertex(left, top + kHeight, 0.1f, 1.0f);
    host_.End();
  };

  auto draw_background = [this, &draw_quad]() {
    host_.SetDiffuse(0.f, 0.f, 1.f);
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_COLOR_MASK, NV097_SET_COLOR_MASK_RED_WRITE_ENABLE |
                                               NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE |
                                               NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE);

    // The value written is dictated by the STENCIL_OP_* operations. In this case, depth testing is disabled so Z will
    // always pass. The stencil value function is set to always pass (NV097_SET_STENCIL_FUNC_V_ALWAYS), so the action is
    // determined by ZPASS.
    Pushbuffer::Push(NV097_SET_STENCIL_FUNC, NV097_SET_STENCIL_FUNC_V_ALWAYS);
    // Since the stencil value was cleared to 0 above, the INVERT operation will change it to 0xFF. However,
    // NV097_SET_STENCIL_MASK can be used to prevent some bits from being modified.
    Pushbuffer::Push(NV097_SET_STENCIL_MASK, kStencilFuncRef);
    Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_INVERT);

    Pushbuffer::End();

    draw_quad();

    // Clean up for the invisible quad that will overlay this patch.
    host_.SetDiffuse(1.f, 1.f, 1.f);
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_COLOR_MASK, 0);
    Pushbuffer::Push(NV097_SET_STENCIL_MASK, 0xFF);
    Pushbuffer::End();

    // Stencil testing was enabled w/ func == "always pass", depth testing was disabled (equivalent to "always pass"),
    // so the operation chosen for OP_ZPASS was taken. Since the depth buffer was set to 0 and the operation was INVERT,
    // the value 0xFF would be written to the stencil buffer. This is masked by STENCIL_MASK, so the stencil
    // buffer should now contain kStencilFuncRef everywhere the quad was rendered. This would be more easily
    // accomplished using NV097_SET_STENCIL_OP_V_REPLACE but is done for illustrative purposes.
  };

  {
    // With colorbuffer writing disabled, update the stencil buffer with a number of quads using different masks to
    // modify the values.
    {
      Pushbuffer::Begin();
      Pushbuffer::Push(NV097_SET_COLOR_MASK, 0);

      // In this case we just want to force the value to be set via the ZPASS operation without any masking.
      Pushbuffer::Push(NV097_SET_STENCIL_MASK, 0xFF);
      Pushbuffer::End();
    }

    // Zero out a region
    {
      draw_background();
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_ZERO);
        Pushbuffer::End();
        draw_quad();
      }
      left += kWidth + 1.f;
    }

    // Set a region to the reference value - 1.
    {
      draw_background();
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_REPLACE);
        Pushbuffer::End();
        draw_quad();
      }
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_DECR);
        Pushbuffer::End();
        draw_quad();
      }

      left += kWidth + 1.f;
    }

    // Set a region to the reference value.
    {
      draw_background();
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_REPLACE);
        Pushbuffer::End();
        draw_quad();
      }

      left += kWidth + 1.f;
    }

    // Set a region to the reference value + 1.
    {
      draw_background();
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_REPLACE);
        Pushbuffer::End();
        draw_quad();
      }
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_INCR);
        Pushbuffer::End();
        draw_quad();
      }

      left += kWidth + 1.f;
    }

    // Set a region to the maximum value.
    {
      draw_background();
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_ZERO);
        Pushbuffer::End();
        draw_quad();
      }
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_INVERT);
        Pushbuffer::End();
        draw_quad();
      }

      left += kWidth + 1.f;
    }

    top += kHeight * 3.f + 2.f;
    left = kLeft + 5.f;

    // Set a region to 0 - 1 with wrapping allowed
    {
      draw_background();
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_ZERO);
        Pushbuffer::End();
        draw_quad();
      }
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_DECR);
        Pushbuffer::End();
        draw_quad();
      }

      left += kWidth + 1.f;
    }

    // Set a region to 0 - 1 clamped
    {
      draw_background();
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_ZERO);
        Pushbuffer::End();
        draw_quad();
      }
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_DECRSAT);
        Pushbuffer::End();
        draw_quad();
      }

      left += kWidth + 1.f;
    }

    // Set a region to the maximum value + 1 with wrapping allowed.
    {
      draw_background();
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_ZERO);
        Pushbuffer::End();
        draw_quad();
      }
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_INVERT);
        Pushbuffer::End();
        draw_quad();
      }
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_INCR);
        Pushbuffer::End();
        draw_quad();
      }

      left += kWidth + 1.f;
    }

    // Set a region to the maximum value + 1 clamped.
    {
      draw_background();
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_ZERO);
        Pushbuffer::End();
        draw_quad();
      }
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_INVERT);
        Pushbuffer::End();
        draw_quad();
      }
      {
        Pushbuffer::Begin();
        Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_INCRSAT);
        Pushbuffer::End();
        draw_quad();
      }

      left += kWidth + 1.f;
    }
  }

  // Finally draw a large green quad across the entire test area.
  {
    Pushbuffer::Begin();
    // Reenable writing to the color buffer.
    Pushbuffer::Push(NV097_SET_COLOR_MASK, NV097_SET_COLOR_MASK_RED_WRITE_ENABLE |
                                               NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE |
                                               NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE);

    // This isn't really relevant since this is the last draw of the test, but leave the stencil buffer alone.
    Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_KEEP);

    // Set the stencil function to the function under test.
    Pushbuffer::Push(NV097_SET_STENCIL_FUNC, stencil_func);
    Pushbuffer::End();
  }

  // Draw a green overlay across the entire patch region.
  {
    auto right = kLeft + (kWidth + 1.f) * 5.f + 5.f;
    auto bottom = kTop + (kHeight + 1.f) * 5.f + 5.f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0.f, 0.75f, 0.f);
    host_.SetVertex(kLeft, kTop, 0.1f, 1.0f);
    host_.SetVertex(right, kTop, 0.1f, 1.0f);
    host_.SetVertex(right, bottom, 0.1f, 1.0f);
    host_.SetVertex(kLeft, bottom, 0.1f, 1.0f);
    host_.End();
  }

  pb_print("%s\n", name.c_str());
  pb_printat(4, 13, (char *)"0x00");
  pb_printat(4, 21, (char *)"0x7E");
  pb_printat(4, 29, (char *)"0x7F");
  pb_printat(4, 37, (char *)"0x80");
  pb_printat(4, 45, (char *)"0xFF");
  pb_printat(8, 11, (char *)"Green if Ref:0x7F <op> Val = true");
  pb_printat(14, 13, (char *)"0xFF");
  pb_printat(14, 21, (char *)"0x00");
  pb_printat(14, 29, (char *)"0x00");
  pb_printat(14, 37, (char *)"0xFF");
  pb_draw_text_screen();

  FinishDraw(name, true);

  // Restore pgraph register 0x880
  *crash_register = crash_register_pre_test;
}
