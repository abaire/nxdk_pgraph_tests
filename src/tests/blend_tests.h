#ifndef NXDK_PGRAPH_TESTS_BLEND_TESTS_H
#define NXDK_PGRAPH_TESTS_BLEND_TESTS_H

#include <functional>
#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests the effects of NV097_SET_BLEND_EQUATION, NV097_SET_BLEND_FUNC_SFACTOR, and NV097_SET_BLEND_FUNC_DFACTOR on
 * rendering unsigned textures.
 *
 * All test names are of the form <sfactor>-<equation>-<dfactor> where:
 *
 * sfactor/dfactor are one of:
 * 0 - ZERO,
 * 1 - ONE,
 * srcRGB - SRC_COLOR,
 * 1-srcRGB - ONE_MINUS_SRC_COLOR,
 * srcA - SRC_ALPHA,
 * 1-srcA - ONE_MINUS_SRC_ALPHA,
 * dstA - DST_ALPHA,
 * 1-dstA - ONE_MINUS_DST_ALPHA,
 * dstRGB - DST_COLOR,
 * 1-dstRGB - ONE_MINUS_DST_COLOR,
 * srcAsat - SRC_ALPHA_SATURATE,
 * cRGB - CONSTANT_COLOR,
 * 1-cRGB - ONE_MINUS_CONSTANT_COLOR,
 * cA - CONSTANT_ALPHA,
 * 1-cA - ONE_MINUS_CONSTANT_ALPHA
 *
 * equation is one of:
 * ADD - NV097_SET_BLEND_EQUATION_V_FUNC_ADD,
 * SUB - NV097_SET_BLEND_EQUATION_V_FUNC_SUBTRACT,
 * REVSUB - NV097_SET_BLEND_EQUATION_V_FUNC_REVERSE_SUBTRACT,
 * MIN - NV097_SET_BLEND_EQUATION_V_MIN,
 * MAX - NV097_SET_BLEND_EQUATION_V_MAX,
 * SADD - NV097_SET_BLEND_EQUATION_V_FUNC_ADD_SIGNED,
 * SREVSUB - NV097_SET_BLEND_EQUATION_V_FUNC_REVERSE_SUBTRACT_SIGNED,
 *
 * The left, center, and right renders are all composited against a 16 pixel checkerboard pattern alternating between a
 * background color and fully opaque black.
 *
 * On the left side of the screen, a column of squares is rendered with colors blended with the checkerboard and alpha
 * always set to 0xDD.
 *
 * In the center of the screen, a stack of concentric squares is rendered with colors that are not blended and alpha
 * values blended with the checkerboard (the first 16x16 will be blended with the BG color, the next with full opaque
 * black, etc...). Each square is composited with the previous square from outside to inside, so the outermost is only
 * blended with the checkerboard, the next is blended with both the checkerboard and the result of the outermost blend,
 * etc...
 *
 * On the right side of the screen, a column of squares is rendered with both color and alpha values blended.
 */
class BlendTests : public TestSuite {
 public:
  struct Instruction {
    const char *name;
    const char *mask;
    const char *swizzle;
    const uint32_t instruction[4];
  };

 public:
  BlendTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  //! Tests interactions of NV097_SET_BLEND_EQUATION, NV097_SET_BLEND_FUNC_SFACTOR, and NV097_SET_BLEND_FUNC_DFACTOR.
  void Test(const std::string &name, uint32_t blend_function, uint32_t src_factor, uint32_t dst_factor);

  //! Renders a series of concentric squares in green, red, blue, and white with their alpha channels blended.
  void DrawAlphaStack(uint32_t blend_function, uint32_t src_factor, uint32_t dst_factor);

  //! Renders a column of squares in green, red, blue, with their color channels blended.
  void DrawColorStack(uint32_t blend_function, uint32_t src_factor, uint32_t dst_factor);

  //! Renders a column of squares in green, red, blue, with their color and alpha channels blended.
  void DrawColorAndAlphaStack(uint32_t blend_function, uint32_t src_factor, uint32_t dst_factor);

  void DrawCheckerboardBackground() const;
  //! Renders a quad where either the alpha channel or color channels are modulated by the given blend settings
  //! depending on `blend_alpha`.
  void DrawQuad(float left, float top, float right, float bottom, uint32_t color, uint32_t func, uint32_t sfactor,
                uint32_t dfactor, bool blend_rgb, bool blend_alpha) const;

  void RenderToTextureStart(uint32_t stage, uint32_t texture_pitch) const;
  void RenderToTextureEnd() const;
};

#endif  // NXDK_PGRAPH_TESTS_BLEND_TESTS_H
