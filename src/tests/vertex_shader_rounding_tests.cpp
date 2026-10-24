#include "vertex_shader_rounding_tests.h"

#include <pbkit/pbkit.h>

#include <cstring>
#include <memory>
#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/passthrough_vertex_shader.h"
#include "shaders/perspective_vertex_shader_no_lighting.h"
#include "test_host.h"
#include "texture_format.h"
#include "texture_generator.h"
#include "third_party/gpu_m2m.h"
#include "xbox_math_matrix.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
const uint32_t kDefaultDMAChannelA = 3;

// From pbkit.c, DMA_COLOR is set to channel 9 by default
const uint32_t kDefaultDMAColorChannel = 9;

static const uint32_t kTextureWidth = 128;
static const uint32_t kTexturePitch = kTextureWidth * 4;
static const uint32_t kTextureHeight = 128;

static constexpr const char kTestRenderTargetName[] = "RenderTarget";
static constexpr const char kTestCompositingRenderTargetName[] = "Compositing";
static constexpr const char kTestGeometryName[] = "Geometry";
static constexpr const char kTestGeometrySubscreenName[] = "GeometrySubscreen";
static constexpr const char kTestGeometrySuperscreenName[] = "GeometrySuperscreen";
static constexpr const char kTestAdjacentGeometryName[] = "AdjacentGeometry";
static constexpr const char kTestProjectedAdjacentGeometryName[] = "ProjAdjacentGeometry";
static constexpr const char kTestTopLeftRasterName[] = "TopLeftRaster";
static constexpr const char kTestOpRoundingDP4Name[] = "OpRounding_DP4";
static constexpr const char kTestOpRoundingMADName[] = "OpRounding_MAD";
static constexpr const char kTestOpRoundingMULName[] = "OpRounding_MUL";

static const uint32_t kRoundingDP4Vsh[] = {
#include "vertex_shader_rounding_dp4.vshinc"
};
static const uint32_t kRoundingMADVsh[] = {
#include "vertex_shader_rounding_mad.vshinc"
};
static const uint32_t kRoundingMULVsh[] = {
#include "vertex_shader_rounding_mul.vshinc"
};

// Operands for the OpRounding tests. `c` is the IEEE float32 round-to-nearest-even value of a * b. The exact product is
// 0.25, 0.75, 0.25, 0.75 and 0.5 ulp above the representable value below its magnitude, respectively.
struct RoundingCase {
  const char *label;
  uint32_t a;
  uint32_t b;
  uint32_t c;
};
static constexpr RoundingCase kRoundingCases[] = {
    {"+0.25", 0x3F800800, 0x3F800400, 0x3F800C00},
    {"+0.75", 0x3F800800, 0x3F800C00, 0x3F801401},
    {"-0.25", 0xBF800800, 0x3F800400, 0xBF800C00},
    {"-0.75", 0xBF800800, 0x3F800C00, 0xBF801401},
    // -1.0014837 * -40 as computed by Rugby 2005's projection (xemu-project/xemu#2388).
    {"Rugby", 0xBF80309E, 0xC2200000, 0x42203CC6},
};

static constexpr float kGeometryTestBiases[] = {
    // Boundaries at 1/16 = 0.0625f
    0.0f, 0.001f, 0.4999f, 0.5f, 0.5624f, 0.5625f, 0.5626f, 0.999f, 1.0f,
};

static std::string MakeCompositingRenderTargetTestName(int z);
static std::string MakeGeometryTestName(const char *prefix, float bias);
static std::string MakeTopLeftRasterTestName(bool fixed);

