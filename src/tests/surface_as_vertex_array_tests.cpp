#include "surface_as_vertex_array_tests.h"

#include <pbkit/pbkit.h>
#include <shaders/passthrough_vertex_shader.h>
#include <shaders/perspective_vertex_shader.h>

#include <cmath>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "test_host.h"
#include "vertex_buffer.h"

// clang-format off
static constexpr uint32_t kDisplacementShader[] = {
#include "surface_displacement.vshinc"




};
// clang-format on

class DisplacementVertexShader : public PBKitPlusPlus::PerspectiveVertexShader {
 public:
  static constexpr uint32_t kGridDim = 3;
  static constexpr uint32_t kPaletteSize = kGridDim * kGridDim;
  static constexpr uint32_t kIndexScaleSlot = 14;   // c[110]
  static constexpr uint32_t kPaletteBaseSlot = 16;  // c[112]

  DisplacementVertexShader(uint32_t framebuffer_width, uint32_t framebuffer_height)
      : PBKitPlusPlus::PerspectiveVertexShader(framebuffer_width, framebuffer_height, 0.f, 0x7FFF, M_PI * 0.25f, 0.5f,
                                               50.f) {
    SetShader(kDisplacementShader, sizeof(kDisplacementShader));
    SetTransposeOnUpload(true);

    for (uint32_t row = 0; row < kGridDim; ++row) {
      auto y = 1.2f - static_cast<float>(row) * 1.2f;
      for (uint32_t col = 0; col < kGridDim; ++col) {
        auto x = -1.5f + static_cast<float>(col) * 1.5f;
        uint32_t index = row * kGridDim + col;
        palette_[index][0] = x;
        palette_[index][1] = y;
        palette_[index][2] = 0.f;
        palette_[index][3] = 1.f;
      }
    }
  }

 protected:
  int OnLoadConstants() override {
    PerspectiveVertexShader::OnLoadConstants();
    vector_t index_scale{static_cast<float>(kPaletteSize), 0.f, 0.f, 1.f};
    SetBaseUniform4F(kIndexScaleSlot, index_scale);
    for (uint32_t i = 0; i < kPaletteSize; ++i) {
      SetBaseUniform4F(kPaletteBaseSlot + i, palette_[i]);
    }
    return kPaletteBaseSlot + kPaletteSize;
  }

 private:
  vector_t palette_[kPaletteSize];
};

static uint8_t *allocation_spacer{nullptr};

static constexpr char kLinearDiffuseTest[] = "LinearDiffuseArray";
static constexpr char kSwizzledDiffuseTest[] = "SwizzledDiffuseArray";
static constexpr char kMultiStreamTest[] = "MultiStream";
static constexpr char kDynamicUpdateLoopTest[] = "DynamicUpdateLoop";
static constexpr char kRenderScalePatternTest[] = "RenderScalePattern";

static constexpr uint32_t kSurfaceWidth = 64;
static constexpr uint32_t kSurfaceHeight = 64;
static constexpr uint32_t kSurfacePitch = kSurfaceWidth * 4;
static constexpr uint32_t kSurfaceSize = kSurfacePitch * kSurfaceHeight;

static void SetVertexAttribute(uint32_t index, uint32_t format, uint32_t size, uint32_t stride, const void *data) {
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_VERTEX_DATA_ARRAY_FORMAT + index * 4,
                   MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, format) |
                       MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, size) |
                       MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, stride));
  if (size && data) {
    Pushbuffer::Push(NV097_SET_VERTEX_DATA_ARRAY_OFFSET + index * 4, reinterpret_cast<uintptr_t>(data) & 0x03ffffff);
  }
  Pushbuffer::End();
}

static void ClearVertexAttribute(uint32_t index) {
  SetVertexAttribute(index, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 0, 0, nullptr);
}

