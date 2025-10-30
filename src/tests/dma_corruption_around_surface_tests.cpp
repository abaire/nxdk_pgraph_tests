#include "dma_corruption_around_surface_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "file_util.h"
#include "pbkit_ext.h"
#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"
#include "texture_generator.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

#define NV2A_MMIO_BASE 0xFD000000
#define BLOCKPGRAPH_ADDR 0x400000
#define PGRAPH_ADDR(a) (NV2A_MMIO_BASE + BLOCKPGRAPH_ADDR + (a))

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
static constexpr uint32_t kDefaultDMAChannelA = 3;
// From pbkit.c, DMA_COLOR is set to channel 9 by default.
// NV097_SET_CONTEXT_DMA_COLOR == NV20_TCL_PRIMITIVE_3D_SET_OBJECT3
static constexpr uint32_t kDefaultDMAColorChannel = 9;

static constexpr char kDMAOverlapTestName[] = "DMAOverlap";
static constexpr char kReadFromFileTestName[] = "xemuReadFromFileIntoSurface";
static constexpr uint32_t kTextureSize = 128;

static constexpr uint32_t kCheckerSize = 8;
static constexpr uint32_t kCheckerboardB = 0xFF3333C0;

DMACorruptionAroundSurfaceTests::DMACorruptionAroundSurfaceTests(TestHost &host, std::string output_dir,
                                                                 const Config &config)
    : TestSuite(host, std::move(output_dir), "DMA corruption around surfaces", config) {
  tests_[kDMAOverlapTestName] = [this]() { Test(); };
  tests_[kReadFromFileTestName] = [this]() { TestReadFromFileIntoSurface(); };
}

void DMACorruptionAroundSurfaceTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  // Copy the test file to the z partition.
  if (!InstallCacheFile("D:\\dma_surface_tests\\opaque_white.raw", "Z:\\opaque_white.raw", FALSE)) {
    ASSERT(!"Failed to copy opaque_white.raw to the cache drive.")
  }
}

void DMACorruptionAroundSurfaceTests::Test() {
  host_.PrepareDraw(0xFF050505);

  const uint32_t kRenderBufferPitch = kTextureSize * 4;
  const auto kHalfTextureSize = kRenderBufferPitch * (kTextureSize >> 1);

  // Configure a surface pointing at texture memory and do a no-op draw to force xemu to create a SurfaceBinding.
  {
    const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(0));
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kRenderBufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kRenderBufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory + kHalfTextureSize));
    Pushbuffer::End();

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, kTextureSize, kTextureSize);
    NoOpDraw();
  }

  // Do a CPU copy to texture memory
  WaitForGPU();
  GenerateRGBACheckerboard(host_.GetTextureMemoryForStage(0), 0, 0, kTextureSize, kTextureSize, kRenderBufferPitch,
                           0xFFCC3333, kCheckerboardB, kCheckerSize);

  // Point the output surface back at the framebuffer.
  {
    const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::End();
  }

  // Restore the surface format to allow the texture to be rendered.
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());

  // Draw a quad with the texture.
  Draw();

  pb_print("%s\n", kDMAOverlapTestName);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kDMAOverlapTestName);
}

