#include "clipping_precision_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "shaders/passthrough_vertex_shader.h"
#include "texture_generator.h"

static constexpr const char kTestClippingPrecision[] = "clip_prec_large_xy";

static std::string MakeTestName(bool perspective_corrected, bool flat, float rotate_angle, int vertex_cycle) {
  char rot[32] = {0};
  snprintf(rot, sizeof(rot), "_rot%03d", (int)rotate_angle);
  char vert[32] = {0};
  snprintf(vert, sizeof(vert), "_vert%d", vertex_cycle);

  return std::string(kTestClippingPrecision) + "_nopers" + (perspective_corrected ? "0" : "1") + "_flat" +
         (flat ? "1" : "0") + rot + vert;
}

ClippingPrecisionTests::ClippingPrecisionTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Clipping precision", config, true) {
  for (auto perspective_corrected : {false, true}) {
    for (auto flat : {false, true}) {
      for (auto rotate_angle : {0.0f, 90.0f, 180.0f, 270.0f}) {
        for (auto vertex_cycle : {0, 1, 2}) {
          tests_[MakeTestName(perspective_corrected, flat, rotate_angle, vertex_cycle)] =
              [this, perspective_corrected, flat, rotate_angle, vertex_cycle]() {
                this->TestClippingPrecision(perspective_corrected, flat, rotate_angle, vertex_cycle);
              };
        }
      }
    }
  }
}

void ClippingPrecisionTests::Initialize() {
  TestSuite::Initialize();
  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
}

void ClippingPrecisionTests::Deinitialize() { TestSuite::Deinitialize(); }

void ClippingPrecisionTests::TestClippingPrecisionFrame(float ofs, bool perspective_corrected, bool flat,
                                                        float rotate_angle, int vertex_cycle, bool done) {
  host_.PrepareDraw(0xFE251135);

  // Generate a distinct texture.
  constexpr uint32_t kTextureSize = 256;
  {
    memset(host_.GetTextureMemory(), 0, host_.GetTextureMemorySize());
    GenerateSwizzledRGBACheckerboard(host_.GetTextureMemory(), 0, 0, kTextureSize, kTextureSize, kTextureSize * 4);

    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
    texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
    texture_stage.SetUWrap(TextureStage::WRAP_REPEAT);
    texture_stage.SetVWrap(TextureStage::WRAP_REPEAT);
    host_.SetupTextureStages();

    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
  }

  {
    auto p = pb_begin();
    p = pb_push1f(p, NV097_SET_CLIP_MIN, 0.0f);
    p = pb_push1f(p, NV097_SET_CLIP_MAX, 16777215.0f);
    p = pb_push1(p, NV097_SET_CONTROL0, 1 | (perspective_corrected ? 0x100000 : 0));
    pb_end(p);
  }

  {
    float tu = (perspective_corrected && !flat) ? 1.0 : 10000.0;
    float w2 = (8.0 + ofs) / 4.0;

    float tri_vertices[] = {
        480.0,
        100.0,
        30000.0,
        tu,
        0.0,
        480.0,
        240.0,
        30000.0,
        tu,
        0.5,
        -15000000.0f / w2,
        -2500000.0f / w2,
        w2,
        0.0,
        0.0,
        480.0,
        240.0,
        30000.0,
        tu,
        0.0,
        480.0,
        380.0,
        30000.0,
        tu,
        0.5,
        -15000000.0f / w2,
        2500000.0f / w2,
        w2,
        0.0,
        0.0,
    };

    if (flat) {
      for (int i = 0; i < sizeof(tri_vertices) / sizeof(float); i += 5) {
        tri_vertices[i + 2] = 1.0f;
      }
    }

    if (rotate_angle != 0.0f) {
      float radians = rotate_angle * M_PI / 180.0;

      for (int i = 0; i < sizeof(tri_vertices) / sizeof(float); i += 5) {
        float x = tri_vertices[i] - 320.0f;
        float y = tri_vertices[i + 1] - 240.0f;
        tri_vertices[i] = x * cos(radians) - y * sin(radians) + 320.0f;
        tri_vertices[i + 1] = x * sin(radians) + y * cos(radians) + 240.0f;
      }
    }

    host_.Begin(TestHost::PRIMITIVE_TRIANGLES);

    for (int i = 0; i < sizeof(tri_vertices) / sizeof(float) / 5; i++) {
      int j = ((i / 3) * 3 + (i + vertex_cycle) % 3) * 5;
      host_.SetTexCoord0(tri_vertices[j + 3], tri_vertices[j + 4], 0.0, 1.0);
      host_.SetVertex(tri_vertices[j], tri_vertices[j + 1], 0.95 * 16777215.0, tri_vertices[j + 2]);
    }

    host_.End();
  }

  pb_printat(8, 0, (char *)(done ? "Done!" : "Textures should not twitch!"));
  pb_print("\nOfs: %.4f ", ofs);
  pb_print("Pers: %d  ", perspective_corrected);
  pb_print("Flat: %d\n", flat);
  pb_print("Rot: %5.1f  ", rotate_angle);
  pb_print("Vert: %d\n", vertex_cycle);
  pb_draw_text_screen();

  std::string test_name = MakeTestName(perspective_corrected, flat, rotate_angle, vertex_cycle);
  FinishDrawNoSave(test_name);

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
}

void ClippingPrecisionTests::TestClippingPrecision(bool perspective_corrected, bool flat, float rotate_angle,
                                                   int vertex_cycle) {
  float ofs = 0.0;
  int num_frames = 200;

  for (int i = 0; i < num_frames; i++) {
    TestClippingPrecisionFrame(ofs, perspective_corrected, flat, rotate_angle, vertex_cycle, i == num_frames - 1);
    ofs += 0.001;
  }
}