/**
 * Initializes the test suite and creates test cases.
 *
 * Geometry*, GeometrySubscreen*, GeometrySuperscreen*, AdjacentGeometry* and ProjAdjacentGeometry* tests are
 * parameterized by a subpixel bias (the numeric suffix) that is added to vertex positions; the biases straddle half
 * pixel and 1/16 subpixel boundaries. Each family is described in its _0.0000 entry.
 *
 * @tc AdjacentGeometry_0.0000
 *  Draws a light grey 400x200 background, then a 4x2 grid of adjacent 100x100 green quads, each offset by the bias in x
 *  and y, through a passthrough vertex shader. Grey seams between neighboring quads show inconsistent rounding of
 *  shared edges. This case uses no offset.
 *
 * @tc AdjacentGeometry_0.0010
 *  As AdjacentGeometry_0.0000 with a bias of 0.0010 (a tiny offset).
 *
 * @tc AdjacentGeometry_0.4999
 *  As AdjacentGeometry_0.0000 with a bias of 0.4999 (just under half a pixel).
 *
 * @tc AdjacentGeometry_0.5000
 *  As AdjacentGeometry_0.0000 with a bias of 0.5000 (exactly half a pixel).
 *
 * @tc AdjacentGeometry_0.5624
 *  As AdjacentGeometry_0.0000 with a bias of 0.5624 (just under the 9/16 subpixel boundary).
 *
 * @tc AdjacentGeometry_0.5625
 *  As AdjacentGeometry_0.0000 with a bias of 0.5625 (exactly 9/16 of a pixel, a 1/16 subpixel boundary).
 *
 * @tc AdjacentGeometry_0.5626
 *  As AdjacentGeometry_0.0000 with a bias of 0.5626 (just over the 9/16 subpixel boundary).
 *
 * @tc AdjacentGeometry_0.9990
 *  As AdjacentGeometry_0.0000 with a bias of 0.9990 (just under one pixel).
 *
 * @tc AdjacentGeometry_1.0000
 *  As AdjacentGeometry_0.0000 with a bias of 1.0000 (exactly one pixel).
 *
 * @tc Compositing_-2
 *  As Compositing_-4, but the final full screen quad is drawn at z = -2.
 *
 * @tc Compositing_-4
 *  Fills a framebuffer sized texture with a centered 256x256 checkerboard, points the color surface at that texture and
 *  draws the left half of the texture onto itself four times through a passthrough vertex shader, with the combiner
 *  outputting the texture's color and half its alpha. The texture is then drawn to the screen with a fixed function
 *  full screen quad at z = -4. Any offset or blurring in the left half shows texel misalignment that accumulates over
 *  the repeated self compositing.
 *
 * @tc Compositing_2
 *  As Compositing_-4, but the final full screen quad is drawn at z = 2.
 *
 * @tc GeometrySubscreen_0.0000
 *  For each of four sizes (128x128, 128x32, 32x128 and 16x64), points the color surface at texture memory sized exactly
 *  to the quad, fills it with magenta on the CPU and draws a green quad covering the whole surface offset by the bias
 *  in x and y through a passthrough vertex shader. Each result is then drawn to the screen as a texture in a 2x2 grid.
 *  Magenta shows texels the offset quad did not cover. This case uses no offset.
 *
 * @tc GeometrySubscreen_0.0010
 *  As GeometrySubscreen_0.0000 with a bias of 0.0010 (a tiny offset).
 *
 * @tc GeometrySubscreen_0.4999
 *  As GeometrySubscreen_0.0000 with a bias of 0.4999 (just under half a pixel).
 *
 * @tc GeometrySubscreen_0.5000
 *  As GeometrySubscreen_0.0000 with a bias of 0.5000 (exactly half a pixel).
 *
 * @tc GeometrySubscreen_0.5624
 *  As GeometrySubscreen_0.0000 with a bias of 0.5624 (just under the 9/16 subpixel boundary).
 *
 * @tc GeometrySubscreen_0.5625
 *  As GeometrySubscreen_0.0000 with a bias of 0.5625 (exactly 9/16 of a pixel, a 1/16 subpixel boundary).
 *
 * @tc GeometrySubscreen_0.5626
 *  As GeometrySubscreen_0.0000 with a bias of 0.5626 (just over the 9/16 subpixel boundary).
 *
 * @tc GeometrySubscreen_0.9990
 *  As GeometrySubscreen_0.0000 with a bias of 0.9990 (just under one pixel).
 *
 * @tc GeometrySubscreen_1.0000
 *  As GeometrySubscreen_0.0000 with a bias of 1.0000 (exactly one pixel).
 *
 * @tc GeometrySuperscreen_0.0000
 *  As GeometrySubscreen, but the color surface is always 1024x1024 and only the first width * height texels of it
 *  (contiguously, not as a rectangle) are filled with magenta before drawing the offset green quad of each size. The
 *  top left width x height region of the surface is drawn to the screen for each size. This case uses no offset.
 *
 * @tc GeometrySuperscreen_0.0010
 *  As GeometrySuperscreen_0.0000 with a bias of 0.0010 (a tiny offset).
 *
 * @tc GeometrySuperscreen_0.4999
 *  As GeometrySuperscreen_0.0000 with a bias of 0.4999 (just under half a pixel).
 *
 * @tc GeometrySuperscreen_0.5000
 *  As GeometrySuperscreen_0.0000 with a bias of 0.5000 (exactly half a pixel).
 *
 * @tc GeometrySuperscreen_0.5624
 *  As GeometrySuperscreen_0.0000 with a bias of 0.5624 (just under the 9/16 subpixel boundary).
 *
 * @tc GeometrySuperscreen_0.5625
 *  As GeometrySuperscreen_0.0000 with a bias of 0.5625 (exactly 9/16 of a pixel, a 1/16 subpixel boundary).
 *
 * @tc GeometrySuperscreen_0.5626
 *  As GeometrySuperscreen_0.0000 with a bias of 0.5626 (just over the 9/16 subpixel boundary).
 *
 * @tc GeometrySuperscreen_0.9990
 *  As GeometrySuperscreen_0.0000 with a bias of 0.9990 (just under one pixel).
 *
 * @tc GeometrySuperscreen_1.0000
 *  As GeometrySuperscreen_0.0000 with a bias of 1.0000 (exactly one pixel).
 *
 * @tc Geometry_0.0000
 *  Draws a 240x240 magenta quad centered on the screen, then a green quad over it whose top and left edges are offset
 *  by the bias (the right and bottom edges are unchanged), both through a passthrough vertex shader. Any magenta left
 *  visible along the top or left edge shows how the offset edge was snapped to the 1/16 subpixel grid and rasterized.
 *  This case uses no offset.
 *
 * @tc Geometry_0.0010
 *  As Geometry_0.0000 with a bias of 0.0010 (a tiny offset).
 *
 * @tc Geometry_0.4999
 *  As Geometry_0.0000 with a bias of 0.4999 (just under half a pixel).
 *
 * @tc Geometry_0.5000
 *  As Geometry_0.0000 with a bias of 0.5000 (exactly half a pixel).
 *
 * @tc Geometry_0.5624
 *  As Geometry_0.0000 with a bias of 0.5624 (just under the 9/16 subpixel boundary).
 *
 * @tc Geometry_0.5625
 *  As Geometry_0.0000 with a bias of 0.5625 (exactly 9/16 of a pixel, a 1/16 subpixel boundary).
 *
 * @tc Geometry_0.5626
 *  As Geometry_0.0000 with a bias of 0.5626 (just over the 9/16 subpixel boundary).
 *
 * @tc Geometry_0.9990
 *  As Geometry_0.0000 with a bias of 0.9990 (just under one pixel).
 *
 * @tc Geometry_1.0000
 *  As Geometry_0.0000 with a bias of 1.0000 (exactly one pixel).
 *
 * @tc OpRounding_DP4
 *  As OpRounding_MUL, but d is computed by a single DP4 of (a, c, 0, 0) and (b, -1, 0, 2^23). On hardware the product
 *  is truncated before it is summed, giving the same results as OpRounding_MUL.
 *
 * @tc OpRounding_MAD
 *  As OpRounding_MUL, but d is computed by a single MAD (a * b - c). On hardware the product is truncated before the
 *  addition (the operation is not fused), giving the same results as OpRounding_MUL.
 *
 * @tc OpRounding_MUL
 *  Measures the rounding behavior of the MUL vertex shader instruction. Each case uses operands a and b whose exact
 *  product lies 0.25 or 0.75 ulp above a representable float (for positive and negative products), or exactly 0.5 ulp
 *  above one (the -1.0014837 * -40 product from Rugby 2005, xemu-project/xemu#2388), along with c, the IEEE
 *  round-to-nearest-even value of the product, all passed as shader constants. The shader computes d = MUL(a, b) - c
 *  (the subtraction is exact) and writes z = d * 2^28 + 2^23 to a Z24S8 surface, which is read back on the CPU. The
 *  screen lists, per case, the depth minus 2^23 and d in ulps of c. IEEE round to nearest would give d = 0 for every
 *  case. Hardware truncates the product toward zero, giving d = 0, -1, 0, 1 and -1 ulp.
 *
 * @tc ProjAdjacentGeometry_0.0000
 *  As AdjacentGeometry, but positions are unprojected through a D3D style perspective projection with z varying from
 *  the top to the bottom of each quad. Quads alternate in a checkerboard between the programmable pipeline (dark green,
 *  0xFF009900) and the fixed function pipeline (light green, 0xFF00BB00), so seams between them show differences in how
 *  the two pipelines round projected vertices. This case uses no offset.
 *
 * @tc ProjAdjacentGeometry_0.0010
 *  As ProjAdjacentGeometry_0.0000 with a bias of 0.0010 (a tiny offset).
 *
 * @tc ProjAdjacentGeometry_0.4999
 *  As ProjAdjacentGeometry_0.0000 with a bias of 0.4999 (just under half a pixel).
 *
 * @tc ProjAdjacentGeometry_0.5000
 *  As ProjAdjacentGeometry_0.0000 with a bias of 0.5000 (exactly half a pixel).
 *
 * @tc ProjAdjacentGeometry_0.5624
 *  As ProjAdjacentGeometry_0.0000 with a bias of 0.5624 (just under the 9/16 subpixel boundary).
 *
 * @tc ProjAdjacentGeometry_0.5625
 *  As ProjAdjacentGeometry_0.0000 with a bias of 0.5625 (exactly 9/16 of a pixel, a 1/16 subpixel boundary).
 *
 * @tc ProjAdjacentGeometry_0.5626
 *  As ProjAdjacentGeometry_0.0000 with a bias of 0.5626 (just over the 9/16 subpixel boundary).
 *
 * @tc ProjAdjacentGeometry_0.9990
 *  As ProjAdjacentGeometry_0.0000 with a bias of 0.9990 (just under one pixel).
 *
 * @tc ProjAdjacentGeometry_1.0000
 *  As ProjAdjacentGeometry_0.0000 with a bias of 1.0000 (exactly one pixel).
 *
 * @tc RenderTarget
 *  Points the color surface at a 128x128 render target that is prefilled with translucent magenta, sets the viewport
 *  offset to (320.53125, 240.53125) and the viewport scale to (320, -240, 16777215), and draws a quad from (-1, 1) to
 *  (1, -1) with a manually loaded vertex program ending in the XDK screen space epilogue (MUL by c[58], RCC of w, MAD
 *  with c[59]). The render target is then drawn to the screen with the fixed function pipeline. Magenta shows texels
 *  the quad did not cover.
 *
 * @tc TopLeftRaster
 *  Tests the rasterizer's top-left fill rule and 1/16 subpixel snapping through a passthrough vertex shader. Draws thin
 *  vertical quads and triangles whose left edges sit at x + 0.5 (or the next float below it) and step by 1/16 pixel, a
 *  0.2 pixel wide quad that should be invisible without multisampling, quads stacked to share edges at x + 0.5625,
 *  black to white horizontal gradient quads whose expected interpolated colors are 0x808080, 0x808080 and 0x000000 from
 *  top to bottom, and the same edge patterns rotated to horizontal on the right side of the screen.
 *
 * @tc TopLeftRaster_Fixed
 *  As TopLeftRaster, but the geometry is drawn through the fixed function pipeline with identity model view and
 *  projection matrices and a zero viewport scale and offset.
 */
