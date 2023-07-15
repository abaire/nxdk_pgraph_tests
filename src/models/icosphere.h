#ifndef NXDK_PGRAPH_TESTS_SRC_MODELS_ICOSPHERE_H_
#define NXDK_PGRAPH_TESTS_SRC_MODELS_ICOSPHERE_H_

#include "model_builder.h"

class Icosphere : public SolidColorModelBuilder {
 public:
  Icosphere() : SolidColorModelBuilder() {}
  Icosphere(const vector_t &diffuse, const vector_t &specular) : SolidColorModelBuilder(diffuse, specular) {}

  [[nodiscard]] uint32_t GetVertexCount() const override;

 protected:
  [[nodiscard]] const float *GetVertexPositions() const override;
  [[nodiscard]] const float *GetVertexNormals() const override;
};

#endif  // NXDK_PGRAPH_TESTS_SRC_MODELS_ICOSPHERE_H_
