#include "texture_depth_source_tests.h"

#include <pbkit/pbkit.h>

#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "test_host.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// NV097_SET_CONTEXT_DMA_ZETA is set to channel 10 by default.
const uint32_t kDefaultDMAZetaChannel = 10;

static constexpr char kD16TestName[] = "D16_Y16";

static constexpr float kLeft = -2.75f;
static constexpr float kRight = 2.75f;
static constexpr float kTop = 1.75f;
static constexpr float kBottom = -1.75f;

TextureDepthSourceTests::TextureDepthSourceTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texture depth source") {
  tests_[kD16TestName] = [this]() { TestD16(); };
}

void TextureDepthSourceTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  // TODO: Provide a mechanism to find the next unused channel.
  auto channel = kNextContextChannel;

  pb_create_dma_ctx(channel++, DMA_CLASS_3D, 0, MAXRAM, &texture_target_ctx_);
  pb_bind_channel(&texture_target_ctx_);

  host_.SetAlphaBlendEnabled(false);
}

void TextureDepthSourceTests::CreateGeometry() {
  constexpr int kNumTriangles = 4;
  auto buffer = host_.AllocateVertexBuffer(kNumTriangles * 3);

  int index = 0;
  {
    float one[] = {-1.5f, -1.5f, 0.0f};
    float two[] = {-2.5f, 0.6f, 0.0f};
    float three[] = {-0.5f, 0.6f, 0.0f};
    buffer->DefineTriangle(index++, one, two, three);
  }

  {
    float one[] = {0.0f, -1.5f, 5.0f};
    float two[] = {-1.0f, 0.75f, 10.0f};
    float three[] = {2.0f, 0.75f, 20.0f};
    buffer->DefineTriangle(index++, one, two, three);
  }

  {
    float one[] = {5.0f, -2.0f, 30};
    float two[] = {3.0f, 2.0f, 40};
    float three[] = {12.0f, 2.0f, 70};
    buffer->DefineTriangle(index++, one, two, three);
  }

  {
    float one[] = {20.0f, -10.0f, 50};
    float two[] = {12.0f, 10.0f, 125};
    float three[] = {80.0f, 10.0f, 200};
    buffer->DefineTriangle(index++, one, two, three);
  }
}

void TextureDepthSourceTests::TestD16() {
  host_.SetDepthBufferFormat(NV097_SET_SURFACE_FORMAT_ZETA_Z16);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_ZETA, texture_target_ctx_.ChannelID);
  p = pb_push1(p, NV097_SET_SURFACE_ZETA_OFFSET, reinterpret_cast<uint32_t>(host_.GetTextureMemory()) & 0x03FFFFFF);
  pb_end(p);

  host_.PrepareDraw(0xFE112233);

  // Render some geometry using texture memory as the zeta buffer.
  {
    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
    host_.SetupTextureStages();

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

    host_.DrawArrays();
  }

  p = pb_begin();
  p = pb_push1(p, NV097_SET_SURFACE_ZETA_OFFSET, 0);
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAZetaChannel);
  pb_end(p);

  //  host_.Clear(0xF0333333);

  //  // Render a quad with the zeta buffer as the texture.
  //  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  //  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  //
  //  auto& stage = host_.GetTextureStage(0);
  //  stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FIXED));
  ////  stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_Y16));
  ////  stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
  //  stage.SetTextureDimensions(1, 1, 1);
  //  stage.SetImageDimensions(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  //  host_.SetTextureStageEnabled(0, true);
  //  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  //  host_.SetupTextureStages();
  //
  //  {
  //    const float z = 1.5f;
  //    host_.Begin(TestHost::PRIMITIVE_QUADS);
  //    host_.SetDiffuse(0xFFFF00FF);
  //    host_.SetTexCoord0(0.0, 0.0);
  //    host_.SetVertex(kLeft, kTop, z, 1.0f);
  //
  //    host_.SetTexCoord0(host_.GetFramebufferWidthF(), 0.0);
  //    host_.SetVertex(kRight, kTop, z, 1.0f);
  //
  //    host_.SetTexCoord0(host_.GetFramebufferWidthF(), host_.GetFramebufferHeightF());
  //    host_.SetVertex(kRight, kBottom, z, 1.0f);
  //
  //    host_.SetTexCoord0(0.0, host_.GetFramebufferHeightF());
  //    host_.SetVertex(kLeft, kBottom, z, 1.0f);
  //    host_.End();
  //  }

  std::string z_name = "Z";
  z_name += kD16TestName;
  host_.FinishDraw(allow_saving_, output_dir_, kD16TestName, z_name);
}