VertexShaderRoundingTests::VertexShaderRoundingTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Vertex shader rounding tests", config) {
  tests_[kTestRenderTargetName] = [this]() { TestRenderTarget(); };

  for (auto z : {-4, -2, 2}) {
    tests_[MakeCompositingRenderTargetTestName(z)] = [this, z]() { TestCompositingRenderTarget(z); };
  }

  for (auto bias : kGeometryTestBiases) {
    std::string test_name = MakeGeometryTestName(kTestGeometryName, bias);
    tests_[test_name] = [this, bias]() { TestGeometry(bias); };

    test_name = MakeGeometryTestName(kTestGeometrySubscreenName, bias);
    tests_[test_name] = [this, bias]() { TestGeometrySubscreen(bias); };

    test_name = MakeGeometryTestName(kTestGeometrySuperscreenName, bias);
    tests_[test_name] = [this, bias]() { TestGeometrySuperscreen(bias); };

    test_name = MakeGeometryTestName(kTestAdjacentGeometryName, bias);
    tests_[test_name] = [this, bias]() { TestAdjacentGeometry(bias); };

    test_name = MakeGeometryTestName(kTestProjectedAdjacentGeometryName, bias);
    tests_[test_name] = [this, bias]() { TestProjectedAdjacentGeometry(bias); };
  }

  for (auto fixed : {false, true}) {
    tests_[MakeTopLeftRasterTestName(fixed)] = [this, fixed]() { TestTopLeftRasterization(fixed); };
  }

  tests_[kTestOpRoundingDP4Name] = [this]() {
    TestOpRounding(kTestOpRoundingDP4Name, kRoundingDP4Vsh, sizeof(kRoundingDP4Vsh));
  };
  tests_[kTestOpRoundingMADName] = [this]() {
    TestOpRounding(kTestOpRoundingMADName, kRoundingMADVsh, sizeof(kRoundingMADVsh));
  };
  tests_[kTestOpRoundingMULName] = [this]() {
    TestOpRounding(kTestOpRoundingMULName, kRoundingMULVsh, sizeof(kRoundingMULVsh));
  };
}

void VertexShaderRoundingTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  const uint32_t texture_size = kTexturePitch * kTextureHeight;
  render_target_ =
      (uint8_t *)MmAllocateContiguousMemoryEx(texture_size, 0, MAXRAM, 0x1000, PAGE_WRITECOMBINE | PAGE_READWRITE);
  ASSERT(render_target_ && "Failed to allocate target surface.");
}

void VertexShaderRoundingTests::Deinitialize() {
  TestSuite::Deinitialize();
  if (render_target_) {
    MmFreeContiguousMemory(render_target_);
  }
}

void VertexShaderRoundingTests::CreateGeometry() {
  framebuffer_vertex_buffer_ = host_.AllocateVertexBuffer(6);
  framebuffer_vertex_buffer_->DefineBiTri(0, -1.75, 1.75, 1.75, -1.75, 0.1f);
  framebuffer_vertex_buffer_->Linearize(host_.GetFramebufferWidthF(), host_.GetFramebufferHeightF());
}

void VertexShaderRoundingTests::TestGeometry(float bias) {
  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.PrepareDraw(0xFE404040);

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  const float height = floorf(host_.GetFramebufferHeightF() / 4.0f) * 2.0f;

  const float left = floorf((host_.GetFramebufferWidthF() - height) / 2.0f);
  const float top = floorf(height / 2.0f);
  const float right = left + height;
  const float bottom = top + height;
  const float z = 1;

  // Draw a background.
  uint32_t color = 0xFFFF00FF;
  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(color);
  host_.SetVertex(left, top, z, 1.0f);
  host_.SetVertex(right, top, z, 1.0f);
  host_.SetVertex(right, bottom, z, 1.0f);
  host_.SetVertex(left, bottom, z, 1.0f);
  host_.End();

  // Draw a subpixel offset green square.
  color = 0xFF009900;
  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(color);
  host_.SetVertex(left + bias, top + bias, z, 1.0f);
  host_.SetVertex(right, top + bias, z, 1.0f);
  host_.SetVertex(right, bottom, z, 1.0f);
  host_.SetVertex(left + bias, bottom, z, 1.0f);
  host_.End();

  std::string test_name = MakeGeometryTestName(kTestGeometryName, bias);
  pb_print("%s\n", test_name.c_str());
  pb_draw_text_screen();

  FinishDraw(test_name);
}

void VertexShaderRoundingTests::TestGeometrySubscreen(float bias) {
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
  host_.PrepareDraw(0xFE414041);

  auto draw_texture = [this, bias](uint32_t texture_width, uint32_t texture_height) {
    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
    host_.SetupTextureStages();

    const uint32_t kTexturePitch = texture_width * 4;
    const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, texture_width, texture_height);
    host_.SetWindowClip(texture_width - 1, texture_height - 1);

    // Point the color buffer at the texture and mix the left hand side with itself multiple times.
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kTexturePitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(host_.GetTextureMemory()));
    Pushbuffer::Push(NV097_NO_OPERATION, 0);
    Pushbuffer::Push(NV097_WAIT_FOR_IDLE, 0);
    Pushbuffer::End();

    // Initialize the texture to magenta.
    auto pixel = reinterpret_cast<uint32_t *>(host_.GetTextureMemory());
    for (uint32_t y = 0; y < texture_height; ++y) {
      for (uint32_t x = 0; x < texture_width; ++x, ++pixel) {
        *pixel = 0xFFFF00FF;
      }
    }

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

    // Draw a subpixel offset green square.
    static constexpr uint32_t kColor = 0xFF009900;
    static constexpr float kZ = 0.0f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(kColor);
    host_.SetVertex(bias, bias, kZ, 1.0f);
    host_.SetVertex(static_cast<float>(texture_width) + bias, bias, kZ, 1.0f);
    host_.SetVertex(static_cast<float>(texture_width) + bias, static_cast<float>(texture_height) + bias, kZ, 1.0f);
    host_.SetVertex(bias, static_cast<float>(texture_height) + bias, kZ, 1.0f);
    host_.End();

    host_.SetWindowClip(host_.GetFramebufferWidth() - 1, host_.GetFramebufferHeight() - 1);
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                    host_.GetFramebufferHeight());

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
    Pushbuffer::Push(NV097_NO_OPERATION, 0);
    Pushbuffer::Push(NV097_WAIT_FOR_IDLE, 0);
    Pushbuffer::End();
  };

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X8R8G8B8));

  auto draw_quad = [this, draw_texture](uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    draw_texture(width, height);

    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetImageDimensions(width, height);
    host_.SetupTextureStages();

    host_.SetDiffuse(0xFFFFFF00);

    const auto kWidth = static_cast<float>(width);
    const auto kHeight = static_cast<float>(height);
    const auto kLeft = static_cast<float>(x);
    const auto kTop = static_cast<float>(y);
    const auto kRight = kLeft + kWidth;
    const auto kBottom = kTop + kHeight;

    static constexpr float kZ = 0.0f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(kLeft, kTop, kZ, 1.0f);
    host_.SetTexCoord0(kWidth, 0.0f);
    host_.SetVertex(kRight, kTop, kZ, 1.0f);
    host_.SetTexCoord0(kWidth, kHeight);
    host_.SetVertex(kRight, kBottom, kZ, 1.0f);
    host_.SetTexCoord0(0.0f, kHeight);
    host_.SetVertex(kLeft, kBottom, kZ, 1.0f);
    host_.End();
  };

  const float kQuadRegion = 128.0f;
  const float kSpacing = 16.0f;
  const float kHorizontalStart = floorf((host_.GetFramebufferWidthF() - (kQuadRegion + kSpacing)) * 0.5f);
  const float kHorizontalEnd = kHorizontalStart * 2;
  float left = kHorizontalStart;
  float top = floorf((host_.GetFramebufferHeightF() - (kQuadRegion + kSpacing)) * 0.5f);

  static constexpr uint32_t kGeometries[][2] = {
      {128, 128},
      {128, 32},
      {32, 128},
      {16, 64},
  };

  for (auto geometry : kGeometries) {
    draw_quad(static_cast<uint32_t>(left), static_cast<uint32_t>(top), geometry[0], geometry[1]);
    left += kQuadRegion + kSpacing;
    if (left >= kHorizontalEnd) {
      left = kHorizontalStart;
      top += kQuadRegion + kSpacing;
    }
  }

  std::string test_name = MakeGeometryTestName(kTestGeometrySubscreenName, bias);
  pb_print("%s\n", test_name.c_str());
  pb_draw_text_screen();

  FinishDraw(test_name);
}

