#include "texture_render_target_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "texture_format.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// From pbkit.c, DMA_COLOR is set to channel 9 by default
// NV097_SET_CONTEXT_DMA_COLOR == NV20_TCL_PRIMITIVE_3D_SET_OBJECT3
const uint32_t kDefaultDMAColorChannel = 9;

static const uint32_t kTextureWidth = 256;
static const uint32_t kTexturePitch = kTextureWidth * 4;
static const uint32_t kTextureHeight = 256;

static int GenerateGradientSurface(SDL_Surface **gradient_surface, int width, int height);
static int GeneratePalettizedGradientSurface(uint8_t **gradient_surface, int width, int height,
                                             TestHost::PaletteSize size);
static uint32_t *GeneratePalette(TestHost::PaletteSize size);

static constexpr TestHost::PaletteSize kPaletteSizes[] = {
    TestHost::PALETTE_256,
    TestHost::PALETTE_128,
    TestHost::PALETTE_64,
    TestHost::PALETTE_32,
};

static bool RequiresSpecialTest(const TextureFormatInfo &format) {
  switch (format.xbox_format) {
    case NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8:
    case NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FIXED:
    case NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FLOAT:
    case NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_X8_Y24_FIXED:
    case NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT23_A8R8G8B8:
    case NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT45_A8R8G8B8:
      return true;

    default:
      return false;
  }
}

TextureRenderTargetTests::TextureRenderTargetTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texture render target") {
  for (auto i = 0; i < kNumFormats; ++i) {
    auto &format = kTextureFormats[i];
    std::string name = MakeTestName(format);

    if (!RequiresSpecialTest(format)) {
      tests_[name] = [this, format]() { Test(format); };
    }
  }

  for (auto size : kPaletteSizes) {
    std::string name = MakePalettizedTestName(size);
    tests_[name] = [this, size]() { TestPalettized(size); };
  }
}

void TextureRenderTargetTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  auto channel = kNextContextChannel;
  pb_create_dma_ctx(channel++, DMA_CLASS_3D, 0, MAXRAM, &texture_target_ctx_);
  pb_bind_channel(&texture_target_ctx_);

  const uint32_t texture_size = kTexturePitch * kTextureHeight;
  render_target_ =
      (uint8_t *)MmAllocateContiguousMemoryEx(texture_size, 0, MAXRAM, 0x1000, PAGE_WRITECOMBINE | PAGE_READWRITE);
  ASSERT(render_target_ && "Failed to allocate target surface.");
  pb_set_dma_address(&texture_target_ctx_, render_target_, texture_size - 1);

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
}

void TextureRenderTargetTests::Deinitialize() {
  TestSuite::Deinitialize();
  if (render_target_) {
    MmFreeContiguousMemory(render_target_);
  }
}

void TextureRenderTargetTests::CreateGeometry() {
  const auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  const auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  render_target_vertex_buffer_ = host_.AllocateVertexBuffer(6);
  render_target_vertex_buffer_->DefineBiTri(0, 0.0f, 0.0f, kTextureWidth, kTextureHeight, 0.1f);
  render_target_vertex_buffer_->Linearize(kTextureWidth, kTextureHeight);

  framebuffer_vertex_buffer_ = host_.AllocateVertexBuffer(6);
  framebuffer_vertex_buffer_->DefineBiTri(0, -1.75, 1.75, 1.75, -1.75, 0.1f);
  framebuffer_vertex_buffer_->Linearize(fb_width, fb_height);
}

void TextureRenderTargetTests::ResetAndDrawFromRenderTarget() const {
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  const auto render_target_address = reinterpret_cast<uint32_t>(render_target_) & 0x03FFFFFF;
  const auto normal_texture_address = reinterpret_cast<uint32_t>(host_.GetTextureMemory()) & 0x03FFFFFF;

  // Reset the color buffer to the framebuffer and set the texture source to the render target.
  {
    host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                           host_.GetFramebufferHeight());
    host_.CommitSurfaceFormat();

    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);

    host_.SetVertexShaderProgram(nullptr);
    host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

    host_.SetWindowClip(host_.GetFramebufferWidth() - 1, host_.GetFramebufferHeight() - 1);
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));

    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
    pb_end(p);

    host_.PrepareDraw(0xFE212021);

    // PrepareDraw overwrites the texture offset.
    p = pb_begin();
    p = pb_push1(p, NV097_SET_TEXTURE_OFFSET, render_target_address);
    pb_end(p);

    host_.SetVertexBuffer(framebuffer_vertex_buffer_);
    host_.DrawArrays();
  }

  // Restore the texture source.
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_TEXTURE_OFFSET, normal_texture_address);
    pb_end(p);
  }
}

