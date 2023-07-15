#ifndef NXDK_PGRAPH_TESTS_SRC_MODELS_SINGLE_QUAD_H_
#define NXDK_PGRAPH_TESTS_SRC_MODELS_SINGLE_QUAD_H_

#include "model_builder.h"

class SingleQuad : public SolidColorModelBuilder {
 public:
  SingleQuad() : SolidColorModelBuilder() {}
  SingleQuad(const vector_t &diffuse, const vector_t &specular) : SolidColorModelBuilder(diffuse, specular) {}

  [[nodiscard]] uint32_t GetVertexCount() const override;

 protected:
  [[nodiscard]] const float *GetVertexPositions() const override;
  [[nodiscard]] const float *GetVertexNormals() const override;
};

#endif  // NXDK_PGRAPH_TESTS_SRC_MODELS_SINGLE_QUAD_H_