void VertexShaderRoundingTests::TestGeometrySuperscreen(float bias) {
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
  host_.PrepareDraw(0xFE414041);

  static constexpr uint32_t kTextureSize = 1024;

  auto draw_texture = [this, bias](uint32_t draw_width, uint32_t draw_height) {
    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
    host_.SetupTextureStages();

    const uint32_t kTexturePitch = kTextureSize * 4;
    const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, kTextureSize, kTextureSize);
    host_.SetWindowClip(kTextureSize - 1, kTextureSize - 1);

    // Point the color buffer at the texture and mix the left hand side with itself multiple times.
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kTexturePitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(host_.GetTextureMemory()));
    Pushbuffer::Push(NV097_NO_OPERATION, 0);
    Pushbuffer::Push(NV097_WAIT_FOR_IDLE, 0);
    Pushbuffer::End();

    // Initialize the texture to magenta.
    auto pixel = reinterpret_cast<uint32_t *>(host_.GetTextureMemory());
    for (uint32_t y = 0; y < draw_height; ++y) {
      for (uint32_t x = 0; x < draw_width; ++x, ++pixel) {
        *pixel = 0xFFFF00FF;
      }
    }

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

    // Draw a subpixel offset green square.
    static constexpr uint32_t kColor = 0xFF009900;
    static constexpr float kZ = 0.0f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(kColor);
    host_.SetVertex(bias, bias, kZ, 1.0f);
    host_.SetVertex(static_cast<float>(draw_width) + bias, bias, kZ, 1.0f);
    host_.SetVertex(static_cast<float>(draw_width) + bias, static_cast<float>(draw_height) + bias, kZ, 1.0f);
    host_.SetVertex(bias, static_cast<float>(draw_height) + bias, kZ, 1.0f);
    host_.End();

    host_.SetWindowClip(host_.GetFramebufferWidth() - 1, host_.GetFramebufferHeight() - 1);
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                    host_.GetFramebufferHeight());

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
    Pushbuffer::Push(NV097_NO_OPERATION, 0);
    Pushbuffer::Push(NV097_WAIT_FOR_IDLE, 0);
    Pushbuffer::End();
  };

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X8R8G8B8));

  auto draw_quad = [this, draw_texture](uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    draw_texture(width, height);

    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
    host_.SetupTextureStages();

    host_.SetDiffuse(0xFFFFFF00);

    const auto kWidth = static_cast<float>(width);
    const auto kHeight = static_cast<float>(height);
    const auto kLeft = static_cast<float>(x);
    const auto kTop = static_cast<float>(y);
    const auto kRight = kLeft + kWidth;
    const auto kBottom = kTop + kHeight;

    static constexpr float kZ = 0.0f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(kLeft, kTop, kZ, 1.0f);
    host_.SetTexCoord0(kWidth, 0.0f);
    host_.SetVertex(kRight, kTop, kZ, 1.0f);
    host_.SetTexCoord0(kWidth, kHeight);
    host_.SetVertex(kRight, kBottom, kZ, 1.0f);
    host_.SetTexCoord0(0.0f, kHeight);
    host_.SetVertex(kLeft, kBottom, kZ, 1.0f);
    host_.End();
  };

  const float kQuadRegion = 128.0f;
  const float kSpacing = 16.0f;
  const float kHorizontalStart = floorf((host_.GetFramebufferWidthF() - (kQuadRegion + kSpacing)) * 0.5f);
  const float kHorizontalEnd = kHorizontalStart * 2;
  float left = kHorizontalStart;
  float top = floorf((host_.GetFramebufferHeightF() - (kQuadRegion + kSpacing)) * 0.5f);

  static constexpr uint32_t kGeometries[][2] = {
      {128, 128},
      {128, 32},
      {32, 128},
      {16, 64},
  };

  for (auto geometry : kGeometries) {
    draw_quad(static_cast<uint32_t>(left), static_cast<uint32_t>(top), geometry[0], geometry[1]);
    left += kQuadRegion + kSpacing;
    if (left >= kHorizontalEnd) {
      left = kHorizontalStart;
      top += kQuadRegion + kSpacing;
    }
  }

  std::string test_name = MakeGeometryTestName(kTestGeometrySuperscreenName, bias);
  pb_print("%s\n", test_name.c_str());
  pb_draw_text_screen();

  FinishDraw(test_name);
}

void VertexShaderRoundingTests::TestRenderTarget() {
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  {
    auto *pixel = reinterpret_cast<uint32_t *>(render_target_);
    for (uint32_t i = 0; i < kTextureWidth * kTextureHeight; ++i) {
      *pixel++ = 0x7FFF00FF;
    }
  }

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.PrepareDraw(0xFE202020);

  // Redirect the color output to the target texture.
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kTexturePitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kTexturePitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(render_target_));
    // TODO: Investigate if this is actually necessary. Morrowind does this after changing offsets.
    Pushbuffer::Push(NV097_NO_OPERATION, 0);
    Pushbuffer::Push(NV097_WAIT_FOR_IDLE, 0);
    Pushbuffer::End();

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kTextureWidth, kTextureHeight, true);

    host_.SetWindowClip(kTextureWidth - 1, kTextureHeight - 1);

    // This is what triggers the erroneous (rounding error) offset
    host_.SetViewportOffset(320.531250f, 240.531250f, 0.0f, 0.0f);
    host_.SetViewportScale(320.0f, -240.0f, 16777215.0f, 0.0f);
    host_.SetDepthClip(0.0f, 16777215.0f);
  }

  // Manually load the vertex shader.
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_TRANSFORM_PROGRAM_START, 0x0);
    Pushbuffer::Push(
        NV097_SET_TRANSFORM_EXECUTION_MODE,
        MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM) |
            MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));
    Pushbuffer::Push(NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0);

    Pushbuffer::Push(NV097_SET_TRANSFORM_PROGRAM_LOAD, 0x0);

    //  MOV(oD0,xyzw, v3);
    Pushbuffer::Push(NV097_SET_TRANSFORM_PROGRAM, 0x00000000, 0x0020061B, 0x0836106C, 0x2070F818);

    //  MOV(oPos,xyzw, v0);
    Pushbuffer::Push(NV097_SET_TRANSFORM_PROGRAM, 0x00000000, 0x0020001B, 0x0836106C, 0x2070F800);

    //  MUL(oPos,xyz, R12, c[58]);
    //  RCC(R1,x, R12.w);
    Pushbuffer::Push(NV097_SET_TRANSFORM_PROGRAM, 0x00000000, 0x0647401B, 0xC4361BFF, 0x1078E800);

    // MAD(oPos,xyz, R12, R1.x, c[59]);
    Pushbuffer::Push(NV097_SET_TRANSFORM_PROGRAM, 0x00000000, 0x0087601B, 0xC400286C, 0x3070E801);
    Pushbuffer::End();
  }

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  // Render a full surface quad into the render target.
  {
    const float kLeft = -1.0f;
    const float kRight = 1.0f;
    const float kTop = 1.0f;
    const float kBottom = -1.0f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFF887733);
    host_.SetVertex(kLeft, kTop, 0.1f, 1.0f);

    host_.SetVertex(kRight, kTop, 0.1f, 1.0f);

    host_.SetVertex(kRight, kBottom, 0.1f, 1.0f);

    host_.SetVertex(kLeft, kBottom, 0.1f, 1.0f);
    host_.End();
  }

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);

  // Render the render buffer from the previous draw call onto the screen.
  {
    host_.SetVertexShaderProgram(nullptr);
    host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

    host_.SetWindowClip(host_.GetFramebufferWidth() - 1, host_.GetFramebufferHeight() - 1);
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X8R8G8B8));
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    texture_stage.SetImageDimensions(kTextureWidth, kTextureHeight);
    host_.SetupTextureStages();

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
    Pushbuffer::End();
    host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                           host_.GetFramebufferHeight());

    host_.SetRawTexture(render_target_, kTextureWidth, kTextureHeight, 1, kTexturePitch, 4, false);

    host_.SetVertexBuffer(framebuffer_vertex_buffer_);

    host_.PrepareDraw(0xFE202020);
    host_.DrawArrays();
  }

  pb_print("%s\n", kTestRenderTargetName);
  pb_draw_text_screen();

  FinishDraw(kTestRenderTargetName);
}

