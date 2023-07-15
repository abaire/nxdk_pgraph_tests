#include "icosphere.h"

#include "icosphere_mesh.h"

uint32_t Icosphere::GetVertexCount() const { return kNumVertices; }
const float* Icosphere::GetVertexPositions() const { return kPosition; }
const float* Icosphere::GetVertexNormals() const { return kNormal; }
