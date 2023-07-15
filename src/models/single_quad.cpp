#include "single_quad.h"

#include "single_quad_mesh.h"

uint32_t SingleQuad::GetVertexCount() const { return kNumVertices; }
const float* SingleQuad::GetVertexPositions() const { return kPosition; }
const float* SingleQuad::GetVertexNormals() const { return kNormal; }