void TextureRenderTargetTests::Test(const TextureFormatInfo &texture_format) {
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

  host_.SetTextureFormat(texture_format);
  std::string test_name = MakeTestName(texture_format);

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetTextureDimensions(host_.GetMaxTextureWidth(), host_.GetMaxTextureHeight());

  SDL_Surface *gradient_surface;
  int update_texture_result =
      GenerateGradientSurface(&gradient_surface, (int)host_.GetMaxTextureWidth(), (int)host_.GetMaxTextureHeight());
  ASSERT(!update_texture_result && "Failed to generate SDL surface");

  update_texture_result = host_.SetTexture(gradient_surface);
  SDL_FreeSurface(gradient_surface);
  ASSERT(!update_texture_result && "Failed to set texture");

  host_.PrepareDraw(0xFE202020);

  // Redirect the color output to the target texture.
  {
    host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kTextureWidth, kTextureHeight, true);
    host_.CommitSurfaceFormat();

    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kTexturePitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, texture_target_ctx_.ChannelID);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);

    // TODO: Investigate if this is actually necessary. Morrowind does this after changing offsets.
    p = pb_push1(p, NV097_NO_OPERATION, 0);
    p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
    pb_end(p);

    auto shader = std::make_shared<PrecalculatedVertexShader>();
    host_.SetVertexShaderProgram(shader);

    host_.SetWindowClip(kTextureWidth - 1, kTextureHeight - 1);
    host_.SetVertexBuffer(render_target_vertex_buffer_);
    host_.DrawArrays();
  }

  ResetAndDrawFromRenderTarget();

  pb_print("N: %s\n", texture_format.name);
  pb_print("F: 0x%x\n", texture_format.xbox_format);
  pb_print("SZ: %d\n", texture_format.xbox_swizzled);
  pb_print("C: %d\n", texture_format.require_conversion);
  pb_print("W: %d\n", host_.GetMaxTextureWidth());
  pb_print("H: %d\n", host_.GetMaxTextureHeight());
  pb_print("P: %d\n", texture_format.xbox_bpp * host_.GetMaxTextureWidth() / 8);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, test_name);

  host_.SetWindowClip(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.SetViewportOffset(0.531250f, 0.531250f, 0, 0);
  host_.SetViewportScale(0, -0, 0, 0);
}

void TextureRenderTargetTests::TestPalettized(TestHost::PaletteSize size) {
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  const auto render_target_address = reinterpret_cast<uint32_t>(render_target_) & 0x03FFFFFF;
  const auto normal_texture_address = reinterpret_cast<uint32_t>(host_.GetTextureMemory()) & 0x03FFFFFF;

  auto &texture_format = GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8);
  host_.SetTextureFormat(texture_format);
  std::string test_name = MakePalettizedTestName(size);

  uint8_t *gradient_surface = nullptr;
  int err = GeneratePalettizedGradientSurface(&gradient_surface, (int)host_.GetMaxTextureWidth(),
                                              (int)host_.GetMaxTextureHeight(), size);
  ASSERT(!err && "Failed to generate palettized surface");

  err = host_.SetRawTexture(gradient_surface, host_.GetMaxTextureWidth(), host_.GetMaxTextureHeight(), 1,
                            host_.GetMaxTextureWidth(), 1, texture_format.xbox_swizzled);
  delete[] gradient_surface;
  ASSERT(!err && "Failed to set texture");

  auto palette = GeneratePalette(size);
  err = host_.SetPalette(palette, size);
  delete[] palette;
  ASSERT(!err && "Failed to set palette");

  host_.PrepareDraw(0xFE222020);

  // Redirect the color output to the target texture.
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kTexturePitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, texture_target_ctx_.ChannelID);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
    // TODO: Investigate if this is actually necessary. Morrowind does this after changing offsets.
    p = pb_push1(p, NV097_NO_OPERATION, 0);
    p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
    pb_end(p);

    auto shader = std::make_shared<PrecalculatedVertexShader>();
    host_.SetVertexShaderProgram(shader);

    host_.SetVertexBuffer(render_target_vertex_buffer_);
    host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kTextureWidth, kTextureHeight, true);
    host_.CommitSurfaceFormat();

    host_.DrawArrays();
  }

  ResetAndDrawFromRenderTarget();

  pb_print("N: %s\n", texture_format.name);
  pb_print("Ps: %d\n", size);
  pb_print("F: 0x%x\n", texture_format.xbox_format);
  pb_print("SZ: %d\n", texture_format.xbox_swizzled);
  pb_print("C: %d\n", texture_format.require_conversion);
  pb_print("W: %d\n", host_.GetMaxTextureWidth());
  pb_print("H: %d\n", host_.GetMaxTextureHeight());
  pb_print("P: %d\n", texture_format.xbox_bpp * host_.GetMaxTextureWidth() / 8);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}

