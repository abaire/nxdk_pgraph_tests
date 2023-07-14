#ifndef NXDK_PGRAPH_TESTS_ANTIALIASING_TESTS_H
#define NXDK_PGRAPH_TESTS_ANTIALIASING_TESTS_H

#include <functional>
#include <memory>
#include <string>

#include "configure.h"
#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

class AntialiasingTests : public TestSuite {
 public:
  struct Instruction {
    const char *name;
    const char *mask;
    const char *swizzle;
    const uint32_t instruction[4];
  };

 public:
  AntialiasingTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void Test(const char *name, TestHost::AntiAliasingSetting aa);
  void TestAARenderToFramebufferSurface(const char *name, TestHost::AntiAliasingSetting aa);
  void TestAAOnThenOffThenCPUWrite();
  void TestModifyNonFramebufferSurface();
  void TestFramebufferIsIndependentOfSurface();
  void TestCPUWriteIgnoresSurfaceConfig();
  void TestGPUAAWriteAfterCPUWrite();
  void TestNonAACPURoundTrip();

#ifdef ENABLE_MULTIFRAME_CPU_BLIT_TEST
  // This test is only useful when viewing live and should generally be disabled.
  void TestMultiframeCPUBlit();
#endif

  void Draw() const;
  void NoOpDraw() const;
  void WaitForGPU() const;
};

#endif  // NXDK_PGRAPH_TESTS_ANTIALIASING_TESTS_H
