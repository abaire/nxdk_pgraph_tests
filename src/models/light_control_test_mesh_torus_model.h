// Generated by collada_converter.py
#ifndef _LIGHT_CONTROL_TEST_MESH_TORUS_MODEL_H_
#define _LIGHT_CONTROL_TEST_MESH_TORUS_MODEL_H_

#include <cstdint>

#include "model_builder.h"
#include "xbox_math_types.h"

class LightControlTestMeshTorusModel : public SolidColorModelBuilder {
 public:
  LightControlTestMeshTorusModel() : SolidColorModelBuilder() {}
  LightControlTestMeshTorusModel(const vector_t &diffuse, const vector_t &specular)
      : SolidColorModelBuilder(diffuse, specular) {}

  [[nodiscard]] uint32_t GetVertexCount() const override;

 protected:
  [[nodiscard]] const float *GetVertexPositions() override;
  [[nodiscard]] const float *GetVertexNormals() override;
  void ReleaseData() override {
    if (vertices_) {
      delete[] vertices_;
      vertices_ = nullptr;
    }
    if (normals_) {
      delete[] normals_;
      normals_ = nullptr;
    }
    if (texcoords_) {
      delete[] texcoords_;
      texcoords_ = nullptr;
    }
  }

  float *vertices_{nullptr};
  float *normals_{nullptr};
  float *texcoords_{nullptr};
};

#endif  // _LIGHT_CONTROL_TEST_MESH_TORUS_MODEL_H_