static void DrawArrays(TestHost::DrawPrimitive primitive, uint32_t count, uint32_t start_index = 0) {
  static constexpr uint32_t kVerticesPerPush = 0xFF;

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_BREAK_VERTEX_BUFFER_CACHE, 0);
  Pushbuffer::Push(NV097_SET_BEGIN_END, primitive);

  uint32_t start = start_index;
  uint32_t end = start_index + count;
  while (start < end) {
    auto remaining = end - start;
    auto cur_count = remaining < kVerticesPerPush ? remaining : kVerticesPerPush;

    Pushbuffer::Push(NV097_DRAW_ARRAYS,
                     MASK(NV097_DRAW_ARRAYS_COUNT, cur_count - 1) | MASK(NV097_DRAW_ARRAYS_START_INDEX, start));

    start += cur_count;
  }

  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  Pushbuffer::End();
}

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc LinearDiffuseArray
 *  Renders four solid color strips (Red, Green, Blue, Yellow) into the first row of a linear A8R8G8B8 surface. The
 *  surface memory is then bound as NV2A_VERTEX_ATTR_DIFFUSE with TYPE_UB_D3D format and drawn via DRAW_ARRAYS.
 *  Expect to see four quads on a dark background: top-left Red, top-right Green, bottom-left Blue, bottom-right Yellow.
 *
 * @tc SwizzledDiffuseArray
 *  Renders four 2x2 color blocks into a swizzled A8R8G8B8 surface (paired with matching 32-bit depth buffer). Due to
 *  Morton order swizzling, the 2x2 blocks occupy contiguous 4-pixel chunks in memory. The surface is then bound as
 *  NV2A_VERTEX_ATTR_DIFFUSE.
 *  Expect to see four quads on a dark background: top-left Red, top-right Green, bottom-left Blue, bottom-right Yellow.
 *
 * @tc MultiStream
 *  Tests multiple simultaneous surface vertex streams. Surface A is rendered with Cyan and Black pixels and bound to
 *  NV2A_VERTEX_ATTR_DIFFUSE; Surface B is rendered with Black and Magenta pixels and bound to
 *  NV2A_VERTEX_ATTR_SPECULAR. A register combiner adds Diffuse and Specular colors.
 *  Expect to see two wide quads on a dark background: left Cyan (Cyan + Black) and right Magenta (Black + Magenta).
 *
 * @tc DynamicUpdateLoop
 *  Sequentially renders a single color into a surface and immediately draws the corresponding quad in four successive
 *  passes within a single frame to verify surface update synchronization between draw passes.
 *  Expect to see four quads on a dark background: top-left Red, top-right Green, bottom-left Blue, bottom-right Yellow.
 *
 * @tc RenderScalePattern
 *  Renders carrier triangle vertex indices and barycentric weights across swizzled surfaces A and B.
 *  The surfaces are bound as Diffuse and Specular vertex streams with a vertex shader that blends three carrier
 *  positions via address-register palette lookups (arl a0.x / c[A0+112]) to reconstruct a 3D mesh.
 *  Expect to see a clean, contiguous 3D mesh at 1x render scale.
 *  When emulator render scale > 1.0 downsamples/filters the surface with interpolation errors across diagonal seams,
 *  shifts in index values cause address-register lookups to select incorrect palette slots, visibly tearing the mesh
 *  into spikes (reproducing the behavior in the XDK DisplacementMap sample and xemu PR #2984).
 */
SurfaceAsVertexArrayTests::SurfaceAsVertexArrayTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Surface as vertex array", config) {
  tests_[kLinearDiffuseTest] = [this]() { TestLinearDiffuseArray(); };
  tests_[kSwizzledDiffuseTest] = [this]() { TestSwizzledDiffuseArray(); };
  tests_[kMultiStreamTest] = [this]() { TestMultiStream(); };
  tests_[kDynamicUpdateLoopTest] = [this]() { TestDynamicUpdateLoop(); };
  tests_[kRenderScalePatternTest] = [this]() { TestRenderScalePattern(); };
}

void SurfaceAsVertexArrayTests::AllocateTestSurfaces(bool need_surface_b) {
  FreeTestSurfaces();

  // Deterministically shift allocations by a variable number of 16 KB pages (cycling through 1..32)
  // based on a static run counter that persists across Initialize()/Deinitialize() cycles.
  // By allocating a spacer before allocating the test surfaces and holding it until the test finishes,
  // the kernel allocator is prevented from returning recently freed addresses, ensuring that xemu's
  // internal vertex attribute tracking and render-target scaling cache cannot mask issues on re-runs.
  static uint32_t s_run_count = 0;
  uint32_t shift = (s_run_count++ % 32) + 1;
  uint32_t spacer_size = shift * kSurfaceSize;

  allocation_spacer = static_cast<uint8_t *>(
      MmAllocateContiguousMemoryEx(spacer_size, 0, MAXRAM, 0x1000, PAGE_WRITECOMBINE | PAGE_READWRITE));
  ASSERT(allocation_spacer && "Failed to allocate spacer memory");

  surface_a_ = static_cast<uint8_t *>(
      MmAllocateContiguousMemoryEx(kSurfaceSize, 0, MAXRAM, 0x1000, PAGE_WRITECOMBINE | PAGE_READWRITE));
  ASSERT(surface_a_ && "Failed to allocate surface_a_");

  if (need_surface_b) {
    surface_b_ = static_cast<uint8_t *>(
        MmAllocateContiguousMemoryEx(kSurfaceSize, 0, MAXRAM, 0x1000, PAGE_WRITECOMBINE | PAGE_READWRITE));
    ASSERT(surface_b_ && "Failed to allocate surface_b_");
  }
}

