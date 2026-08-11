#include "texture_cpu_update_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"
#include "texture_format.h"
#include "vertex_buffer.h"

static constexpr float kTextureSize = 256.0f;

static constexpr char kRGBATest[] = "RGBA";
static constexpr char kMultipleSwatchTest[] = "MultipleSwatches";
// static constexpr char kPalettizedTest[] = "PaletteCycle";

// PBKit initializes channel 8 as the DMA_SEMAPHORE context object.
// https://github.com/XboxDev/nxdk/blob/4171d5bfe5260c0dd2d42f4efeb9ec1d44788867/lib/pbkit/pbkit.c#L2827
const uint32_t kDefaultSemaphoreContextChannel = 8;

TextureCPUUpdateTests::TextureCPUUpdateTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texture CPU Update", config) {
  tests_[kRGBATest] = [this]() { TestRGBA(); };
  tests_[kMultipleSwatchTest] = [this]() { TestMultipleSwatches(); };
  //  tests_[kPalettizedTest] = [this]() { TestPalettized(); };
}

void TextureCPUUpdateTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  static constexpr auto semaphore_object_size = 32;
  semaphore_context_object_ =
      static_cast<uint32_t *>(MmAllocateContiguousMemoryEx(semaphore_object_size, 0, MAXRAM, 0, PAGE_READWRITE));
  memset(semaphore_context_object_, 0, semaphore_object_size);
  pb_create_dma_ctx(kNextContextChannel, DMA_CLASS_3D, (DWORD)semaphore_context_object_, 0x20, &semaphore_dma_ctx_);
  pb_bind_channel(&semaphore_dma_ctx_);
}

static void Draw(const TestHost &host_) {
  const float kLeft = 0;
  const float kRight = host_.GetFramebufferWidthF();
  const float kTop = 0;
  const float kBottom = host_.GetFramebufferHeightF();
  const float kZ = 1.0f;

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetTexCoord0(0.0f, 0.0f);
  host_.SetVertex(kLeft, kTop, kZ, 1.0f);
  host_.SetVertex(kRight, kTop, kZ, 1.0f);
  host_.SetVertex(kRight, kBottom, kZ, 1.0f);
  host_.SetVertex(kLeft, kBottom, kZ, 1.0f);
  host_.End();
}

void TextureCPUUpdateTests::TestRGBA() {
  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  auto &stage = host_.GetTextureStage(0);
  stage.SetTextureDimensions(kTextureSize, kTextureSize);
  host_.SetupTextureStages();

  pb_erase_text_screen();

  // Set the texture to pure red.
  auto texels = host_.GetTextureMemoryForStage(0);
  for (uint32_t y = 0; y < kTextureSize; ++y) {
    for (uint32_t x = 0; x < kTextureSize; ++x) {
      *texels++ = 0x66;
      *texels++ = 0x00;
      *texels++ = 0x00;
      *texels++ = 0xFF;
    }
  }

  host_.PrepareDraw(0xFE202020);
  Draw(host_);
  FinishDrawNoSave(kRGBATest);

  // Set the texture to pure green.
  texels = host_.GetTextureMemoryForStage(0);
  for (uint32_t y = 0; y < kTextureSize; ++y) {
    for (uint32_t x = 0; x < kTextureSize; ++x) {
      *texels++ = 0x00;
      *texels++ = 0x66;
      *texels++ = 0x00;
      *texels++ = 0xFF;
    }
  }
  Draw(host_);

  pb_erase_text_screen();
  pb_printat(0, 0, (char *)"%s", kRGBATest);
  pb_printat(7, 12, (char *)"Expect a green screen");
  pb_draw_text_screen();

  FinishDraw(kRGBATest);
}

