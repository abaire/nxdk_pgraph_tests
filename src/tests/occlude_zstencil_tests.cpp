#include "occlude_zstencil_tests.h"

#include <memory>

#include "models/flat_mesh_grid_model.h"
#include "shaders/precalculated_vertex_shader.h"

static std::string MakeTestName(uint32_t occlude_zstencil_value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "ZS_%08X");
  return buf;
}

/**
 */
OccludeZStencilTests::OccludeZStencilTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Occlude zstencil", config) {
  // Values beyond 3 raise an exception.
  for (auto test_value : {0, 1, 2, 3}) {
    std::string test_name = MakeTestName(test_value);
    tests_[test_name] = [this, test_value, test_name]() { this->Test(test_name, test_value); };
  }
}

void OccludeZStencilTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_SPECULAR), 0);
    p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_BACK_DIFFUSE), 0);
    p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_BACK_SPECULAR), 0);

    p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
    p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);

    p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
    p = pb_push3f(p, NV097_SET_SCENE_AMBIENT_COLOR, 0.01f, 0.01f, 0.01f);
    p = pb_push3(p, NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);
    p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 0.70f);

    // Values taken from MechAssault
    p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x00, 0xBF56C33A);  // -0.838916
    p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x04, 0xC038C729);  // -2.887156
    p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x08, 0x4043165A);  // 3.048239
    p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x0c, 0xBF34DCE5);  // -0.706496
    p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x10, 0xC020743F);  // -2.507095
    p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x14, 0x40333D06);  // 2.800600

    pb_end(p);
  }

  {
    vector_t position{0.f, -0.25f, -8.f, 1.f};
    vector_t direction{0.f, 0.1f, 1.f, 1.f};
    spotlight_ = std::make_shared<Spotlight>(0, position, direction, 10.f, 30.f, 10.f, 0.2f, 0.f, 0.f,
                                             Spotlight::FalloffFactor::ONE);
    spotlight_->SetDiffuse(0.25f, 0.f, 0.f);
    spotlight_->SetSpecular(0.f, 0.01f, 0.f);
  }
  {
    vector_t position{1.f, 1.f, -1.5f, 1.f};
    point_light_ = std::make_shared<PointLight>(1, position, 4.f, 0.4f, 0.1f, 0.2f);
    point_light_->SetDiffuse(0.f, 0.f, 0.75f);
    point_light_->SetSpecular(0.f, 0.2f, 0.f);
  }

  CreateGeometry();
}

void OccludeZStencilTests::Deinitialize() {
  vertex_buffer_plane_.reset();
  spotlight_.reset();
  point_light_.reset();
  TestSuite::Deinitialize();
}

void OccludeZStencilTests::CreateGeometry() {
  // SET_COLOR_MATERIAL causes per-vertex diffuse color to be ignored entirely.
  vector_t diffuse{1.f, 0.f, 1.f, 1.f};

  // However:
  // 1) the alpha from the specular value is added to the material alpha.
  // 2) the color is added to the computed vertex color if SEPARATE_SPECULAR is OFF
  vector_t specular{0.1f, 0.1, 0.1f, 0.25f};

  auto construct_model = [this](ModelBuilder& model, std::shared_ptr<VertexBuffer>& vertex_buffer) {
    vertex_buffer = host_.AllocateVertexBuffer(model.GetVertexCount());
    model.PopulateVertexBuffer(vertex_buffer);
  };

  {
    auto model = FlatMeshGridModel(diffuse, specular);
    construct_model(model, vertex_buffer_plane_);
  }
}

void OccludeZStencilTests::SetupLights() {
  spotlight_->Commit(host_);
  point_light_->Commit(host_);

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
    p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, false);
    p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, spotlight_->light_enable_mask() + point_light_->light_enable_mask());
    p = pb_push1(p, NV097_SET_LIGHT_CONTROL, NV097_SET_LIGHT_CONTROL_V_SEPARATE_SPECULAR);
    pb_end(p);
  }

  host_.SetCombinerControl(1);
  host_.SetInputColorCombiner(0, TestHost::SRC_DIFFUSE, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT, TestHost::SRC_SPECULAR, false,
                              TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false,
                              TestHost::MAP_UNSIGNED_INVERT);
  host_.SetInputAlphaCombiner(0, TestHost::SRC_DIFFUSE, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT, TestHost::SRC_SPECULAR, true,
                              TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false,
                              TestHost::MAP_UNSIGNED_INVERT);

  host_.SetOutputColorCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);
  host_.SetOutputAlphaCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);

  host_.SetFinalCombiner0Just(TestHost::SRC_R0);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_R0, true,
                          false, /*specular_add_invert_r0*/ false, /* specular_add_invert_v1*/ false,
                          /* specular_clamp */ true);
}

void OccludeZStencilTests::Test(const std::string& name, uint32_t zstencil_value) {
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());
  host_.PrepareDraw(0xFF222322);

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_OCCLUDE_ZSTENCIL_EN, zstencil_value);
    p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, false);
    p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, true);
    pb_end(p);
  }

  {
    host_.SetupControl0(true);

    constexpr auto kLeft = -3.f;
    constexpr auto kTop = 1.f;
    constexpr auto kRight = 0.f;
    constexpr auto kBottom = -kTop;
    constexpr auto kOccluderZ = -1.f;

    {
      auto p = pb_begin();
      p = pb_push1(p, NV097_SET_DEPTH_MASK, true);
      p = pb_push1(p, NV097_SET_STENCIL_MASK, true);
      pb_end(p);
    }

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetSpecular(0.f, 1.f, 0.f, 0.1f);
    host_.SetDiffuse(0.f, 0.5f, 0.f, 0.1f);

    host_.SetVertex(kLeft, kTop, kOccluderZ, 1.0f);
    host_.SetVertex(kRight, kTop, kOccluderZ, 1.0f);
    host_.SetVertex(kRight, kBottom, kOccluderZ, 1.0f);
    host_.SetVertex(kLeft, kBottom, kOccluderZ, 1.0f);
    host_.End();

    {
      auto p = pb_begin();
      p = pb_push1(p, NV097_SET_DEPTH_MASK, false);
      p = pb_push1(p, NV097_SET_STENCIL_MASK, false);
      pb_end(p);
    }

    host_.SetupControl0(false);
  }

  SetupLights();

  host_.SetVertexBuffer(vertex_buffer_plane_);
  host_.DrawArrays(host_.POSITION | host_.NORMAL | host_.DIFFUSE | host_.SPECULAR);

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, false);
    p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
    pb_end(p);
  }

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
