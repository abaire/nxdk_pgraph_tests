#include "null_surface_tests.h"

#include "shaders/passthrough_vertex_shader.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
static constexpr uint32_t kDefaultDMAChannelA = 3;
// From pbkit.c, DMA_COLOR is set to channel 9 by default
static constexpr uint32_t kDefaultDMAColorChannel = 9;
// NV097_SET_CONTEXT_DMA_ZETA is set to channel 10 by default.
static constexpr uint32_t kDefaultDMAZetaChannel = 10;

static constexpr const char kNullColorTest[] = "NullColor";
static constexpr const char kNullZetaTest[] = "NullZeta";
static constexpr const char kXemuBug893Test[] = "XemuBug893";

NullSurfaceTests::NullSurfaceTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Null surface", config) {
  //  tests_[kNullColorTest] = [this]() { TestNullColor(); };
  //  tests_[kNullZetaTest] = [this]() { TestNullZeta(); };
  tests_[kXemuBug893Test] = [this]() { TestXemuBug893(); };
}

void NullSurfaceTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
}

void NullSurfaceTests::TestNullColor() {
  host_.PrepareDraw(0xFE131413);
  SetSurfaceDMAs();
  RestoreSurfaceDMAs();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kNullColorTest);
}

void NullSurfaceTests::TestNullZeta() {
  host_.PrepareDraw(0xFE131414);
  SetSurfaceDMAs();
  RestoreSurfaceDMAs();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kNullZetaTest);
}

void NullSurfaceTests::TestXemuBug893() {
  host_.PrepareDraw(0xFE131415);

  static constexpr uint32_t kSurfaceWidth = 512;
  static constexpr uint32_t kSurfaceHeight = 512;
  static constexpr uint32_t kSurfacePitch = kSurfaceWidth * 4;
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

  const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemory());

  // Change the color and zeta DMA channels such that they can address all of vram.
  SetSurfaceDMAs();

  {
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kSurfaceWidth, kSurfaceHeight, false);

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kSurfacePitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kSurfacePitch));
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, reinterpret_cast<uint32_t>(pb_back_buffer()) & 0x03FFFFFF);
    Pushbuffer::Push(NV097_SET_SURFACE_ZETA_OFFSET, kTextureMemory & 0x03FFFFFF);
    // Note: Enabling depth testing is critical to reproducing the bug.
    Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, 1);
    Pushbuffer::Push(NV097_SET_COLOR_MASK, 0x1010101);
    Pushbuffer::End();
  }

  DrawTestQuad(false);

  // Set the color offset to something that overlaps but is not exactly equal to the zeta surface and clear out the
  // zeta surface.
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, (kTextureMemory + kSurfacePitch) & 0x03FFFFFF);
    Pushbuffer::Push(NV097_SET_SURFACE_ZETA_OFFSET, 0);
    Pushbuffer::Push(NV097_SET_DEPTH_MASK, 0);
    Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, 0);
    Pushbuffer::Push(NV097_SET_STENCIL_TEST_ENABLE, 0);
    Pushbuffer::End();
  }

  DrawTestQuad(false);

  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());

  RestoreSurfaceDMAs();
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
    Pushbuffer::Push(NV097_SET_SURFACE_ZETA_OFFSET, 0);
    Pushbuffer::Push(NV097_SET_DEPTH_MASK, 1);
    Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, 0);
    Pushbuffer::Push(NV097_SET_COLOR_MASK,
                     NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE | NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE |
                         NV097_SET_COLOR_MASK_RED_WRITE_ENABLE | NV097_SET_COLOR_MASK_ALPHA_WRITE_ENABLE);
    Pushbuffer::End();
  }

  host_.PrepareDraw(0xFE134415);

  DrawTestQuad(false);
  pb_print("Test passed\n");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kXemuBug893Test);
}

void NullSurfaceTests::SetSurfaceDMAs() const {
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAChannelA);
  Pushbuffer::End();
}

void NullSurfaceTests::RestoreSurfaceDMAs() const {
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAZetaChannel);
  Pushbuffer::End();
}

void NullSurfaceTests::DrawTestQuad(bool swizzle) const {
  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0.1f, 0.6f, 0.1f);
    auto left = 0.15f;
    auto top = 0.15f;
    auto right = 0.85f;
    auto bottom = 0.85f;

    if (!swizzle) {
      left *= host_.GetFramebufferWidthF();
      right *= host_.GetFramebufferWidthF();
      top *= host_.GetFramebufferHeightF();
      bottom *= host_.GetFramebufferHeightF();
    }

    host_.SetVertex(left, top, 0.1f, 1.0f);
    host_.SetVertex(right, top, 0.1f, 1.0f);
    host_.SetVertex(right, bottom, 0.1f, 1.0f);
    host_.SetVertex(left, bottom, 0.1f, 1.0f);
    host_.End();
  }
}