// This does not actually change the texture on HW
// void TextureCPUUpdateTests::TestPalettized() {
//  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8));
//
//  // Set the texture to use palette index 1 for everything.
//  auto texels = host_.GetTextureMemoryForStage(0);
//  memset(texels, 0x01, kTextureSize * kTextureSize);
//
//  auto palette = host_.GetPaletteMemoryForStage(0);
//  host_.SetPaletteSize(TestHost::PALETTE_32, 0);
//  palette[1] = 0xFF770000;
//
//  auto &stage = host_.GetTextureStage(0);
//  stage.SetTextureDimensions(kTextureSize, kTextureSize);
//  host_.SetupTextureStages();
//
//  pb_erase_text_screen();
//
//  host_.PrepareDraw(0xFE212021);
//
//  Draw(host_);
//  FinishDrawNoSave(kPalettizedTest);
//
//  palette[1] = 0xFF007700;
//  Draw(host_);
//
//  pb_erase_text_screen();
//  pb_print("%s\n", kPalettizedTest);
//  pb_printat(7, 12, (char *)"Expect a green screen");
//  pb_draw_text_screen();
//
//  FinishDraw(kPalettizedTest);
//}

void TextureCPUUpdateTests::TestMultipleSwatches() {
  host_.PrepareDraw(0xFF505050);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_SEMAPHORE, semaphore_dma_ctx_.ChannelID);
  Pushbuffer::End();

  static constexpr uint32_t kTextureSize = 32;

  auto update_texture = [this](uint32_t color) {
    auto *texture_memory = reinterpret_cast<uint32_t *>(host_.GetTextureMemoryForStage(0));

    for (auto y = 0; y < kTextureSize; ++y) {
      for (auto x = 0; x < kTextureSize; ++x) {
        *texture_memory++ = color;
      }
    }

    DbgPrint("Setting texture memory to 0x%08X\n", *reinterpret_cast<uint32_t *>(host_.GetTextureMemoryForStage(0)));
  };

  auto semaphore_release_value = 0xABCD;
  auto set_fence_and_wait = [this, &semaphore_release_value]() {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_BACK_END_WRITE_SEMAPHORE_RELEASE, semaphore_release_value);
    Pushbuffer::Push(NV097_SET_COLOR_CLEAR_VALUE, 0);
    Pushbuffer::Push(NV097_SET_COLOR_CLEAR_VALUE, 0);
    Pushbuffer::End(true);

    // TODO: See if there is a better mechanism to delay until the report is fetched without spin locking.
    for (auto i = 0; i < 32 && *semaphore_context_object_ != semaphore_release_value; ++i) {
      Sleep(1);
    }

    semaphore_release_value += 2;
  };

  auto draw_quad_and_wait = [this, set_fence_and_wait](float left, float top) {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.f, 0.f);
    host_.SetVertex(left, top, 1.f);

    host_.SetTexCoord0(0.f, 1.f);
    host_.SetVertex(left + kTextureSize, top, 1.f);

    host_.SetTexCoord0(1.f, 0.f);
    host_.SetVertex(left + kTextureSize, top + kTextureSize, 1.f);

    host_.SetTexCoord0(1.f, 1.f);
    host_.SetVertex(left, top + kTextureSize, 1.f);
    host_.End();

    // Technically this does not seem to be necessary on hardware; reinitializing the texture stage either consistently
    // takes long enough to prevent mutation-before-use or performs an implicit fence.
    // On xemu, this is necessary and seems to be consistent with practical use seen in retail titles.
    set_fence_and_wait();
  };

  static constexpr float kSpacing = 18.f;
  static constexpr float kLeft = 32.f;
  static constexpr float kTop = 64.f;
  static constexpr float kTextColumn = 6;

  auto y = kTop;
  auto text_row = 2;
  static constexpr auto kTextRowInc = 2;

  static constexpr struct {
    uint32_t color;
    const char *name;
  } kTestCases[] = {
      {0xFFFF0000, "Blue"},    {0xFF00FF00, "Green"},  {0xFF0000FF, "Red"},
      {0xFFFF00FF, "Magenta"}, {0xFF00FFFF, "Yellow"}, {0xFFFFFF00, "Cyan"},
  };

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetEnabled();
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
  host_.SetupTextureStages();

  for (const auto &test_case : kTestCases) {
    update_texture(test_case.color);
    host_.SetupTextureStages();

    draw_quad_and_wait(kLeft, y);

    pb_printat(text_row, kTextColumn, "%s\n", test_case.name);
    y += kTextureSize + kSpacing;
    text_row += kTextRowInc;
  }

  pb_printat(0, 0, "%s\n", kMultipleSwatchTest);
  pb_draw_text_screen();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_SEMAPHORE, kDefaultSemaphoreContextChannel);
  Pushbuffer::End(true);

  FinishDraw(kMultipleSwatchTest);
}