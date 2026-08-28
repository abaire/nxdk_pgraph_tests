#ifndef NXDK_PGRAPH_TESTS_ANTIALIASING_TESTS_H
#define NXDK_PGRAPH_TESTS_ANTIALIASING_TESTS_H

#include <string>

#include "configure.h"
#include "test_host.h"
#include "test_suite.h"

namespace PBKitPlusPlus {
struct TextureFormatInfo;
class VertexBuffer;
}  // namespace PBKitPlusPlus

/**
 * Tests the effects of NV097_SET_SURFACE_FORMAT_ANTI_ALIASING in various
 * scenarios.
 *
 * Many of these tests were created to demonstrate specific bugs in xemu and are
 * not of practical value in typical hardware use cases.
 */
class AntialiasingTests : public TestSuite {
 public:
  struct Instruction {
    const char *name;
    const char *mask;
    const char *swizzle;
    const uint32_t instruction[4];
  };

 public:
  AntialiasingTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  //! Tests rendering with the specified antialiasing setting configured on a texture surface then drawn to the
  //! framebuffer.
  void Test(const char *name, TestHost::AntiAliasingSetting aa);

  //! Tests rendering directly to an antialiased framebuffer surface.
  void TestAARenderToFramebufferSurface(const char *name, TestHost::AntiAliasingSetting aa);

  //! Tests configuring antialiasing, toggling it off, performing a direct CPU write to the framebuffer, and verifying
  //! display.
  void TestAAOnThenOffThenCPUWrite();

  //! Tests that configuring antialiasing on an auxiliary surface does not affect the framebuffer.
  void TestModifyNonFramebufferSurface();

  //! Tests that direct CPU writes to the framebuffer remain independent of 3D surface configuration.
  void TestFramebufferIsIndependentOfSurface();

  //! Tests that direct CPU writes to VRAM bypass surface antialiasing configuration.
  void TestCPUWriteIgnoresSurfaceConfig();

  //! Tests GPU rendering to an antialiased surface that was initialized via CPU write.
  void TestGPUAAWriteAfterCPUWrite();

  //! Tests that non-antialiased rendering with mismatched surface pitch preserves CPU-written content.
  void TestNonAACPURoundTrip();

#ifdef ENABLE_MULTIFRAME_CPU_BLIT_TEST
  //! Tests multi-frame CPU blits to the framebuffer across surface reconfigurations.
  void TestMultiframeCPUBlit();
#endif

  void Draw() const;
  void NoOpDraw() const;
  void WaitForGPU() const;
};

#endif  // NXDK_PGRAPH_TESTS_ANTIALIASING_TESTS_H