void VertexShaderRoundingTests::TestCompositingRenderTarget(int z) {
  static constexpr uint32_t kNumCompositingIterations = 4;
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

  {
    memset(host_.GetTextureMemory(), 0, host_.GetTextureMemorySize());
    static constexpr uint32_t kCheckerboardWidth = 256;
    static constexpr uint32_t kCheckerboardHeight = 256;
    uint32_t x = (host_.GetFramebufferWidth() - kCheckerboardWidth) / 2;
    uint32_t y = (host_.GetFramebufferHeight() - kCheckerboardHeight) / 2;
    GenerateRGBACheckerboard(host_.GetTextureMemory(), x, y, kCheckerboardWidth, kCheckerboardHeight,
                             kFramebufferPitch);
  }

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());
  host_.PrepareDraw(0xFE202020);

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetTextureDimensions(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  texture_stage.SetImageDimensions(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.SetupTextureStages();

  host_.SetWindowClip(host_.GetFramebufferWidth() - 1, host_.GetFramebufferHeight() - 1);

  // Point the color buffer at the texture and mix the left hand side with itself multiple times.
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, reinterpret_cast<uint32_t>(host_.GetTextureMemory()) & 0x03FFFFFF);

    // TODO: Investigate if this is actually necessary. Morrowind does this after changing offsets.
    Pushbuffer::Push(NV097_NO_OPERATION, 0);
    Pushbuffer::Push(NV097_WAIT_FOR_IDLE, 0);
    Pushbuffer::End();

    host_.SetCombinerFactorC1(0, 0.0f, 0.0f, 0.0f, 0.5f);
    host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_TEX0), TestHost::OneInput());
    host_.SetOutputColorCombiner(0, TestHost::DST_R0);
    host_.SetInputAlphaCombiner(0, TestHost::AlphaInput(TestHost::SRC_TEX0), TestHost::AlphaInput(TestHost::SRC_C1));
    host_.SetOutputAlphaCombiner(0, TestHost::DST_R0);
    host_.SetFinalCombiner0Just(TestHost::SRC_R0);
    host_.SetFinalCombiner1Just(TestHost::SRC_R0, true);

    const float kLeft = 0.0f;
    const float kRight = floorf(host_.GetFramebufferWidthF() * 0.5f);
    const float kTop = 0.0f;
    const float kBottom = host_.GetFramebufferHeightF();

    for (uint32_t i = 0; i < kNumCompositingIterations; ++i) {
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetTexCoord0(kLeft, kTop);
      host_.SetVertex(kLeft, kTop, 0, 1.0f);

      host_.SetTexCoord0(kRight, kTop);
      host_.SetVertex(kRight, kTop, 0, 1.0f);

      host_.SetTexCoord0(kRight, kBottom);
      host_.SetVertex(kRight, kBottom, 0, 1.0f);

      host_.SetTexCoord0(kLeft, kBottom);
      host_.SetVertex(kLeft, kBottom, 0, 1.0f);
      host_.End();
    }
  }

  // Restore the color buffer and render one last time to the screen.
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
  // TODO: Investigate if this is actually necessary. Morrowind does this after changing offsets.
  Pushbuffer::Push(NV097_NO_OPERATION, 0);
  Pushbuffer::Push(NV097_WAIT_FOR_IDLE, 0);
  Pushbuffer::End();

  {
    const float left = -1.75;
    const float top = 1.75;
    const float right = 1.75;
    const float bottom = -1.75;
    const auto depth = static_cast<float>(z);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(left, top, depth, 1.0f);

    host_.SetTexCoord0(host_.GetFramebufferWidthF(), 0.0f);
    host_.SetVertex(right, top, depth, 1.0f);

    host_.SetTexCoord0(host_.GetFramebufferWidthF(), host_.GetFramebufferHeightF());
    host_.SetVertex(right, bottom, depth, 1.0f);

    host_.SetTexCoord0(0.0f, host_.GetFramebufferHeightF());
    host_.SetVertex(left, bottom, depth, 1.0f);
    host_.End();
  }

  std::string name = MakeCompositingRenderTargetTestName(z);
  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  FinishDraw(name);
}

void VertexShaderRoundingTests::TestAdjacentGeometry(float bias) {
  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.PrepareDraw(0xFE404040);

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  static constexpr float kQuadSize = 100.0f;

  const float kMidX = floorf(host_.GetFramebufferWidthF() * 0.5f);
  const float kMidY = floor(host_.GetFramebufferHeightF() * 0.5f);
  const float kRowStart = kMidX - (kQuadSize * 2);
  float left = kRowStart;
  float top = kMidY - kQuadSize;
  const float kZ = 1;
  const float kBackgroundZ = kZ + 0.0001f;

  // Draw a background.
  uint32_t color = 0xFFE0E0E0;
  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(color);
  host_.SetVertex(left, top, kBackgroundZ, 1.0f);
  host_.SetVertex(left + kQuadSize * 4.0f, top, kBackgroundZ, 1.0f);
  host_.SetVertex(left + kQuadSize * 4.0f, top + kQuadSize * 2.0f, kBackgroundZ, 1.0f);
  host_.SetVertex(left, top + kQuadSize * 2.0f, kBackgroundZ, 1.0f);
  host_.End();

  // Draw a subpixel offset green square.
  color = 0xFF009900;
  for (uint32_t y = 0; y < 2; ++y) {
    float bottom = top + kQuadSize;
    for (uint32_t x = 0; x < 4; ++x) {
      float right = left + kQuadSize;
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetDiffuse(color);
      host_.SetVertex(left + bias, top + bias, kZ, 1.0f);
      host_.SetVertex(right + bias, top + bias, kZ, 1.0f);
      host_.SetVertex(right + bias, bottom + bias, kZ, 1.0f);
      host_.SetVertex(left + bias, bottom + bias, kZ, 1.0f);
      host_.End();
      left += kQuadSize;
    }
    left = kRowStart;
    top = bottom;
  }

  std::string test_name = MakeGeometryTestName(kTestAdjacentGeometryName, bias);
  pb_print("%s\n", test_name.c_str());
  pb_draw_text_screen();

  FinishDraw(test_name);
}

/**
 * Draws a light colored quad, then renders quads whose coordinates have had
 * `bias` added to X and Y. Dark green quads are rendered using a programmable
 * vertex shader and light green quads are rendered using the fixed function
 * pipeline.
 *
 * @param bias amount to shift the green quads relative to the white background.
 */