void SurfaceAsVertexArrayTests::FreeTestSurfaces() {
  if (allocation_spacer) {
    MmFreeContiguousMemory(allocation_spacer);
    allocation_spacer = nullptr;
  }
  if (surface_a_) {
    MmFreeContiguousMemory(surface_a_);
    surface_a_ = nullptr;
  }
  if (surface_b_) {
    MmFreeContiguousMemory(surface_b_);
    surface_b_ = nullptr;
  }
}

void SurfaceAsVertexArrayTests::Initialize() {
  TestSuite::Initialize();

  vertex_buffer_ = host_.AllocateVertexBuffer(24);
  auto v = vertex_buffer_->Lock();

  (v++)->SetPosition(60.f, 45.f, 0.1f, 1.f);
  (v++)->SetPosition(260.f, 45.f, 0.1f, 1.f);
  (v++)->SetPosition(260.f, 205.f, 0.1f, 1.f);
  (v++)->SetPosition(60.f, 205.f, 0.1f, 1.f);

  (v++)->SetPosition(380.f, 45.f, 0.1f, 1.f);
  (v++)->SetPosition(580.f, 45.f, 0.1f, 1.f);
  (v++)->SetPosition(580.f, 205.f, 0.1f, 1.f);
  (v++)->SetPosition(380.f, 205.f, 0.1f, 1.f);

  (v++)->SetPosition(60.f, 265.f, 0.1f, 1.f);
  (v++)->SetPosition(260.f, 265.f, 0.1f, 1.f);
  (v++)->SetPosition(260.f, 425.f, 0.1f, 1.f);
  (v++)->SetPosition(60.f, 425.f, 0.1f, 1.f);

  (v++)->SetPosition(380.f, 265.f, 0.1f, 1.f);
  (v++)->SetPosition(580.f, 265.f, 0.1f, 1.f);
  (v++)->SetPosition(580.f, 425.f, 0.1f, 1.f);
  (v++)->SetPosition(380.f, 425.f, 0.1f, 1.f);

  (v++)->SetPosition(60.f, 80.f, 0.1f, 1.f);
  (v++)->SetPosition(280.f, 80.f, 0.1f, 1.f);
  (v++)->SetPosition(280.f, 400.f, 0.1f, 1.f);
  (v++)->SetPosition(60.f, 400.f, 0.1f, 1.f);

  (v++)->SetPosition(360.f, 80.f, 0.1f, 1.f);
  (v++)->SetPosition(580.f, 80.f, 0.1f, 1.f);
  (v++)->SetPosition(580.f, 400.f, 0.1f, 1.f);
  (v++)->SetPosition(360.f, 400.f, 0.1f, 1.f);

  vertex_buffer_->Unlock();

  for (uint32_t i = 0; i < 16; ++i) {
    ClearVertexAttribute(i);
  }

  host_.SetBlend(false);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_CULL_FACE_ENABLE, false);
  Pushbuffer::Push(NV097_SET_ALPHA_TEST_ENABLE, false);
  Pushbuffer::End();
}

void SurfaceAsVertexArrayTests::Deinitialize() {
  TestSuite::Deinitialize();

  vertex_buffer_.reset();
  FreeTestSurfaces();
}

void SurfaceAsVertexArrayTests::DrawQuads(const void *diffuse_surface) {
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
  host_.PrepareDraw(0xFF202020);

  auto v = vertex_buffer_->Lock();
  SetVertexAttribute(NV2A_VERTEX_ATTR_POSITION, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, sizeof(Vertex), v[0].pos);
  vertex_buffer_->Unlock();

  SetVertexAttribute(NV2A_VERTEX_ATTR_DIFFUSE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_D3D, 4, 4, diffuse_surface);

  DrawArrays(TestHost::PRIMITIVE_QUADS, 16);

  ClearVertexAttribute(NV2A_VERTEX_ATTR_POSITION);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_DIFFUSE);
}

