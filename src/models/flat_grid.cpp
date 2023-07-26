#include "flat_grid.h"

#include "flat_grid_mesh.h"

uint32_t FlatGrid::GetVertexCount() const { return kNumVertices; }
const float* FlatGrid::GetVertexPositions() const { return kPosition; }
const float* FlatGrid::GetVertexNormals() const { return kNormal; }