void VertexShaderRoundingTests::TestProjectedAdjacentGeometry(float bias) {
  float depth_buffer_max_value = host_.GetMaxDepthBufferValue();
  auto shader =
      std::make_shared<PerspectiveVertexShaderNoLighting>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                                          0.0f, depth_buffer_max_value, M_PI * 0.25f, 1.0f, 200.0f);
  {
    shader->SetUseD3DStyleViewport();
    vector_t camera_position = {0.0f, 0.0f, -7.0f, 1.0f};
    vector_t camera_look_at = {0.0f, 0.0f, 0.0f, 1.0f};
    shader->LookAt(camera_position, camera_look_at);
  }
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetVertexShaderProgram(shader);

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.PrepareDraw(0xFE404040);

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  static constexpr float kQuadSize = 100.0f;

  const float kMidX = floorf(host_.GetFramebufferWidthF() * 0.5f);
  const float kMidY = floor(host_.GetFramebufferHeightF() * 0.5f);
  const float kRowStart = kMidX - (kQuadSize * 2);
  float left = kRowStart;
  float top = kMidY - kQuadSize;
  const float kZBottom = 0.0f;
  const float kZTop = 10.0f;
  const float kBackgroundZBottom = kZBottom + 0.0001f;
  const float kBackgroundZTop = kZTop + 0.0001f;

  auto set_vertex = [this](float x, float y, float z) {
    vector_t world;
    vector_t screen_point = {x, y, z, 1.0f};
    host_.UnprojectPoint(world, screen_point, z);
    host_.SetVertex(world[0], world[1], z, 1.0f);
  };

  // Draw a light background to make it obvious where quads are misaligned.
  // The background is a single quad whose z value ranges from kBackgroundZTop
  // along the top to kBackgroundZBottom along the bottom edge.
  uint32_t color = 0xFFE0E0E0;
  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(color);
  set_vertex(left, top, kBackgroundZTop);
  set_vertex(left + kQuadSize * 4.0f, top, kBackgroundZTop);
  set_vertex(left + kQuadSize * 4.0f, top + kQuadSize * 2.0f, kBackgroundZBottom);
  set_vertex(left, top + kQuadSize * 2.0f, kBackgroundZBottom);
  host_.End();

  float z_inc = kZTop - kZBottom * 0.5f;
  float bottom_z = kZBottom;

  struct Quad {
    vector_t ul;
    vector_t ur;
    vector_t lr;
    vector_t ll;
  };

  Quad quads[8];
  for (uint32_t y = 0, i = 0; y < 2; ++y) {
    float bottom = top + kQuadSize;
    float top_z = bottom_z + z_inc;

    for (uint32_t x = 0; x < 4; ++x, ++i) {
      float right = left + kQuadSize;

      {
        vector_t world_point = {left + bias, top + bias, top_z, 1.f};
        host_.UnprojectPoint(quads[i].ul, world_point, world_point[2]);
      }

      {
        vector_t world_point = {right + bias, top + bias, top_z, 1.f};
        host_.UnprojectPoint(quads[i].ur, world_point, world_point[2]);
      }

      {
        vector_t world_point = {right + bias, bottom + bias, bottom_z, 1.f};
        host_.UnprojectPoint(quads[i].lr, world_point, world_point[2]);
      }

      {
        vector_t world_point = {left + bias, bottom + bias, bottom_z, 1.f};
        host_.UnprojectPoint(quads[i].ll, world_point, world_point[2]);
      }

      left += kQuadSize;
    }
    left = kRowStart;
    bottom_z = kZBottom;
    top = bottom;
  }

  // Draw subpixel offset green squares using the programmable pipeline.
  {
    color = 0xFF009900;
    for (uint32_t i = 0; i < 4; i += 2) {
      auto &q = quads[i];
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetDiffuse(color);
      host_.SetVertex(q.ul[0], q.ul[1], q.ul[2]);
      host_.SetVertex(q.ur[0], q.ur[1], q.ur[2]);
      host_.SetVertex(q.lr[0], q.lr[1], q.lr[2]);
      host_.SetVertex(q.ll[0], q.ll[1], q.ll[2]);
      host_.End();
    }
    for (uint32_t i = 5; i < 8; i += 2) {
      auto &q = quads[i];
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetDiffuse(color);
      host_.SetVertex(q.ul[0], q.ul[1], q.ul[2]);
      host_.SetVertex(q.ur[0], q.ur[1], q.ur[2]);
      host_.SetVertex(q.lr[0], q.lr[1], q.lr[2]);
      host_.SetVertex(q.ll[0], q.ll[1], q.ll[2]);
      host_.End();
    }
  }

  // Draw subpixel offset green squares using the fixed pipeline.
  color = 0xFF00BB00;
  host_.SetVertexShaderProgram(nullptr);
  for (uint32_t i = 1; i < 4; i += 2) {
    auto &q = quads[i];
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(color);
    host_.SetVertex(q.ul[0], q.ul[1], q.ul[2]);
    host_.SetVertex(q.ur[0], q.ur[1], q.ur[2]);
    host_.SetVertex(q.lr[0], q.lr[1], q.lr[2]);
    host_.SetVertex(q.ll[0], q.ll[1], q.ll[2]);
    host_.End();
  }
  for (uint32_t i = 4; i < 8; i += 2) {
    auto &q = quads[i];
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(color);
    host_.SetVertex(q.ul[0], q.ul[1], q.ul[2]);
    host_.SetVertex(q.ur[0], q.ur[1], q.ur[2]);
    host_.SetVertex(q.lr[0], q.lr[1], q.lr[2]);
    host_.SetVertex(q.ll[0], q.ll[1], q.ll[2]);
    host_.End();
  }

  std::string test_name = MakeGeometryTestName(kTestProjectedAdjacentGeometryName, bias);
  pb_print("%s\n", test_name.c_str());
  pb_print("Dark green: Programmable Light: FF\n");
  pb_draw_text_screen();

  FinishDraw(test_name);
}

static float decrease(float f) {
  uint32_t u = *(uint32_t *)&f;
  u--;
  return *(float *)&u;
}