void SurfaceAsVertexArrayTests::TestLinearDiffuseArray() {
  AllocateTestSurfaces(false);
  memset(surface_a_, 0, kSurfaceSize);

  host_.SetVertexShaderProgram(nullptr);
  host_.RenderToSurfaceStart(surface_a_, TestHost::SCF_A8R8G8B8, pb_depth_stencil_buffer(), TestHost::SZF_Z24S8,
                             kSurfaceWidth, kSurfaceHeight, false);

  host_.SetDiffuse(1.f, 0.f, 0.f, 1.f);
  host_.DrawScreenQuad(0.f, 0.f, 4.f, 1.f, 0.5f);

  host_.SetDiffuse(0.f, 1.f, 0.f, 1.f);
  host_.DrawScreenQuad(4.f, 0.f, 8.f, 1.f, 0.5f);

  host_.SetDiffuse(0.f, 0.f, 1.f, 1.f);
  host_.DrawScreenQuad(8.f, 0.f, 12.f, 1.f, 0.5f);

  host_.SetDiffuse(1.f, 1.f, 0.f, 1.f);
  host_.DrawScreenQuad(12.f, 0.f, 16.f, 1.f, 0.5f);

  host_.RenderToSurfaceEnd();

  DrawQuads(surface_a_);

  pb_print("%s\n", kLinearDiffuseTest);
  pb_draw_text_screen();

  FinishDraw(kLinearDiffuseTest);
}

void SurfaceAsVertexArrayTests::TestSwizzledDiffuseArray() {
  AllocateTestSurfaces(false);
  memset(surface_a_, 0, kSurfaceSize);

  host_.SetVertexShaderProgram(nullptr);
  host_.RenderToSurfaceStart(surface_a_, TestHost::SCF_A8R8G8B8, pb_depth_stencil_buffer(), TestHost::SZF_Z24S8,
                             kSurfaceWidth, kSurfaceHeight, true);

  host_.SetDiffuse(1.f, 0.f, 0.f, 1.f);
  host_.DrawScreenQuad(0.f, 0.f, 2.f, 2.f, 0.5f);

  host_.SetDiffuse(0.f, 1.f, 0.f, 1.f);
  host_.DrawScreenQuad(2.f, 0.f, 4.f, 2.f, 0.5f);

  host_.SetDiffuse(0.f, 0.f, 1.f, 1.f);
  host_.DrawScreenQuad(0.f, 2.f, 2.f, 4.f, 0.5f);

  host_.SetDiffuse(1.f, 1.f, 0.f, 1.f);
  host_.DrawScreenQuad(2.f, 2.f, 4.f, 4.f, 0.5f);

  host_.RenderToSurfaceEnd();

  DrawQuads(surface_a_);

  pb_print("%s\n", kSwizzledDiffuseTest);
  pb_draw_text_screen();

  FinishDraw(kSwizzledDiffuseTest);
}

void SurfaceAsVertexArrayTests::TestMultiStream() {
  AllocateTestSurfaces(true);
  memset(surface_a_, 0, kSurfaceSize);
  memset(surface_b_, 0, kSurfaceSize);

  host_.SetVertexShaderProgram(nullptr);
  host_.RenderToSurfaceStart(surface_a_, TestHost::SCF_A8R8G8B8, pb_depth_stencil_buffer(), TestHost::SZF_Z24S8,
                             kSurfaceWidth, kSurfaceHeight, false);
  host_.SetDiffuse(0.f, 1.f, 1.f, 1.f);
  host_.DrawScreenQuad(0.f, 0.f, 4.f, 1.f, 0.5f);
  host_.SetDiffuse(0.f, 0.f, 0.f, 1.f);
  host_.DrawScreenQuad(4.f, 0.f, 8.f, 1.f, 0.5f);
  host_.RenderToSurfaceEnd();

  host_.RenderToSurfaceStart(surface_b_, TestHost::SCF_A8R8G8B8, pb_depth_stencil_buffer(), TestHost::SZF_Z24S8,
                             kSurfaceWidth, kSurfaceHeight, false);
  host_.SetDiffuse(0.f, 0.f, 0.f, 1.f);
  host_.DrawScreenQuad(0.f, 0.f, 4.f, 1.f, 0.5f);
  host_.SetDiffuse(1.f, 0.f, 1.f, 1.f);
  host_.DrawScreenQuad(4.f, 0.f, 8.f, 1.f, 0.5f);
  host_.RenderToSurfaceEnd();

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
  host_.PrepareDraw(0xFF202020);

  host_.SetCombinerControl(1);
  host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_DIFFUSE), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_SPECULAR), TestHost::OneInput());
  host_.SetOutputColorCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);
  host_.SetInputAlphaCombiner(0, TestHost::AlphaInput(TestHost::SRC_DIFFUSE), TestHost::OneInput(),
                              TestHost::AlphaInput(TestHost::SRC_SPECULAR), TestHost::OneInput());
  host_.SetOutputAlphaCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);
  host_.SetFinalCombiner0Just(TestHost::SRC_R0);
  host_.SetFinalCombiner1Just(TestHost::SRC_R0, true);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_SPECULAR_ENABLE, true);
  Pushbuffer::End();

  auto v = vertex_buffer_->Lock();
  SetVertexAttribute(NV2A_VERTEX_ATTR_POSITION, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, sizeof(Vertex),
                     v[16].pos);
  vertex_buffer_->Unlock();

  SetVertexAttribute(NV2A_VERTEX_ATTR_DIFFUSE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_D3D, 4, 4, surface_a_);
  SetVertexAttribute(NV2A_VERTEX_ATTR_SPECULAR, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_D3D, 4, 4, surface_b_);

  DrawArrays(TestHost::PRIMITIVE_QUADS, 8);

  ClearVertexAttribute(NV2A_VERTEX_ATTR_POSITION);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_DIFFUSE);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_SPECULAR);

  pb_print("%s\n", kMultiStreamTest);
  pb_draw_text_screen();

  FinishDraw(kMultiStreamTest);
}

