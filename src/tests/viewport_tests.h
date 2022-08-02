#pragma once

#include <memory>
#include <string>

#include "math3d.h"
#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

class ViewportTests : public TestSuite {
 public:
  struct Viewport {
    VECTOR offset;
    VECTOR scale;
  };

 public:
  ViewportTests(TestHost &host, std::string output_dir);

 private:
  void Test(const Viewport &vp);
};