void VertexShaderRoundingTests::TestTopLeftRasterization(bool fixed) {
  if (fixed) {
    host_.SetVertexShaderProgram(nullptr);
    host_.SetWindowClip(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
    host_.SetViewportOffset(0, 0, 0, 0);
    host_.SetViewportScale(0, 0, 0, 0);
    matrix4_t matrix;
    MatrixSetIdentity(matrix);
    host_.SetFixedFunctionModelViewMatrix(matrix);
    host_.SetFixedFunctionProjectionMatrix(matrix);
  } else {
    auto shader = std::make_shared<PassthroughVertexShader>();
    host_.SetVertexShaderProgram(shader);
  }

  host_.PrepareDraw(0xFF202224);

  float x = 4.0f;
  float dx = 4.0f;
  float y0 = 45.0f;
  float y1 = 135.0f;
  float y2 = 315.0f;
  float y3 = 450.0f;

  for (int i = 0; i < 20; i++) {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(decrease(x + 0.5f), y0, 1.0f);
    host_.SetVertex(x + 1.9f, y0, 1.0f);
    host_.SetVertex(x + 1.9f, y2, 1.0f);
    host_.SetVertex(x + 0.5f + i * 0.0625f, y2, 1.0f);
    host_.End();

    x += dx;
  }

  {
    host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
    for (int i = 0; i < 16; i++) {
      host_.SetDiffuse(0xFFFFFFFF);
      host_.SetVertex(decrease(x + 0.5f - i * 0.0625f), y0, 1.0f);
      host_.SetVertex(x + 0.5625f + i * 0.0625f, y3, 1.0f);
      host_.SetVertex(decrease(x + 0.5f - i * 0.0625f), y3, 1.0f);
      x += dx;
    }
    host_.End();
  }

  {
    // Should be invisible when MSAA not used

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(x + 0.9f, y0, 1.0f);
    host_.SetVertex(x + 1.1f, y0, 1.0f);
    host_.SetVertex(x + 1.1f, y3, 1.0f);
    host_.SetVertex(x + 0.9f, y3, 1.0f);

    x += dx;
    host_.SetVertex(x, y0, 1.0f);
    host_.SetVertex(x + 0.5f, y0, 1.0f);
    host_.SetVertex(x + 0.5f, y3, 1.0f);
    host_.SetVertex(x, y3, 1.0f);
    host_.End();

    x += dx;
  }

  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(x + 0.4f, y0, 1.0f);
    host_.SetVertex(x + 0.6f, y0, 1.0f);
    host_.SetVertex(x + 0.6f, y3, 1.0f);
    host_.SetVertex(x + 0.4f, y3, 1.0f);
    host_.End();

    x += dx;
  }

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(0xFFFFFFFF);
  host_.SetVertex(x + 0.5f, y0, 1.0f);
  host_.SetVertex(x + 2.4f, y0, 1.0f);
  host_.SetVertex(x + 2.4f, y1, 1.0f);
  host_.SetVertex(x + 0.5f, y1, 1.0f);

  host_.SetVertex(decrease(x + 0.5625f), y1, 1.0f);
  host_.SetVertex(x + 2.4f, y1, 1.0f);
  host_.SetVertex(x + 2.4f, y2, 1.0f);
  host_.SetVertex(decrease(x + 0.5625f), y2, 1.0f);

  host_.SetVertex(x + 0.5625f, y2, 1.0f);
  host_.SetVertex(x + 2.4f, y2, 1.0f);
  host_.SetVertex(x + 2.4f, y3, 1.0f);
  host_.SetVertex(x + 0.5625f, y3, 1.0f);

  x += dx;
  host_.SetVertex(x + 0.5f, y0, 1.0f);
  host_.SetVertex(x + 1.0f, y0, 1.0f);
  host_.SetVertex(x + 1.0f, y1, 1.0f);
  host_.SetVertex(x + 0.5f, y1, 1.0f);

  host_.SetVertex(x + 0.5f, y1, 1.0f);
  host_.SetVertex(decrease(x + 1.5625f), y1, 1.0f);
  host_.SetVertex(decrease(x + 1.5625f), y2, 1.0f);
  host_.SetVertex(x + 0.5f, y2, 1.0f);

  host_.SetVertex(x + 0.5f, y2, 1.0f);
  host_.SetVertex(x + 1.5625f, y2, 1.0f);
  host_.SetVertex(x + 1.5625f, y3, 1.0f);
  host_.SetVertex(x + 0.5f, y3, 1.0f);

  host_.End();

  x += 5 * dx;

  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);

    // Expected interpolated RGB 0x808080 (due to vertex rounding)
    host_.SetDiffuse(0xFF000000);
    host_.SetVertex(decrease(x + 0.5f), 100.0f, 1.0f);
    host_.SetVertex(decrease(x + 0.5f), 50.0f, 1.0f);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(x + 0.5625f, 50.0f, 1.0f);
    host_.SetVertex(x + 0.5625f, 100.0f, 1.0f);

    // Expected interpolated RGB 0x808080
    host_.SetDiffuse(0xFF000000);
    host_.SetVertex(x + 0.4375f, 200.0f, 1.0f);
    host_.SetVertex(x + 0.4375f, 150.0f, 1.0f);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(x + 0.5625f, 150.0f, 1.0f);
    host_.SetVertex(x + 0.5625f, 200.0f, 1.0f);

    // Expected interpolated RGB 0x000000
    host_.SetDiffuse(0xFF000000);
    host_.SetVertex(x + 0.5f, 300.0f, 1.0f);
    host_.SetVertex(x + 0.5f, 250.0f, 1.0f);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(x + 0.5625f, 250.0f, 1.0f);
    host_.SetVertex(x + 0.5625f, 300.0f, 1.0f);

    host_.End();
  }

  float y = y0;
  float dy = 4.0f;
  float x0 = 320.0f;
  float x1 = 420.0f;
  float x2 = 520.0f;
  float x3 = 620.0f;

  for (int i = 0; i < 20; i++) {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(x0, y + 1.9f, 1.0f);
    host_.SetVertex(x0, decrease(y + 0.5f), 1.0f);
    host_.SetVertex(x2, y + 0.5f + i * 0.0625f, 1.0f);
    host_.SetVertex(x2, y + 1.9f, 1.0f);
    host_.End();

    y += dy;
  }

  {
    host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
    for (int i = 0; i < 16; i++) {
      host_.SetDiffuse(0xFFFFFFFF);
      host_.SetVertex(x0, decrease(y + 0.5f - i * 0.0625f), 1.0f);
      host_.SetVertex(x3, decrease(y + 0.5f - i * 0.0625f), 1.0f);
      host_.SetVertex(x3, y + 0.5625f + i * 0.0625f, 1.0f);
      y += dy;
    }
    host_.End();
  }

  {
    // Should be invisible when MSAA not used

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(x3, y + 0.9f, 1.0f);
    host_.SetVertex(x3, y + 1.1f, 1.0f);
    host_.SetVertex(x0, y + 1.1f, 1.0f);
    host_.SetVertex(x0, y + 0.9f, 1.0f);

    y += dy;
    host_.SetVertex(x3, y, 1.0f);
    host_.SetVertex(x3, y + 0.5f, 1.0f);
    host_.SetVertex(x0, y + 0.5f, 1.0f);
    host_.SetVertex(x0, y, 1.0f);
    host_.End();

    y += dy;
  }

  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(x3, y + 0.4f, 1.0f);
    host_.SetVertex(x3, y + 0.6f, 1.0f);
    host_.SetVertex(x0, y + 0.6f, 1.0f);
    host_.SetVertex(x0, y + 0.4f, 1.0f);
    host_.End();

    y += dy;
  }

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(0xFFFFFFFF);
  host_.SetVertex(x1, y + 0.5f, 1.0f);
  host_.SetVertex(x1, y + 2.4f, 1.0f);
  host_.SetVertex(x0, y + 2.4f, 1.0f);
  host_.SetVertex(x0, y + 0.5f, 1.0f);

  host_.SetVertex(x2, decrease(y + 0.5625f), 1.0f);
  host_.SetVertex(x2, y + 2.4f, 1.0f);
  host_.SetVertex(x1, y + 2.4f, 1.0f);
  host_.SetVertex(x1, decrease(y + 0.5625f), 1.0f);

  host_.SetVertex(x3, y + 0.5625f, 1.0f);
  host_.SetVertex(x3, y + 2.4f, 1.0f);
  host_.SetVertex(x2, y + 2.4f, 1.0f);
  host_.SetVertex(x2, y + 0.5625f, 1.0f);

  y += dy;
  host_.SetVertex(x1, y + 0.5f, 1.0f);
  host_.SetVertex(x1, y + 1.0f, 1.0f);
  host_.SetVertex(x0, y + 1.0f, 1.0f);
  host_.SetVertex(x0, y + 0.5f, 1.0f);

  host_.SetVertex(x2, y + 0.5f, 1.0f);
  host_.SetVertex(x2, decrease(y + 1.5625f), 1.0f);
  host_.SetVertex(x1, decrease(y + 1.5625f), 1.0f);
  host_.SetVertex(x1, y + 0.5f, 1.0f);

  host_.SetVertex(x3, y + 0.5f, 1.0f);
  host_.SetVertex(x3, y + 1.5625f, 1.0f);
  host_.SetVertex(x2, y + 1.5625f, 1.0f);
  host_.SetVertex(x2, y + 0.5f, 1.0f);

  host_.End();

  y += 5 * dy;

  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);

    // Expected interpolated RGB 0x808080 (due to vertex rounding)
    host_.SetDiffuse(0xFF000000);
    host_.SetVertex(320.0f, decrease(y + 0.5f), 1.0f);
    host_.SetVertex(380.0f, decrease(y + 0.5f), 1.0f);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(380.0f, y + 0.5625f, 1.0f);
    host_.SetVertex(320.0f, y + 0.5625f, 1.0f);

    // Expected interpolated RGB 0x808080
    host_.SetDiffuse(0xFF000000);
    host_.SetVertex(440.0f, y + 0.4375f, 1.0f);
    host_.SetVertex(500.0f, y + 0.4375f, 1.0f);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(500.0f, y + 0.5625f, 1.0f);
    host_.SetVertex(440.0f, y + 0.5625f, 1.0f);

    // Expected interpolated RGB 0x000000
    host_.SetDiffuse(0xFF000000);
    host_.SetVertex(560.0f, y + 0.5f, 1.0f);
    host_.SetVertex(620.0f, y + 0.5f, 1.0f);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(620.0f, y + 0.5625f, 1.0f);
    host_.SetVertex(560.0f, y + 0.5625f, 1.0f);

    host_.End();
  }

  {
    // two pixels high with top-left rastization rule
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(10.0f, 455.5f, 1.0f);
    host_.SetVertex(630.0f, 455.5f, 1.0f);
    host_.SetVertex(630.0f, 456.6f, 1.0f);
    host_.SetVertex(10.0f, 456.6f, 1.0f);
    host_.End();
  }

  {
    // two pixels high with bottom-left rastization rule
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(10.0f, 460.4f, 1.0f);
    host_.SetVertex(630.0f, 460.4f, 1.0f);
    host_.SetVertex(630.0f, 461.5f, 1.0f);
    host_.SetVertex(10.0f, 461.5f, 1.0f);
    host_.End();
  }

  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);

    // Expected interpolated RGB 0x878787
    host_.SetDiffuse(0xFF000000);
    host_.SetVertex(-0.0625f, 100.0f, 1.0f);
    host_.SetVertex(-0.0625f, 50.0f, 1.0f);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(1.0f, 50.0f, 1.0f);
    host_.SetVertex(1.0f, 100.0f, 1.0f);

    // Expected interpolated RGB 0x808080 (due to vertex rounding)
    host_.SetDiffuse(0xFF000000);
    host_.SetVertex(-0.0624f, 200.0f, 1.0f);
    host_.SetVertex(-0.0624f, 150.0f, 1.0f);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(1.0f, 150.0f, 1.0f);
    host_.SetVertex(1.0f, 200.0f, 1.0f);

    // Expected interpolated RGB 0x808080
    host_.SetDiffuse(0xFF000000);
    host_.SetVertex(0.0f, 300.0f, 1.0f);
    host_.SetVertex(0.0f, 250.0f, 1.0f);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(1.0f, 250.0f, 1.0f);
    host_.SetVertex(1.0f, 300.0f, 1.0f);

    host_.End();
  }

  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);

    // Expected interpolated RGB 0x878787
    host_.SetDiffuse(0xFF000000);
    host_.SetVertex(320.0f, -0.0625f, 1.0f);
    host_.SetVertex(380.0f, -0.0625f, 1.0f);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(380.0f, 1.0f, 1.0f);
    host_.SetVertex(320.0f, 1.0f, 1.0f);

    // Expected interpolated RGB 0x808080 (due to vertex rounding)
    host_.SetDiffuse(0xFF000000);
    host_.SetVertex(440.0f, -0.0624f, 1.0f);
    host_.SetVertex(500.0f, -0.0624f, 1.0f);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(500.0f, 1.0f, 1.0f);
    host_.SetVertex(440.0f, 1.0f, 1.0f);

    // Expected interpolated RGB 0x808080
    host_.SetDiffuse(0xFF000000);
    host_.SetVertex(560.0f, 0.0f, 1.0f);
    host_.SetVertex(620.0f, 0.0f, 1.0f);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(620.0f, 1.0f, 1.0f);
    host_.SetVertex(560.0f, 1.0f, 1.0f);

    host_.End();
  }

  std::string test_name = MakeTopLeftRasterTestName(fixed);
  pb_print("%s\n", test_name.c_str());
  pb_draw_text_screen();

  FinishDraw(test_name);
}