void DMACorruptionAroundSurfaceTests::TestReadFromFileIntoSurface() {
  host_.PrepareDraw(0xFF444444);

  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

  const uint32_t kStage = 0;
  const auto kTextureMemoryAddr = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(kStage));

  // Initialize the texture memory to dark grey
  memset(host_.GetTextureMemoryForStage(kStage), 0x22, host_.GetFramebufferWidth() * host_.GetFramebufferHeight() * 4);

  int info_row = 12;
  pb_printat(info_row++, 0, "Init 0x%X", reinterpret_cast<uint32_t *>(host_.GetTextureMemoryForStage(kStage))[0]);

  // Configure a surface pointing at texture memory and do a no-op draw to force xemu to create a SurfaceBinding.
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemoryAddr));
    Pushbuffer::End();

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                    host_.GetFramebufferHeight());

    NoOpDraw();

    // Note: Performing a read after the surface is created but before the DMA activity may make things work, depending
    // on whether the texture region had been accessed prior to the surface create elsewhere (e.g., by running this test
    // twice).
    //    pb_printat(info_row++, 0, "Init 0x%X", reinterpret_cast<uint32_t
    //    *>(host_.GetTextureMemoryForStage(kStage))[0]);

    host_.PBKitBusyWait();
  }

  // Kick off an async DMA transfer into the buffer
  {
    STRING path;
    static constexpr const char kTestFilename[] = "Z:\\opaque_white.raw";
    RtlInitAnsiString(&path, kTestFilename);

    HANDLE handle;
    OBJECT_ATTRIBUTES attributes;
    InitializeObjectAttributes(&attributes, &path, OBJ_CASE_INSENSITIVE, ObDosDevicesDirectory(), nullptr);

    IO_STATUS_BLOCK status_block = {{0}};
    NTSTATUS status =
        NtCreateFile(&handle,                                            // FileHandle
                     GENERIC_READ | FILE_READ_ATTRIBUTES | SYNCHRONIZE,  // DesiredAccess
                     &attributes,                                        // ObjectAttributes
                     &status_block,                                      // IoStatusBlock
                     0,                                                  // AllocationSize
                     FILE_ATTRIBUTE_NORMAL,                              // FileAttributes
                     FILE_SHARE_READ,                                    // ShareAccess
                     FILE_OPEN,                                          // CreateDisposition
                     FILE_NON_DIRECTORY_FILE | FILE_RANDOM_ACCESS | FILE_NO_INTERMEDIATE_BUFFERING);  // CreateOptions
    ASSERT(status == 0);

    LARGE_INTEGER byte_offset = {{0, 0}};
    status_block.Status = STATUS_PENDING;

    status = NtReadFile(handle,
                        nullptr,                                 // Event
                        nullptr,                                 // ApcRoutine
                        nullptr,                                 // ApcContext
                        &status_block,                           // IoStatusBlock
                        host_.GetTextureMemoryForStage(kStage),  // Buffer
                        kTextureSize * kTextureSize * 4,         // Length
                        &byte_offset);
    ASSERT(status == STATUS_PENDING);

    while (status_block.Status == STATUS_PENDING) {
      Sleep(1);
    }

    NtClose(handle);

    pb_printat(info_row++, 0, "Read 0x%X", reinterpret_cast<uint32_t *>(host_.GetTextureMemoryForStage(kStage))[0]);
  }

  // Point the output surface back at the framebuffer.
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::End();
  }

  // Restore the surface format to allow the texture to be rendered.
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());

  // Draw a quad with the texture.
  Draw(kStage);

  pb_printat(0, 0, "%s\n", kReadFromFileTestName);
  pb_print("Texture should be full white");
  pb_printat(info_row++, 0, "Surf flush 0x%X", reinterpret_cast<uint32_t *>(host_.GetTextureMemoryForStage(kStage))[0]);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kReadFromFileTestName);
}

void DMACorruptionAroundSurfaceTests::Draw(uint32_t stage) const {
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  switch (stage) {
    case 0:
      host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
      host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
      break;
    case 1:
      host_.SetFinalCombiner0Just(TestHost::SRC_TEX1);
      host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_2D_PROJECTIVE);
      break;
    default:
      ASSERT(!"Not implemented");
  }
  host_.SetTextureStageEnabled(stage, true);

  auto &texture_stage = host_.GetTextureStage(stage);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
  texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
  host_.SetupTextureStages();

  static constexpr float inset = 64.0f;
  const float left = inset;
  const float right = host_.GetFramebufferWidthF() - inset;
  const float top = inset;
  const float bottom = host_.GetFramebufferHeightF() - inset;
  const float mid_x = left + (right - left) * 0.5f;
  const float mid_y = top + (bottom - top) * 0.5f;

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetTexCoord0(0.f, 0.f);
  host_.SetTexCoord1(0.f, 0.f);
  host_.SetTexCoord2(0.f, 0.f);
  host_.SetTexCoord3(0.f, 0.f);
  host_.SetVertex(left, mid_y, 0.1f, 1.0f);

  host_.SetTexCoord0(kTextureSize, 0.f);
  host_.SetTexCoord1(kTextureSize, 0.f);
  host_.SetTexCoord2(kTextureSize, 0.f);
  host_.SetTexCoord3(kTextureSize, 0.f);
  host_.SetVertex(mid_x, top, 0.1f, 1.0f);

  host_.SetTexCoord0(kTextureSize, kTextureSize);
  host_.SetTexCoord1(kTextureSize, kTextureSize);
  host_.SetTexCoord2(kTextureSize, kTextureSize);
  host_.SetTexCoord3(kTextureSize, kTextureSize);
  host_.SetVertex(right, mid_y, 0.1f, 1.0f);

  host_.SetTexCoord0(0.f, kTextureSize);
  host_.SetTexCoord1(0.f, kTextureSize);
  host_.SetTexCoord2(0.f, kTextureSize);
  host_.SetTexCoord3(0.f, kTextureSize);
  host_.SetVertex(mid_x, bottom, 0.1f, 1.0f);
  host_.End();

  host_.SetTextureStageEnabled(stage, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
}

void DMACorruptionAroundSurfaceTests::NoOpDraw() const {
  host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
  host_.SetVertex(0.f, 0.f, 0.1f, 1.0f);
  host_.SetVertex(0.f, 0.f, 0.1f, 1.0f);
  host_.SetVertex(0.f, 0.f, 0.1f, 1.0f);
  host_.End();
}

void DMACorruptionAroundSurfaceTests::WaitForGPU() const {
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_NO_OPERATION, 0);
  Pushbuffer::Push(NV097_WAIT_FOR_IDLE, 0);
  Pushbuffer::End();
}