void SurfaceAsVertexArrayTests::TestDynamicUpdateLoop() {
  AllocateTestSurfaces(false);
  memset(surface_a_, 0, kSurfaceSize);

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.PrepareDraw(0xFF202020);

  static constexpr struct {
    float r, g, b;
  } kPasses[4] = {
      {1.f, 0.f, 0.f},
      {0.f, 1.f, 0.f},
      {0.f, 0.f, 1.f},
      {1.f, 1.f, 0.f},
  };

  SetVertexAttribute(NV2A_VERTEX_ATTR_DIFFUSE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_D3D, 4, 4, surface_a_);

  auto v = vertex_buffer_->Lock();

  for (int pass = 0; pass < 4; ++pass) {
    host_.SetVertexShaderProgram(nullptr);
    host_.RenderToSurfaceStart(surface_a_, TestHost::SCF_A8R8G8B8, pb_depth_stencil_buffer(), TestHost::SZF_Z24S8,
                               kSurfaceWidth, kSurfaceHeight, false);
    host_.SetDiffuse(kPasses[pass].r, kPasses[pass].g, kPasses[pass].b, 1.f);
    host_.DrawScreenQuad(0.f, 0.f, 4.f, 1.f, 0.5f);
    host_.RenderToSurfaceEnd();

    host_.SetVertexShaderProgram(shader);
    SetVertexAttribute(NV2A_VERTEX_ATTR_POSITION, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, sizeof(Vertex),
                       v[pass * 4].pos);
    DrawArrays(TestHost::PRIMITIVE_QUADS, 4);
  }

  vertex_buffer_->Unlock();

  ClearVertexAttribute(NV2A_VERTEX_ATTR_POSITION);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_DIFFUSE);

  pb_print("%s\n", kDynamicUpdateLoopTest);
  pb_draw_text_screen();

  FinishDraw(kDynamicUpdateLoopTest);
}

