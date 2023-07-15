#include "distorted_cylinder.h"

#include "distorted_cylinder_mesh.h"

uint32_t DistortedCylinder::GetVertexCount() const { return kNumVertices; }
const float* DistortedCylinder::GetVertexPositions() const { return kPosition; }
const float* DistortedCylinder::GetVertexNormals() const { return kNormal; }