void VertexShaderRoundingTests::TestOpRounding(const char *name, const uint32_t *shader, uint32_t shader_size) {
  // The shaders output z = d * kErrorScale + kDepthOffset. kErrorScale makes 1 unit of least precision of c an integer
  // number of depth units (2^(exponent + 5)) and kDepthOffset keeps any small error within the depth range.
  static constexpr float kErrorScale = 268435456.f;  // 2^28
  static constexpr int32_t kDepthOffset = 0x800000;
  static constexpr uint32_t kNotDrawn = 0xFFFFFFFF;
  static constexpr uint32_t kSurfaceWidth = 256;
  static constexpr uint32_t kSurfaceHeight = 32;
  static constexpr uint32_t kQuadSize = 16;
  static constexpr uint32_t kQuadSpacing = 48;
  static constexpr uint32_t kNumCases = sizeof(kRoundingCases) / sizeof(kRoundingCases[0]);
  static constexpr uint32_t kZetaSize = kSurfaceWidth * kSurfaceHeight * 4;

  auto as_float = [](uint32_t value) {
    float ret;
    memcpy(&ret, &value, sizeof(ret));
    return ret;
  };

  auto zeta = host_.GetTextureMemory();
  memset(zeta, 0xFF, kZetaSize);

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  auto program = std::make_shared<VertexShaderProgram>(shader, shader_size);
  host_.SetVertexShaderProgram(program);

  host_.PrepareDraw(0xFE404040);

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  host_.RenderToSurfaceStart(render_target_, TestHost::SCF_A8R8G8B8, zeta, TestHost::SZF_Z24S8, kSurfaceWidth,
                             kSurfaceHeight);
  host_.SetDepthClip(0.f, 16777215.f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_ALWAYS);
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, true);
  Pushbuffer::End();

  // Operands are supplied as shader constants, which are passed through verbatim: c[96] = (a, c, 0, scale),
  // c[97] = (b, -1, 0, offset).
  for (uint32_t i = 0; i < kNumCases; ++i) {
    const auto &test_case = kRoundingCases[i];
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_TRANSFORM_CONSTANT_LOAD, VertexShaderProgram::kShaderUserConstantOffset);
    Pushbuffer::PushF(NV097_SET_TRANSFORM_CONSTANT, as_float(test_case.a), as_float(test_case.c), 0.f, kErrorScale);
    Pushbuffer::PushF(NV097_SET_TRANSFORM_CONSTANT + 16, as_float(test_case.b), -1.f, 0.f,
                      static_cast<float>(kDepthOffset));
    Pushbuffer::End();

    const auto left = static_cast<float>(i * kQuadSpacing);
    const auto right = left + static_cast<float>(kQuadSize);
    const auto bottom = static_cast<float>(kQuadSize);
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetVertex(left, 0.f, 0.f, 1.f);
    host_.SetVertex(right, 0.f, 0.f, 1.f);
    host_.SetVertex(right, bottom, 0.f, 1.f);
    host_.SetVertex(left, bottom, 0.f, 1.f);
    host_.End();
  }

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, false);
  Pushbuffer::End();
  host_.RenderToSurfaceEnd();
  host_.PBKitBusyWait();

  auto readback = static_cast<uint32_t *>(MmAllocateContiguousMemoryEx(kZetaSize, 0, MAXRAM, 0, PAGE_READWRITE));
  ASSERT(readback && "Failed to allocate zeta readback buffer");
  ASSERT(gpum_copy(readback, zeta, kZetaSize) && "Failed to copy zeta with M2M");

  pb_print("%s\n\n", name);
  pb_print("d = op(a,b) - RN(a*b), IEEE round to nearest: d = 0\n\n");
  pb_print("Case     Depth - offset   d (ulp)\n");
  for (uint32_t i = 0; i < kNumCases; ++i) {
    const auto &test_case = kRoundingCases[i];
    const uint32_t value = readback[(kQuadSize / 2) * kSurfaceWidth + i * kQuadSpacing + kQuadSize / 2];
    if (value == kNotDrawn) {
      pb_print("%s   not drawn\n", test_case.label);
      continue;
    }

    const int32_t delta = static_cast<int32_t>(value >> 8) - kDepthOffset;
    const int32_t exponent = static_cast<int32_t>((test_case.c >> 23) & 0xFF) - 127;
    const auto units_per_ulp = static_cast<float>(1 << (exponent + 5));
    pb_print("%s   %14d   %.2f\n", test_case.label, delta, static_cast<float>(delta) / units_per_ulp);
  }
  pb_draw_text_screen();

  MmFreeContiguousMemory(readback);

  FinishDraw(name);
}

static std::string MakeGeometryTestName(const char *prefix, float bias) {
  char buf[32] = {0};
  snprintf(buf, sizeof(buf), "%s_%.04f", prefix, bias);
  return buf;
}

static std::string MakeCompositingRenderTargetTestName(int z) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%s_%d", kTestCompositingRenderTargetName, z);
  return buf;
}

static std::string MakeTopLeftRasterTestName(bool fixed) {
  std::string test_name = kTestTopLeftRasterName;
  if (fixed) {
    test_name += "_Fixed";
  }
  return test_name;
}
