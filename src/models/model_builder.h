#ifndef NXDK_PGRAPH_TESTS_SRC_MODELS_MODEL_BUILDER_H_
#define NXDK_PGRAPH_TESTS_SRC_MODELS_MODEL_BUILDER_H_

#include <cstdint>
#include <vector>

#include "vertex_buffer.h"
#include "xbox_math_types.h"

//! A factory for model meshes.
class ModelBuilder {
 public:
  virtual ~ModelBuilder() = default;

  //! Returns the number of kPositions required to hold the model.
  [[nodiscard]] virtual uint32_t GetVertexCount() const = 0;

  //! Populates the given VertexBuffer with model data.
  virtual void PopulateVertexBuffer(const std::shared_ptr<VertexBuffer> &vertices);

  virtual void PopulateVertexBuffer(const std::shared_ptr<VertexBuffer> &vertices, const float *transformation);

 protected:
  [[nodiscard]] virtual const float *GetVertexPositions() = 0;
  [[nodiscard]] virtual const float *GetVertexNormals() = 0;

  // Called after vertex arrays have been consumed.
  virtual void ReleaseData() {};
};

//! Builder for untextured models.
class SolidColorModelBuilder : public ModelBuilder {
 public:
  SolidColorModelBuilder();
  SolidColorModelBuilder(const vector_t &diffuse, const vector_t &specular);

  //! Populates the given VertexBuffer with model data.
  void PopulateVertexBuffer(const std::shared_ptr<VertexBuffer> &vertices) override;

  //! Populates the given VertexBuffer with transformed model data.
  virtual void PopulateVertexBuffer(const std::shared_ptr<VertexBuffer> &vertices,
                                    const float *transformation) override;

 private:
  void ApplyColors(const std::shared_ptr<VertexBuffer> &vertices) const;

 protected:
  vector_t diffuse_;
  vector_t specular_;
};

#endif  // NXDK_PGRAPH_TESTS_SRC_MODELS_MODEL_BUILDER_H_
