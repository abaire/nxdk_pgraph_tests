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
    vector_t norm = {*normal++, *normal++, *normal++, 1.f};
    VectorMultMatrix(norm, trans_mat);
    vertex->SetNormal(norm);

    vertex->SetDiffuseGrey(1.f);
    vertex->SetSpecularGrey(1.f);
  }
  ReleaseData();
  vertices->Unlock();
}

SolidColorModelBuilder::SolidColorModelBuilder(const XboxMath::vector_t& diffuse, const XboxMath::vector_t& specular)
    : ModelBuilder() {
  memcpy(diffuse_, diffuse, sizeof(diffuse_));
  memcpy(specular_, specular, sizeof(specular_));
}

SolidColorModelBuilder::SolidColorModelBuilder(const vector_t& diffuse, const vector_t& specular,
                                               const vector_t& back_diffuse, const vector_t& back_specular)
    : ModelBuilder() {
  memcpy(diffuse_, diffuse, sizeof(diffuse_));
  memcpy(specular_, specular, sizeof(specular_));
  memcpy(back_diffuse_, back_diffuse, sizeof(back_diffuse_));
  memcpy(back_specular_, back_specular, sizeof(back_specular_));
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
    vertex->SetBackDiffuse(back_diffuse_);
    vertex->SetBackSpecular(back_specular_);
  }
  vertices->Unlock();
}