std::string TextureRenderTargetTests::MakeTestName(const TextureFormatInfo &texture_format) {
  std::string test_name = "TexFmt_";
  test_name += texture_format.name;
  if (texture_format.xbox_linear) {
    test_name += "_L";
  }
  return std::move(test_name);
}

std::string TextureRenderTargetTests::MakePalettizedTestName(TestHost::PaletteSize size) {
  std::string test_name = "TexFmt_";
  auto &fmt = GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8);
  test_name += fmt.name;

  char buf[32] = {0};
  snprintf(buf, 31, "_p%d", size);
  test_name += buf;

  return std::move(test_name);
}

static int GenerateGradientSurface(SDL_Surface **gradient_surface, int width, int height) {
  *gradient_surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
  if (!(*gradient_surface)) {
    return 1;
  }

  if (SDL_LockSurface(*gradient_surface)) {
    SDL_FreeSurface(*gradient_surface);
    *gradient_surface = nullptr;
    return 2;
  }

  auto pixels = static_cast<uint32_t *>((*gradient_surface)->pixels);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x, ++pixels) {
      int x_normal = static_cast<int>(static_cast<float>(x) * 255.0f / static_cast<float>(width));
      int y_normal = static_cast<int>(static_cast<float>(y) * 255.0f / static_cast<float>(height));
      *pixels = SDL_MapRGBA((*gradient_surface)->format, y_normal, x_normal, 255 - y_normal, x_normal + y_normal);
    }
  }

  SDL_UnlockSurface(*gradient_surface);

  return 0;
}

static int GeneratePalettizedGradientSurface(uint8_t **gradient_surface, int width, int height,
                                             TestHost::PaletteSize palette_size) {
  *gradient_surface = new uint8_t[width * height];
  if (!(*gradient_surface)) {
    return 1;
  }

  auto pixel = *gradient_surface;

  uint32_t total_size = width * height;
  uint32_t half_size = total_size >> 1;

  for (uint32_t i = 0; i < half_size; ++i, ++pixel) {
    *pixel = i & (palette_size - 1);
  }

  for (uint32_t i = half_size; i < total_size; i += 4) {
    uint8_t value = i & (palette_size - 1);
    *pixel++ = value;
    *pixel++ = value;
    *pixel++ = value;
    *pixel++ = value;
  }

  return 0;
}

static uint32_t *GeneratePalette(TestHost::PaletteSize size) {
  auto ret = new uint32_t[size];

  uint32_t block_size = size / 4;
  auto component_inc = (uint32_t)ceilf(255.0f / (float)block_size);
  uint32_t i = 0;
  uint32_t component = 0;
  for (; i < block_size; ++i, component += component_inc) {
    uint32_t color_value = 0xFF - component;
    ret[i + block_size * 0] = 0xFF000000 + color_value;
    ret[i + block_size * 1] = 0xFF000000 + (color_value << 8);
    ret[i + block_size * 2] = 0xFF000000 + (color_value << 16);
    ret[i + block_size * 3] = 0xFF000000 + color_value + (color_value << 8) + (color_value << 16);
  }

  return ret;
}
