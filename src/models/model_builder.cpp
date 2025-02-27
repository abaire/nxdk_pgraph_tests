#include "model_builder.h"

#include "xbox_math_matrix.h"

using namespace XboxMath;

void ModelBuilder::PopulateVertexBuffer(const std::shared_ptr<VertexBuffer>& vertices) {
  auto vertex = vertices->Lock();
  auto position = GetVertexPositions();
  auto normal = GetVertexNormals();
  auto vertex_count = GetVertexCount();
  for (auto i = 0; i < vertex_count; ++i, ++vertex) {
    vector_t pos = {*position++, *position++, *position++, 1.0f};
    vertex->SetPosition(pos);
    vertex->SetNormal(normal);
    normal += 3;
    vertex->SetDiffuseGrey(1.f);
    vertex->SetSpecularGrey(1.f);
  }
  ReleaseData();
  vertices->Unlock();
}

void ModelBuilder::PopulateVertexBuffer(const std::shared_ptr<VertexBuffer>& vertices, const float* transformation) {
  matrix4_t trans_mat;
  memcpy(trans_mat, transformation, sizeof(trans_mat));
  auto position = GetVertexPositions();
  auto normal = GetVertexNormals();
  auto vertex_count = GetVertexCount();
  auto vertex = vertices->Lock();
  for (auto i = 0; i < vertex_count; ++i, ++vertex) {
    vector_t pos = {*position++, *position++, *position++, 1.0f};
    VectorMultMatrix(pos, trans_mat);
    vertex->SetPosition(pos);
    vertex->SetNormal(normal);
    normal += 3;
    vertex->SetDiffuseGrey(1.f);
    vertex->SetSpecularGrey(1.f);
  }
  ReleaseData();
  vertices->Unlock();
}

SolidColorModelBuilder::SolidColorModelBuilder()
    : diffuse_{1.0f, 1.0f, 1.0f, 1.0f}, specular_{0.0f, 0.0f, 0.0f, 1.0f} {}

SolidColorModelBuilder::SolidColorModelBuilder(const XboxMath::vector_t& diffuse, const XboxMath::vector_t& specular)
    : ModelBuilder() {
  memcpy(diffuse_, diffuse, sizeof(diffuse_));
  memcpy(specular_, specular, sizeof(specular_));
}

//! Populates the given VertexBuffer with model data.
void SolidColorModelBuilder::PopulateVertexBuffer(const std::shared_ptr<VertexBuffer>& vertices) {
  ModelBuilder::PopulateVertexBuffer(vertices);
  ApplyColors(vertices);
}

//! Populates the given VertexBuffer with transformed model data.
void SolidColorModelBuilder::PopulateVertexBuffer(const std::shared_ptr<VertexBuffer>& vertices,
                                                  const float* transformation) {
  ModelBuilder::PopulateVertexBuffer(vertices, reinterpret_cast<const float*>(transformation));
  ApplyColors(vertices);
}

void SolidColorModelBuilder::ApplyColors(const std::shared_ptr<VertexBuffer>& vertices) const {
  auto vertex_count = GetVertexCount();
  auto vertex = vertices->Lock();
  for (auto i = 0; i < vertex_count; ++i, ++vertex) {
    vertex->SetDiffuse(diffuse_);
    vertex->SetSpecular(specular_);
  }
  vertices->Unlock();
}
