#ifndef NXDK_PGRAPH_TESTS_BLEND_TESTS_H
#define NXDK_PGRAPH_TESTS_BLEND_TESTS_H

#include <functional>
#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

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
  void Test(const std::string &name, const std::function<void()> &body);

  void Body(uint32_t blend_function, uint32_t src_factor, uint32_t dst_factor);

  void DrawCheckerboardBackground() const;
  void Draw(float left, float top, float right, float bottom, uint32_t color, uint32_t func, uint32_t sfactor,
            uint32_t dfactor) const;

  void RenderToTextureStart(uint32_t stage) const;
  void RenderToTextureEnd() const;
};

#endif  // NXDK_PGRAPH_TESTS_BLEND_TESTS_H
