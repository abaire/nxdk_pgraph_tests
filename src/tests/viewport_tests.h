#pragma once

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"
#include "xbox_math_types.h"

using namespace XboxMath;

struct TextureFormatInfo;
class VertexBuffer;

/**
 * Tests the effects of NV097_SET_VIEWPORT_OFFSET and NV097_SET_VIEWPORT_SCALE
 * on quads rendered via the fixed function and programmable pipelines.
 */
class ViewportTests : public TestSuite {
 public:
  struct Viewport {
    vector_t offset;
    vector_t scale;
  };

 public:
  ViewportTests(TestHost &host, std::string output_dir, const Config &config);

 private:
  void Test(const Viewport &vp);
};
