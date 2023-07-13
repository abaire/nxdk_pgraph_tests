#pragma once

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"
#include "xbox_math_types.h"

using namespace XboxMath;

struct TextureFormatInfo;
class VertexBuffer;

class ViewportTests : public TestSuite {
 public:
  struct Viewport {
    vector_t offset;
    vector_t scale;
  };

 public:
  ViewportTests(TestHost &host, std::string output_dir);

 private:
  void Test(const Viewport &vp);
};