void SurfaceAsVertexArrayTests::TestRenderScalePattern() {
  AllocateTestSurfaces(true);

  memset(surface_a_, 0, kSurfaceSize);
  memset(surface_b_, 0, kSurfaceSize);

  static constexpr auto kPaletteScale = static_cast<float>(DisplacementVertexShader::kPaletteSize);

  auto render_carrier_mesh = [this](auto emit_triangle) {
    host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
    for (uint32_t cell_row = 0; cell_row < 2; ++cell_row) {
      for (uint32_t cell_col = 0; cell_col < 2; ++cell_col) {
        auto x0 = static_cast<float>(cell_col * 4);
        auto x1 = static_cast<float>((cell_col + 1) * 4);
        auto y0 = static_cast<float>(cell_row * 4);
        auto y1 = static_cast<float>((cell_row + 1) * 4);

        uint32_t v00 = cell_row * 3 + cell_col;
        uint32_t v01 = cell_row * 3 + (cell_col + 1);
        uint32_t v11 = (cell_row + 1) * 3 + (cell_col + 1);
        uint32_t v10 = (cell_row + 1) * 3 + cell_col;

        emit_triangle(x0, y0, x1, y0, x1, y1, v00, v01, v11);
        emit_triangle(x0, y0, x1, y1, x0, y1, v00, v11, v10);
      }
    }
    host_.End();
  };

  // Render index texture into swizzled surface_a_.
  host_.SetVertexShaderProgram(nullptr);
  host_.RenderToSurfaceStart(surface_a_, TestHost::SCF_A8R8G8B8, pb_depth_stencil_buffer(), TestHost::SZF_Z24S8,
                             kSurfaceWidth, kSurfaceHeight, true);
  render_carrier_mesh(
      [this](float x0, float y0, float x1, float y1, float x2, float y2, uint32_t idx0, uint32_t idx1, uint32_t idx2) {
        auto r = (static_cast<float>(idx0) + 0.5f) / kPaletteScale;
        auto g = (static_cast<float>(idx1) + 0.5f) / kPaletteScale;
        auto b = (static_cast<float>(idx2) + 0.5f) / kPaletteScale;
        host_.SetDiffuse(r, g, b, 1.f);
        host_.SetScreenVertex(x0, y0, 0.5f);
        host_.SetScreenVertex(x1, y1, 0.5f);
        host_.SetScreenVertex(x2, y2, 0.5f);
      });
  host_.RenderToSurfaceEnd();

  // Render weight texture into swizzled surface_b_.
  host_.RenderToSurfaceStart(surface_b_, TestHost::SCF_A8R8G8B8, pb_depth_stencil_buffer(), TestHost::SZF_Z24S8,
                             kSurfaceWidth, kSurfaceHeight, true);
  render_carrier_mesh([this](float x0, float y0, float x1, float y1, float x2, float y2, uint32_t, uint32_t, uint32_t) {
    host_.SetDiffuse(1.f, 0.f, 0.f, 1.f);
    host_.SetScreenVertex(x0, y0, 0.5f);
    host_.SetDiffuse(0.f, 1.f, 0.f, 1.f);
    host_.SetScreenVertex(x1, y1, 0.5f);
    host_.SetDiffuse(0.f, 0.f, 1.f, 1.f);
    host_.SetScreenVertex(x2, y2, 0.5f);
  });
  host_.RenderToSurfaceEnd();

  auto shader = std::make_shared<DisplacementVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  shader->LookAt({0.f, 0.f, -4.f, 1.f}, {0.f, 0.f, 0.f, 1.f}, {0.f, 1.f, 0.f, 1.f});

  host_.SetVertexShaderProgram(shader);
  host_.PrepareDraw(0xFF202020);

  // Positions are synthesized entirely within the vertex shader from the diffuse and specular streams; no position
  // attribute stream is required.
  SetVertexAttribute(NV2A_VERTEX_ATTR_DIFFUSE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_D3D, 4, 4, surface_a_);
  SetVertexAttribute(NV2A_VERTEX_ATTR_SPECULAR, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_D3D, 4, 4, surface_b_);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LESS);
  Pushbuffer::Push(NV097_SET_CULL_FACE_ENABLE, false);

  Pushbuffer::Push(NV097_BREAK_VERTEX_BUFFER_CACHE, 0);
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_QUADS);

  auto swizzle = [](uint32_t u, uint32_t v) {
    uint32_t r = 0;
    for (uint32_t i = 0; i < 16; ++i) {
      r |= ((u >> i) & 1) << (2 * i);
      r |= ((v >> i) & 1) << (2 * i + 1);
    }
    return r;
  };

  for (uint32_t y = 0; y < 7; ++y) {
    for (uint32_t x = 0; x < 7; ++x) {
      uint32_t q0 = swizzle(x, y);
      uint32_t q1 = swizzle(x + 1, y);
      uint32_t q2 = swizzle(x + 1, y + 1);
      uint32_t q3 = swizzle(x, y + 1);

      Pushbuffer::Push(NV097_ARRAY_ELEMENT16, q0 | (q1 << 16));
      Pushbuffer::Push(NV097_ARRAY_ELEMENT16, q2 | (q3 << 16));
    }
  }

  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  Pushbuffer::End();

  ClearVertexAttribute(NV2A_VERTEX_ATTR_DIFFUSE);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_SPECULAR);

  pb_print("%s\n", kRenderScalePatternTest);
  pb_draw_text_screen();

  FinishDraw(kRenderScalePatternTest);
}
