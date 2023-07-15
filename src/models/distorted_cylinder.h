#ifndef NXDK_PGRAPH_TESTS_SRC_MODELS_DISTORTED_CYLINDER_H_
#define NXDK_PGRAPH_TESTS_SRC_MODELS_DISTORTED_CYLINDER_H_

#include "model_builder.h"

class DistortedCylinder : public SolidColorModelBuilder {
 public:
  DistortedCylinder() : SolidColorModelBuilder() {}
  DistortedCylinder(const vector_t &diffuse, const vector_t &specular) : SolidColorModelBuilder(diffuse, specular) {}

  [[nodiscard]] uint32_t GetVertexCount() const override;

 protected:
  [[nodiscard]] const float *GetVertexPositions() const override;
  [[nodiscard]] const float *GetVertexNormals() const override;
};

#endif  // NXDK_PGRAPH_TESTS_SRC_MODELS_DISTORTED_CYLINDER_H_
