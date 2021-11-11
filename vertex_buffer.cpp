#include "vertex_buffer.h"

#include <memory>
#include <xboxkrnl/xboxkrnl.h>

#define MAXRAM 0x03FFAFFF

VertexBuffer::VertexBuffer(uint32_t num_vertices) : num_vertices_(num_vertices) {
  uint32_t buffer_size = sizeof(Vertex) * num_vertices;
  normalized_vertex_buffer_ = static_cast<Vertex *>(
      MmAllocateContiguousMemoryEx(buffer_size, 0, MAXRAM, 0, PAGE_WRITECOMBINE | PAGE_READWRITE));
}

VertexBuffer::~VertexBuffer() {
  if (linear_vertex_buffer_) {
    MmFreeContiguousMemory(linear_vertex_buffer_);
  }
  if (normalized_vertex_buffer_) {
    MmFreeContiguousMemory(normalized_vertex_buffer_);
  }
}

Vertex *VertexBuffer::Lock() {
  return normalized_vertex_buffer_;
}

void VertexBuffer::Unlock() {
}

void VertexBuffer::Linearize(float texture_width, float texture_height) {
  uint32_t buffer_size = sizeof(Vertex) * num_vertices_;
  if (!linear_vertex_buffer_) {
    linear_vertex_buffer_ = static_cast<Vertex *>(
        MmAllocateContiguousMemoryEx(buffer_size, 0, MAXRAM, 0, PAGE_WRITECOMBINE | PAGE_READWRITE));
  }

  memcpy(linear_vertex_buffer_, normalized_vertex_buffer_, buffer_size);

  for (int i = 0; i < num_vertices_; i++) {
    linear_vertex_buffer_[i].texcoord[0] *= static_cast<float>(texture_width);
    linear_vertex_buffer_[i].texcoord[1] *= static_cast<float>(texture_height);
  }
}

void VertexBuffer::DefineQuad(uint32_t start_index, float left, float top, float right, float bottom) {
  DefineQuad(start_index, left, top, right, bottom, 0.0f, 0.0f, 0.0f, 0.0f);
}

void VertexBuffer::DefineQuad(uint32_t start_index, float left, float top, float right, float bottom, float z) {
  DefineQuad(start_index, left, top, right, bottom, z, z, z, z);
}

void VertexBuffer::DefineQuad(uint32_t start_index, float left, float top, float right, float bottom, float ul_z,
                              float ll_z, float lr_z, float ur_z) {

    assert(start_index <= (num_vertices_ - 6) && "Invalid start_index, need at least 6 vertices to define quad.");

    Vertex *vb = normalized_vertex_buffer_ + (start_index * 6);

    auto set = [vb](int index, float x, float y, float z, float u, float v) {
      vb[index].pos[0] = x;
      vb[index].pos[1] = y;
      vb[index].pos[2] = z;

      vb[index].texcoord[0] = u;
      vb[index].texcoord[1] = v;

      vb[index].normal[0] = 0.0f;
      vb[index].normal[1] = 0.0f;
      vb[index].normal[2] = 1.0f;

      vb[index].diffuse[0] = 1.0f;
      vb[index].diffuse[1] = 1.0f;
      vb[index].diffuse[2] = 1.0f;
      vb[index].diffuse[3] = 1.0f;
    };

    set(0, left, top, ul_z, 0.0f, 0.0f);
    set(1, right, bottom, lr_z, 1.0f, 1.0f);
    set(2, right, top, ur_z, 1.0f, 0.0f);

    set(3, left, top, ul_z, 0.0f, 0.0f);
    set(4, left, bottom, ll_z, 0.0f, 1.0f);
    set(5, right, bottom, lr_z, 1.0f, 1.0f);
}
