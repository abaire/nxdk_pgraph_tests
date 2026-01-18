#include "vertex_shader_swizzle_tests.h"

#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "shaders/passthrough_vertex_shader.h"
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
    {"mov", "ra", "xw", {/* mov oDiffuse.xw, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20709819}},
    {"mov", "ra", "zy", {/* mov oDiffuse.xw, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x20709819}},
    {"mov", "ra", "yw", {/* mov oDiffuse.xw, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x20709819}},
    {"mov", "ra", "yzw", {/* mov oDiffuse.xw, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20709819}},
    {"mov", "ra", "yx", {/* mov oDiffuse.xw, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20709819}},
    {"mov", "ra", "wzy", {/* mov oDiffuse.xw, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20709819}},
    {"mov", "ra", "xzw", {/* mov oDiffuse.xw, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20709819}},
    {"mov", "ra", "zx", {/* mov oDiffuse.xw, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20709819}},
    {"mov", "ra", "zxy", {/* mov oDiffuse.xw, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20709819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov01[] = {
    {"mov", "ra", "wz", {/* mov oDiffuse.xw, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20709819}},
    {"mov", "r", "xw", {/* mov oDiffuse.x, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20708819}},
    {"mov", "r", "zy", {/* mov oDiffuse.x, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x20708819}},
    {"mov", "r", "yw", {/* mov oDiffuse.x, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x20708819}},
    {"mov", "r", "yzw", {/* mov oDiffuse.x, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20708819}},
    {"mov", "r", "yx", {/* mov oDiffuse.x, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20708819}},
    {"mov", "r", "wzy", {/* mov oDiffuse.x, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20708819}},
    {"mov", "r", "xzw", {/* mov oDiffuse.x, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20708819}},
    {"mov", "r", "zx", {/* mov oDiffuse.x, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20708819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov02[] = {
    {"mov", "r", "zxy", {/* mov oDiffuse.x, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20708819}},
    {"mov", "r", "wz", {/* mov oDiffuse.x, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20708819}},
    {"mov", "gb", "xw", {/* mov oDiffuse.yz, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20706819}},
    {"mov", "gb", "zy", {/* mov oDiffuse.yz, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x20706819}},
    {"mov", "gb", "yw", {/* mov oDiffuse.yz, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x20706819}},
    {"mov", "gb", "yzw", {/* mov oDiffuse.yz, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20706819}},
    {"mov", "gb", "yx", {/* mov oDiffuse.yz, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20706819}},
    {"mov", "gb", "wzy", {/* mov oDiffuse.yz, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20706819}},
    {"mov", "gb", "xzw", {/* mov oDiffuse.yz, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20706819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov03[] = {
    {"mov", "gb", "zx", {/* mov oDiffuse.yz, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20706819}},
    {"mov", "gb", "zxy", {/* mov oDiffuse.yz, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20706819}},
    {"mov", "gb", "wz", {/* mov oDiffuse.yz, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20706819}},
    {"mov", "rgba", "xw", {/* mov oDiffuse, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x2070f819}},
    {"mov", "rgba", "zy", {/* mov oDiffuse, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x2070f819}},
    {"mov", "rgba", "yw", {/* mov oDiffuse, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x2070f819}},
    {"mov", "rgba", "yzw", {/* mov oDiffuse, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x2070f819}},
    {"mov", "rgba", "yx", {/* mov oDiffuse, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x2070f819}},
    {"mov", "rgba", "wzy", {/* mov oDiffuse, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x2070f819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov04[] = {
    {"mov", "rgba", "xzw", {/* mov oDiffuse, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x2070f819}},
    {"mov", "rgba", "zx", {/* mov oDiffuse, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x2070f819}},
    {"mov", "rgba", "zxy", {/* mov oDiffuse, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x2070f819}},
    {"mov", "rgba", "wz", {/* mov oDiffuse, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x2070f819}},
    {"mov", "ga", "xw", {/* mov oDiffuse.yw, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20705819}},
    {"mov", "ga", "zy", {/* mov oDiffuse.yw, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x20705819}},
    {"mov", "ga", "yw", {/* mov oDiffuse.yw, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x20705819}},
    {"mov", "ga", "yzw", {/* mov oDiffuse.yw, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20705819}},
    {"mov", "ga", "yx", {/* mov oDiffuse.yw, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20705819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov05[] = {
    {"mov", "ga", "wzy", {/* mov oDiffuse.yw, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20705819}},
    {"mov", "ga", "xzw", {/* mov oDiffuse.yw, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20705819}},
    {"mov", "ga", "zx", {/* mov oDiffuse.yw, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20705819}},
    {"mov", "ga", "zxy", {/* mov oDiffuse.yw, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20705819}},
    {"mov", "ga", "wz", {/* mov oDiffuse.yw, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20705819}},
    {"mov", "ba", "xw", {/* mov oDiffuse.zw, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20703819}},
    {"mov", "ba", "zy", {/* mov oDiffuse.zw, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x20703819}},
    {"mov", "ba", "yw", {/* mov oDiffuse.zw, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x20703819}},
    {"mov", "ba", "yzw", {/* mov oDiffuse.zw, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20703819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov06[] = {
    {"mov", "ba", "yx", {/* mov oDiffuse.zw, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20703819}},
    {"mov", "ba", "wzy", {/* mov oDiffuse.zw, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20703819}},
    {"mov", "ba", "xzw", {/* mov oDiffuse.zw, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20703819}},
    {"mov", "ba", "zx", {/* mov oDiffuse.zw, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20703819}},
    {"mov", "ba", "zxy", {/* mov oDiffuse.zw, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20703819}},
    {"mov", "ba", "wz", {/* mov oDiffuse.zw, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20703819}},
    {"mov", "rg", "xw", {/* mov oDiffuse.xy, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "zy", {/* mov oDiffuse.xy, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "yw", {/* mov oDiffuse.xy, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x2070c819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov07[] = {
    {"mov", "rg", "yzw", {/* mov oDiffuse.xy, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "yx", {/* mov oDiffuse.xy, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "wzy", {/* mov oDiffuse.xy, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "xzw", {/* mov oDiffuse.xy, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "zx", {/* mov oDiffuse.xy, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "zxy", {/* mov oDiffuse.xy, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "wz", {/* mov oDiffuse.xy, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x2070c819}},
    {"mov", "b", "xw", {/* mov oDiffuse.z, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20702819}},
    {"mov", "b", "zy", {/* mov oDiffuse.z, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x20702819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov08[] = {
    {"mov", "b", "yw", {/* mov oDiffuse.z, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x20702819}},
    {"mov", "b", "yzw", {/* mov oDiffuse.z, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20702819}},
    {"mov", "b", "yx", {/* mov oDiffuse.z, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20702819}},
    {"mov", "b", "wzy", {/* mov oDiffuse.z, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20702819}},
    {"mov", "b", "xzw", {/* mov oDiffuse.z, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20702819}},
    {"mov", "b", "zx", {/* mov oDiffuse.z, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20702819}},
    {"mov", "b", "zxy", {/* mov oDiffuse.z, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20702819}},
    {"mov", "b", "wz", {/* mov oDiffuse.z, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20702819}},
    {"mov", "gba", "xw", {/* mov oDiffuse.yzw, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20707819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov09[] = {
    {"mov", "gba", "zy", {/* mov oDiffuse.yzw, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x20707819}},
    {"mov", "gba", "yw", {/* mov oDiffuse.yzw, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x20707819}},
    {"mov", "gba", "yzw", {/* mov oDiffuse.yzw, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20707819}},
    {"mov", "gba", "yx", {/* mov oDiffuse.yzw, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20707819}},
    {"mov", "gba", "wzy", {/* mov oDiffuse.yzw, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20707819}},
    {"mov", "gba", "xzw", {/* mov oDiffuse.yzw, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20707819}},
    {"mov", "gba", "zx", {/* mov oDiffuse.yzw, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20707819}},
    {"mov", "gba", "zxy", {/* mov oDiffuse.yzw, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20707819}},
    {"mov", "gba", "wz", {/* mov oDiffuse.yzw, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20707819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov10[] = {
    {"mov", "a", "xw", {/* mov oDiffuse.w, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20701819}},
    {"mov", "a", "zy", {/* mov oDiffuse.w, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x20701819}},
    {"mov", "a", "yw", {/* mov oDiffuse.w, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x20701819}},
    {"mov", "a", "yzw", {/* mov oDiffuse.w, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20701819}},
    {"mov", "a", "yx", {/* mov oDiffuse.w, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20701819}},
    {"mov", "a", "wzy", {/* mov oDiffuse.w, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20701819}},
    {"mov", "a", "xzw", {/* mov oDiffuse.w, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20701819}},
    {"mov", "a", "zx", {/* mov oDiffuse.w, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20701819}},
    {"mov", "a", "zxy", {/* mov oDiffuse.w, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20701819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov11[] = {
    {"mov", "a", "wz", {/* mov oDiffuse.w, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20701819}},
    {"mov", "rba", "xw", {/* mov oDiffuse.xzw, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x2070b819}},
    {"mov", "rba", "zy", {/* mov oDiffuse.xzw, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x2070b819}},
    {"mov", "rba", "yw", {/* mov oDiffuse.xzw, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x2070b819}},
    {"mov", "rba", "yzw", {/* mov oDiffuse.xzw, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x2070b819}},
    {"mov", "rba", "yx", {/* mov oDiffuse.xzw, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x2070b819}},
    {"mov", "rba", "wzy", {/* mov oDiffuse.xzw, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x2070b819}},
    {"mov", "rba", "xzw", {/* mov oDiffuse.xzw, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x2070b819}},
    {"mov", "rba", "zx", {/* mov oDiffuse.xzw, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x2070b819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov12[] = {
    {"mov", "rba", "zxy", {/* mov oDiffuse.xzw, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x2070b819}},
    {"mov", "rba", "wz", {/* mov oDiffuse.xzw, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x2070b819}},
    {"mov", "rga", "xw", {/* mov oDiffuse.xyw, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x2070d819}},
    {"mov", "rga", "zy", {/* mov oDiffuse.xyw, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x2070d819}},
    {"mov", "rga", "yw", {/* mov oDiffuse.xyw, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x2070d819}},
    {"mov", "rga", "yzw", {/* mov oDiffuse.xyw, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x2070d819}},
    {"mov", "rga", "yx", {/* mov oDiffuse.xyw, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x2070d819}},
    {"mov", "rga", "wzy", {/* mov oDiffuse.xyw, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x2070d819}},
    {"mov", "rga", "xzw", {/* mov oDiffuse.xyw, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x2070d819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov13[] = {
    {"mov", "rga", "zx", {/* mov oDiffuse.xyw, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x2070d819}},
    {"mov", "rga", "zxy", {/* mov oDiffuse.xyw, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x2070d819}},
    {"mov", "rga", "wz", {/* mov oDiffuse.xyw, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x2070d819}},
    {"mov", "rb", "xw", {/* mov oDiffuse.xz, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "zy", {/* mov oDiffuse.xz, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "yw", {/* mov oDiffuse.xz, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "yzw", {/* mov oDiffuse.xz, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "yx", {/* mov oDiffuse.xz, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "wzy", {/* mov oDiffuse.xz, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x2070a819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov14[] = {
    {"mov", "rb", "xzw", {/* mov oDiffuse.xz, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "zx", {/* mov oDiffuse.xz, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "zxy", {/* mov oDiffuse.xz, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "wz", {/* mov oDiffuse.xz, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x2070a819}},
    {"mov", "rgb", "xw", {/* mov oDiffuse.xyz, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "zy", {/* mov oDiffuse.xyz, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "yw", {/* mov oDiffuse.xyz, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "yzw", {/* mov oDiffuse.xyz, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "yx", {/* mov oDiffuse.xyz, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x2070e819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov15[] = {
    {"mov", "rgb", "wzy", {/* mov oDiffuse.xyz, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "xzw", {/* mov oDiffuse.xyz, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "zx", {/* mov oDiffuse.xyz, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "zxy", {/* mov oDiffuse.xyz, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "wz", {/* mov oDiffuse.xyz, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x2070e819}},
    {"mov", "g", "xw", {/* mov oDiffuse.y, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20704819}},
    {"mov", "g", "zy", {/* mov oDiffuse.y, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x20704819}},
    {"mov", "g", "yw", {/* mov oDiffuse.y, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x20704819}},
    {"mov", "g", "yzw", {/* mov oDiffuse.y, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20704819}},
};
static const VertexShaderSwizzleTests::Instruction kTestMov16[] = {
    {"mov", "g", "yx", {/* mov oDiffuse.y, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20704819}},
    {"mov", "g", "wzy", {/* mov oDiffuse.y, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20704819}},
    {"mov", "g", "xzw", {/* mov oDiffuse.y, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20704819}},
    {"mov", "g", "zx", {/* mov oDiffuse.y, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20704819}},
    {"mov", "g", "zxy", {/* mov oDiffuse.y, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20704819}},
    {"mov", "g", "wz", {/* mov oDiffuse.y, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20704819}},
};

static const VertexShaderSwizzleTests::Instruction kTestNAMov00[] = {
    {"mov", "g", "wz", {/* mov oDiffuse.y, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20704819}},
    {"mov", "g", "xw", {/* mov oDiffuse.y, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20704819}},
    {"mov", "g", "zx", {/* mov oDiffuse.y, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20704819}},
    {"mov", "g", "xy", {/* mov oDiffuse.y, v3.xyyy */ 0x00000000, 0x00200615, 0x0836106c, 0x20704819}},
    {"mov", "g", "yzw", {/* mov oDiffuse.y, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20704819}},
    {"mov", "g", "wzy", {/* mov oDiffuse.y, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20704819}},
    {"mov", "g", "zy", {/* mov oDiffuse.y, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x20704819}},
    {"mov", "g", "yw", {/* mov oDiffuse.y, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x20704819}},
    {"mov", "g", "xzw", {/* mov oDiffuse.y, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20704819}},
};
static const VertexShaderSwizzleTests::Instruction kTestNAMov01[] = {
    {"mov", "g", "zxy", {/* mov oDiffuse.y, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20704819}},
    {"mov", "g", "yx", {/* mov oDiffuse.y, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20704819}},
    {"mov", "g", "wy", {/* mov oDiffuse.y, v3.wyyy */ 0x00000000, 0x002006d5, 0x0836106c, 0x20704819}},
    {"mov", "rb", "wz", {/* mov oDiffuse.xz, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "xw", {/* mov oDiffuse.xz, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "zx", {/* mov oDiffuse.xz, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "xy", {/* mov oDiffuse.xz, v3.xyyy */ 0x00000000, 0x00200615, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "yzw", {/* mov oDiffuse.xz, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "wzy", {/* mov oDiffuse.xz, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x2070a819}},
};
static const VertexShaderSwizzleTests::Instruction kTestNAMov02[] = {
    {"mov", "rb", "zy", {/* mov oDiffuse.xz, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "yw", {/* mov oDiffuse.xz, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "xzw", {/* mov oDiffuse.xz, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "zxy", {/* mov oDiffuse.xz, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "yx", {/* mov oDiffuse.xz, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x2070a819}},
    {"mov", "rb", "wy", {/* mov oDiffuse.xz, v3.wyyy */ 0x00000000, 0x002006d5, 0x0836106c, 0x2070a819}},
    {"mov", "rg", "wz", {/* mov oDiffuse.xy, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "xw", {/* mov oDiffuse.xy, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "zx", {/* mov oDiffuse.xy, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x2070c819}},
};
static const VertexShaderSwizzleTests::Instruction kTestNAMov03[] = {
    {"mov", "rg", "xy", {/* mov oDiffuse.xy, v3.xyyy */ 0x00000000, 0x00200615, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "yzw", {/* mov oDiffuse.xy, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "wzy", {/* mov oDiffuse.xy, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "zy", {/* mov oDiffuse.xy, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "yw", {/* mov oDiffuse.xy, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "xzw", {/* mov oDiffuse.xy, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "zxy", {/* mov oDiffuse.xy, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "yx", {/* mov oDiffuse.xy, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x2070c819}},
    {"mov", "rg", "wy", {/* mov oDiffuse.xy, v3.wyyy */ 0x00000000, 0x002006d5, 0x0836106c, 0x2070c819}},
};
static const VertexShaderSwizzleTests::Instruction kTestNAMov04[] = {
    {"mov", "b", "wz", {/* mov oDiffuse.z, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20702819}},
    {"mov", "b", "xw", {/* mov oDiffuse.z, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20702819}},
    {"mov", "b", "zx", {/* mov oDiffuse.z, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20702819}},
    {"mov", "b", "xy", {/* mov oDiffuse.z, v3.xyyy */ 0x00000000, 0x00200615, 0x0836106c, 0x20702819}},
    {"mov", "b", "yzw", {/* mov oDiffuse.z, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20702819}},
    {"mov", "b", "wzy", {/* mov oDiffuse.z, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20702819}},
    {"mov", "b", "zy", {/* mov oDiffuse.z, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x20702819}},
    {"mov", "b", "yw", {/* mov oDiffuse.z, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x20702819}},
    {"mov", "b", "xzw", {/* mov oDiffuse.z, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20702819}},
};
static const VertexShaderSwizzleTests::Instruction kTestNAMov05[] = {
    {"mov", "b", "zxy", {/* mov oDiffuse.z, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20702819}},
    {"mov", "b", "yx", {/* mov oDiffuse.z, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20702819}},
    {"mov", "b", "wy", {/* mov oDiffuse.z, v3.wyyy */ 0x00000000, 0x002006d5, 0x0836106c, 0x20702819}},
    {"mov", "gb", "wz", {/* mov oDiffuse.yz, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20706819}},
    {"mov", "gb", "xw", {/* mov oDiffuse.yz, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20706819}},
    {"mov", "gb", "zx", {/* mov oDiffuse.yz, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20706819}},
    {"mov", "gb", "xy", {/* mov oDiffuse.yz, v3.xyyy */ 0x00000000, 0x00200615, 0x0836106c, 0x20706819}},
    {"mov", "gb", "yzw", {/* mov oDiffuse.yz, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20706819}},
    {"mov", "gb", "wzy", {/* mov oDiffuse.yz, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20706819}},
};
static const VertexShaderSwizzleTests::Instruction kTestNAMov06[] = {
    {"mov", "gb", "zy", {/* mov oDiffuse.yz, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x20706819}},
    {"mov", "gb", "yw", {/* mov oDiffuse.yz, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x20706819}},
    {"mov", "gb", "xzw", {/* mov oDiffuse.yz, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20706819}},
    {"mov", "gb", "zxy", {/* mov oDiffuse.yz, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20706819}},
    {"mov", "gb", "yx", {/* mov oDiffuse.yz, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20706819}},
    {"mov", "gb", "wy", {/* mov oDiffuse.yz, v3.wyyy */ 0x00000000, 0x002006d5, 0x0836106c, 0x20706819}},
    {"mov", "r", "wz", {/* mov oDiffuse.x, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x20708819}},
    {"mov", "r", "xw", {/* mov oDiffuse.x, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x20708819}},
    {"mov", "r", "zx", {/* mov oDiffuse.x, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x20708819}},
};
static const VertexShaderSwizzleTests::Instruction kTestNAMov07[] = {
    {"mov", "r", "xy", {/* mov oDiffuse.x, v3.xyyy */ 0x00000000, 0x00200615, 0x0836106c, 0x20708819}},
    {"mov", "r", "yzw", {/* mov oDiffuse.x, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x20708819}},
    {"mov", "r", "wzy", {/* mov oDiffuse.x, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x20708819}},
    {"mov", "r", "zy", {/* mov oDiffuse.x, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x20708819}},
    {"mov", "r", "yw", {/* mov oDiffuse.x, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x20708819}},
    {"mov", "r", "xzw", {/* mov oDiffuse.x, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x20708819}},
    {"mov", "r", "zxy", {/* mov oDiffuse.x, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x20708819}},
    {"mov", "r", "yx", {/* mov oDiffuse.x, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x20708819}},
    {"mov", "r", "wy", {/* mov oDiffuse.x, v3.wyyy */ 0x00000000, 0x002006d5, 0x0836106c, 0x20708819}},
};
static const VertexShaderSwizzleTests::Instruction kTestNAMov08[] = {
    {"mov", "rgb", "wz", {/* mov oDiffuse.xyz, v3.wzzz */ 0x00000000, 0x002006ea, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "xw", {/* mov oDiffuse.xyz, v3.xwww */ 0x00000000, 0x0020063f, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "zx", {/* mov oDiffuse.xyz, v3.zxxx */ 0x00000000, 0x00200680, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "xy", {/* mov oDiffuse.xyz, v3.xyyy */ 0x00000000, 0x00200615, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "yzw", {/* mov oDiffuse.xyz, v3.yzww */ 0x00000000, 0x0020066f, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "wzy", {/* mov oDiffuse.xyz, v3.wzyy */ 0x00000000, 0x002006e5, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "zy", {/* mov oDiffuse.xyz, v3.zyyy */ 0x00000000, 0x00200695, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "yw", {/* mov oDiffuse.xyz, v3.ywww */ 0x00000000, 0x0020067f, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "xzw", {/* mov oDiffuse.xyz, v3.xzww */ 0x00000000, 0x0020062f, 0x0836106c, 0x2070e819}},
};
static const VertexShaderSwizzleTests::Instruction kTestNAMov09[] = {
    {"mov", "rgb", "zxy", {/* mov oDiffuse.xyz, v3.zxyy */ 0x00000000, 0x00200685, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "yx", {/* mov oDiffuse.xyz, v3.yxxx */ 0x00000000, 0x00200640, 0x0836106c, 0x2070e819}},
    {"mov", "rgb", "wy", {/* mov oDiffuse.xyz, v3.wyyy */ 0x00000000, 0x002006d5, 0x0836106c, 0x2070e819}},
};

static const VertexShaderSwizzleTests::Instruction kTestControl[] = {
    {"mov", "default", "default", {0, 0, 0, 1}},
};

static constexpr uint32_t kCheckerboardA = 0xFF202020;
static constexpr uint32_t kCheckerboardB = 0xFF000000;

VertexShaderSwizzleTests::VertexShaderSwizzleTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Vertex shader swizzle tests", config) {
  tests_["Control"] = [this]() { Test("Control", kTestControl, sizeof(kTestControl) / sizeof(kTestControl[0])); };
  tests_["ControlNA"] = [this]() {
    Test("ControlNA", kTestControl, sizeof(kTestControl) / sizeof(kTestControl[0]), true);
  };

  tests_["kTestMov00"] = [this]() { Test("Mov00", kTestMov00, sizeof(kTestMov00) / sizeof(kTestMov00[0])); };
  tests_["kTestMov01"] = [this]() { Test("Mov01", kTestMov01, sizeof(kTestMov01) / sizeof(kTestMov01[0])); };
  tests_["kTestMov02"] = [this]() { Test("Mov02", kTestMov02, sizeof(kTestMov02) / sizeof(kTestMov02[0])); };
  tests_["kTestMov03"] = [this]() { Test("Mov03", kTestMov03, sizeof(kTestMov03) / sizeof(kTestMov03[0])); };
  tests_["kTestMov04"] = [this]() { Test("Mov04", kTestMov04, sizeof(kTestMov04) / sizeof(kTestMov04[0])); };
  tests_["kTestMov05"] = [this]() { Test("Mov05", kTestMov05, sizeof(kTestMov05) / sizeof(kTestMov05[0])); };
  tests_["kTestMov06"] = [this]() { Test("Mov06", kTestMov06, sizeof(kTestMov06) / sizeof(kTestMov06[0])); };
  tests_["kTestMov07"] = [this]() { Test("Mov07", kTestMov07, sizeof(kTestMov07) / sizeof(kTestMov07[0])); };
  tests_["kTestMov08"] = [this]() { Test("Mov08", kTestMov08, sizeof(kTestMov08) / sizeof(kTestMov08[0])); };
  tests_["kTestMov09"] = [this]() { Test("Mov09", kTestMov09, sizeof(kTestMov09) / sizeof(kTestMov09[0])); };
  tests_["kTestMov10"] = [this]() { Test("Mov10", kTestMov10, sizeof(kTestMov10) / sizeof(kTestMov10[0])); };
  tests_["kTestMov11"] = [this]() { Test("Mov11", kTestMov11, sizeof(kTestMov11) / sizeof(kTestMov11[0])); };
  tests_["kTestMov12"] = [this]() { Test("Mov12", kTestMov12, sizeof(kTestMov12) / sizeof(kTestMov12[0])); };
  tests_["kTestMov13"] = [this]() { Test("Mov13", kTestMov13, sizeof(kTestMov13) / sizeof(kTestMov13[0])); };
  tests_["kTestMov14"] = [this]() { Test("Mov14", kTestMov14, sizeof(kTestMov14) / sizeof(kTestMov14[0])); };
  tests_["kTestMov15"] = [this]() { Test("Mov15", kTestMov15, sizeof(kTestMov15) / sizeof(kTestMov15[0])); };
  tests_["kTestMov16"] = [this]() { Test("Mov16", kTestMov16, sizeof(kTestMov16) / sizeof(kTestMov16[0])); };

  tests_["kTestNAMov00"] = [this]() {
    Test("NAMov00", kTestNAMov00, sizeof(kTestNAMov00) / sizeof(kTestNAMov00[0]), true);
  };
  tests_["kTestNAMov01"] = [this]() {
    Test("NAMov01", kTestNAMov01, sizeof(kTestNAMov01) / sizeof(kTestNAMov01[0]), true);
  };
  tests_["kTestNAMov02"] = [this]() {
    Test("NAMov02", kTestNAMov02, sizeof(kTestNAMov02) / sizeof(kTestNAMov02[0]), true);
  };
  tests_["kTestNAMov03"] = [this]() {
    Test("NAMov03", kTestNAMov03, sizeof(kTestNAMov03) / sizeof(kTestNAMov03[0]), true);
  };
  tests_["kTestNAMov04"] = [this]() {
    Test("NAMov04", kTestNAMov04, sizeof(kTestNAMov04) / sizeof(kTestNAMov04[0]), true);
  };
  tests_["kTestNAMov05"] = [this]() {
    Test("NAMov05", kTestNAMov05, sizeof(kTestNAMov05) / sizeof(kTestNAMov05[0]), true);
  };
  tests_["kTestNAMov06"] = [this]() {
    Test("NAMov06", kTestNAMov06, sizeof(kTestNAMov06) / sizeof(kTestNAMov06[0]), true);
  };
  tests_["kTestNAMov07"] = [this]() {
    Test("NAMov07", kTestNAMov07, sizeof(kTestNAMov07) / sizeof(kTestNAMov07[0]), true);
  };
  tests_["kTestNAMov08"] = [this]() {
    Test("NAMov08", kTestNAMov08, sizeof(kTestNAMov08) / sizeof(kTestNAMov08[0]), true);
  };
  tests_["kTestNAMov09"] = [this]() {
    Test("NAMov09", kTestNAMov09, sizeof(kTestNAMov09) / sizeof(kTestNAMov09[0]), true);
  };
}

void VertexShaderSwizzleTests::Initialize() {
  TestSuite::Initialize();

  host_.SetCombinerControl(1);
  host_.SetBlend();

  // Allocate enough space for the shader header and the final instruction.
  shader_code_size_ = sizeof(kShaderHeader) + 16;
  shader_code_ = new uint32_t[shader_code_size_];
  memcpy(shader_code_, kShaderHeader, sizeof(kShaderHeader));
}

void VertexShaderSwizzleTests::Deinitialize() {
  TestSuite::Deinitialize();
  delete[] shader_code_;
}

void VertexShaderSwizzleTests::Test(const std::string &name, const Instruction *instructions, uint32_t count,
                                    bool full_opacity) {
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.PrepareDraw(0xFE312F31);

  DrawCheckerboardBackground();

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  if (full_opacity) {
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  } else {
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
  }

  constexpr float kWidth = 96.0f;
  constexpr float kHeight = 96.0f;
  constexpr float z = 0.0f;

  const float kStartHorizontal = (host_.GetFramebufferWidthF() - kWidth * 3) * 0.25f;
  const float kStartVertical = (host_.GetFramebufferHeightF() - kHeight * 3) * 0.25f + 10.0f;
  const float kEndHorizontal = host_.GetFramebufferWidthF() - kWidth;
  float left = kStartHorizontal;
  float top = kStartVertical;

  constexpr float kDefault = 0.33f;
  constexpr float kRed = 0.60f;
  constexpr float kGreen = 0.70f;
  constexpr float kBlue = 0.80f;
  constexpr float kAlpha = 1.0f;

  for (uint32_t i = 0; i < count; ++i) {
    memcpy(shader_code_ + sizeof(kShaderHeader) / 4, instructions[i].instruction, sizeof(instructions[i].instruction));
    shader->SetShader(shader_code_, shader_code_size_);
    // Set the default color for unset components.
    shader->SetUniformF(0, kDefault, kDefault, kDefault, kDefault);
    shader->Activate();
    shader->PrepareDraw();
    host_.SetVertexShaderProgram(shader);

    const float right = left + kWidth;
    const float bottom = top + kHeight;

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

  pb_print("%s\n\n", name.c_str());
  pb_print("x%.02f\ny%.02f\nz%.02f\nw%.02f\n", kRed, kGreen, kBlue, kAlpha);

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

  FinishDraw(name);
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
