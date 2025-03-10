#include "vertex_buffer.h"

#include <pbkit/pbkit.h>
#include <xboxkrnl/xboxkrnl.h>

#include <memory>

#include "debug_output.h"
#include "pbkit_ext.h"

void Vertex::Translate(float x, float y, float z, float w) {
  pos[0] += x;
  pos[1] += y;
  pos[2] += z;
  pos[3] += w;
}

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
  cache_valid_ = false;
  return normalized_vertex_buffer_;
}

void VertexBuffer::Unlock() {}

void VertexBuffer::Linearize(float texture_width, float texture_height) {
  uint32_t buffer_size = sizeof(Vertex) * num_vertices_;
  if (!linear_vertex_buffer_) {
    linear_vertex_buffer_ = static_cast<Vertex *>(
        MmAllocateContiguousMemoryEx(buffer_size, 0, MAXRAM, 0, PAGE_WRITECOMBINE | PAGE_READWRITE));
  }

  memcpy(linear_vertex_buffer_, normalized_vertex_buffer_, buffer_size);

  for (int i = 0; i < num_vertices_; i++) {
    linear_vertex_buffer_[i].texcoord0[0] *= static_cast<float>(texture_width);
    linear_vertex_buffer_[i].texcoord0[1] *= static_cast<float>(texture_height);
  }
}

void VertexBuffer::DefineTriangleCCW(uint32_t start_index, const float *one, const float *two, const float *three) {
  Color diffuse = {1.0, 1.0, 1.0, 1.0};
  DefineTriangleCCW(start_index, one, two, three, diffuse, diffuse, diffuse);
}

void VertexBuffer::DefineTriangleCCW(uint32_t start_index, const float *one, const float *two, const float *three,
                                     const float *normal_one, const float *normal_two, const float *normal_three) {
  Color diffuse = {1.0, 1.0, 1.0, 1.0};
  DefineTriangleCCW(start_index, one, two, three, normal_one, normal_two, normal_three, diffuse, diffuse, diffuse);
}

void VertexBuffer::DefineTriangleCCW(uint32_t start_index, const float *one, const float *two, const float *three,
                                     const Color &one_diffuse, const Color &two_diffuse, const Color &three_diffuse) {
  float normal[3] = {0.0f};
  DefineTriangleCCW(start_index, one, two, three, normal, normal, normal, one_diffuse, two_diffuse, three_diffuse);
}

void VertexBuffer::DefineTriangleCCW(uint32_t start_index, const float *one, const float *two, const float *three,
                                     const float *normal_one, const float *normal_two, const float *normal_three,
                                     const Color &diffuse_one, const Color &diffuse_two, const Color &diffuse_three) {
  DefineTriangle(start_index, one, two, three, normal_one, normal_two, normal_three, diffuse_one, diffuse_two,
                 diffuse_three);
  Vertex *vb = normalized_vertex_buffer_ + (start_index * 3);
  Vertex temp = vb[2];
  vb[2] = vb[1];
  vb[1] = temp;
}

void VertexBuffer::DefineTriangle(uint32_t start_index, const float *one, const float *two, const float *three) {
  Color diffuse = {1.0, 1.0, 1.0, 1.0};
  DefineTriangle(start_index, one, two, three, diffuse, diffuse, diffuse);
}

void VertexBuffer::DefineTriangle(uint32_t start_index, const float *one, const float *two, const float *three,
                                  const float *normal_one, const float *normal_two, const float *normal_three) {
  Color diffuse = {1.0, 1.0, 1.0, 1.0};
  DefineTriangle(start_index, one, two, three, normal_one, normal_two, normal_three, diffuse, diffuse, diffuse);
}

void VertexBuffer::DefineTriangle(uint32_t start_index, const float *one, const float *two, const float *three,
                                  const Color &diffuse_one, const Color &diffuse_two, const Color &diffuse_three) {
  float normal[3] = {0.0f};
  DefineTriangle(start_index, one, two, three, normal, normal, normal, diffuse_one, diffuse_two, diffuse_three);
}

void VertexBuffer::DefineTriangle(uint32_t start_index, const float *one, const float *two, const float *three,
                                  const float *normal_one, const float *normal_two, const float *normal_three,
                                  const Color &diffuse_one, const Color &diffuse_two, const Color &diffuse_three) {
  ASSERT(start_index <= (num_vertices_ - 3) && "Invalid start_index, need at least 3 vertices to define triangle.");

  cache_valid_ = false;

  Vertex *vb = normalized_vertex_buffer_ + (start_index * 3);

  auto set = [vb](int index, const float *pos, const float *normal, const Color &diffuse) {
    vb[index].pos[0] = pos[0];
    vb[index].pos[1] = pos[1];
    vb[index].pos[2] = pos[2];

    vb[index].normal[0] = normal[0];
    vb[index].normal[1] = normal[1];
    vb[index].normal[2] = normal[2];

    vb[index].diffuse[0] = diffuse.r;
    vb[index].diffuse[1] = diffuse.g;
    vb[index].diffuse[2] = diffuse.b;
    vb[index].diffuse[3] = diffuse.a;
  };

  set(0, one, normal_one, diffuse_one);
  set(1, two, normal_two, diffuse_two);
  set(2, three, normal_three, diffuse_three);
}

void VertexBuffer::DefineBiTriCCW(uint32_t start_index, float left, float top, float right, float bottom) {
  DefineBiTriCCW(start_index, left, top, right, bottom, 0.0f, 0.0f, 0.0f, 0.0f);
}

void VertexBuffer::DefineBiTriCCW(uint32_t start_index, float left, float top, float right, float bottom, float z) {
  DefineBiTriCCW(start_index, left, top, right, bottom, z, z, z, z);
}

void VertexBuffer::DefineBiTriCCW(uint32_t start_index, float left, float top, float right, float bottom, float ul_z,
                                  float ll_z, float lr_z, float ur_z) {
  Color diffuse = {1.0, 1.0, 1.0, 1.0};
  DefineBiTriCCW(start_index, left, top, right, bottom, ul_z, ll_z, lr_z, ur_z, diffuse, diffuse, diffuse, diffuse);
}

void VertexBuffer::DefineBiTriCCW(uint32_t start_index, float left, float top, float right, float bottom, float ul_z,
                                  float ll_z, float lr_z, float ur_z, const Color &ul_diffuse, const Color &ll_diffuse,
                                  const Color &lr_diffuse, const Color &ur_diffuse) {
  Color specular = {1.0, 1.0, 1.0, 1.0};
  DefineBiTriCCW(start_index, left, top, right, bottom, ul_z, ll_z, lr_z, ur_z, ul_diffuse, ll_diffuse, lr_diffuse,
                 ur_diffuse, specular, specular, specular, specular);
}

void VertexBuffer::DefineBiTriCCW(uint32_t start_index, float left, float top, float right, float bottom, float ul_z,
                                  float ll_z, float lr_z, float ur_z, const Color &ul_diffuse, const Color &ll_diffuse,
                                  const Color &lr_diffuse, const Color &ur_diffuse, const Color &ul_specular,
                                  const Color &ll_specular, const Color &lr_specular, const Color &ur_specular) {
  ASSERT(start_index <= (num_vertices_ - 6) && "Invalid start_index, need at least 6 vertices to define quad.");

  cache_valid_ = false;

  Vertex *vb = normalized_vertex_buffer_ + (start_index * 6);

  auto set = [vb](int index, float x, float y, float z, float u, float v, const Color &diffuse, const Color &specular) {
    vb[index].pos[0] = x;
    vb[index].pos[1] = y;
    vb[index].pos[2] = z;
    vb[index].pos[3] = 1.0f;

    vb[index].texcoord0[0] = u;
    vb[index].texcoord0[1] = v;
    vb[index].texcoord0[2] = 0.0f;
    vb[index].texcoord0[3] = 0.0f;

    vb[index].texcoord1[0] = u;
    vb[index].texcoord1[1] = v;
    vb[index].texcoord1[2] = 0.0f;
    vb[index].texcoord1[3] = 0.0f;

    vb[index].texcoord2[0] = u;
    vb[index].texcoord2[1] = v;
    vb[index].texcoord2[2] = 0.0f;
    vb[index].texcoord2[3] = 0.0f;

    vb[index].texcoord3[0] = u;
    vb[index].texcoord3[1] = v;
    vb[index].texcoord3[2] = 0.0f;
    vb[index].texcoord3[3] = 0.0f;

    vb[index].normal[0] = 0.0f;
    vb[index].normal[1] = 0.0f;
    vb[index].normal[2] = 1.0f;

    vb[index].diffuse[0] = diffuse.r;
    vb[index].diffuse[1] = diffuse.g;
    vb[index].diffuse[2] = diffuse.b;
    vb[index].diffuse[3] = diffuse.a;

    vb[index].specular[0] = specular.r;
    vb[index].specular[1] = specular.g;
    vb[index].specular[2] = specular.b;
    vb[index].specular[3] = specular.a;
  };

  set(0, left, top, ul_z, 0.0f, 0.0f, ul_diffuse, ul_specular);
  set(1, right, bottom, lr_z, 1.0f, 1.0f, lr_diffuse, lr_specular);
  set(2, right, top, ur_z, 1.0f, 0.0f, ur_diffuse, ur_specular);

  set(3, left, top, ul_z, 0.0f, 0.0f, ul_diffuse, ul_specular);
  set(4, left, bottom, ll_z, 0.0f, 1.0f, ll_diffuse, ll_specular);
  set(5, right, bottom, lr_z, 1.0f, 1.0f, lr_diffuse, lr_specular);
}

void VertexBuffer::DefineBiTri(uint32_t start_index, float left, float top, float right, float bottom) {
  DefineBiTri(start_index, left, top, right, bottom, 0.0f, 0.0f, 0.0f, 0.0f);
}

void VertexBuffer::DefineBiTri(uint32_t start_index, float left, float top, float right, float bottom, float z) {
  DefineBiTri(start_index, left, top, right, bottom, z, z, z, z);
}

void VertexBuffer::DefineBiTri(uint32_t start_index, float left, float top, float right, float bottom, float ul_z,
                               float ll_z, float lr_z, float ur_z) {
  Color diffuse = {1.0, 1.0, 1.0, 1.0};
  DefineBiTri(start_index, left, top, right, bottom, ul_z, ll_z, lr_z, ur_z, diffuse, diffuse, diffuse, diffuse);
}

void VertexBuffer::DefineBiTri(uint32_t start_index, float left, float top, float right, float bottom, float ul_z,
                               float ll_z, float lr_z, float ur_z, const Color &ul_diffuse, const Color &ll_diffuse,
                               const Color &lr_diffuse, const Color &ur_diffuse) {
  Color specular = {1.0, 1.0, 1.0, 1.0};
  DefineBiTri(start_index, left, top, right, bottom, ul_z, ll_z, lr_z, ur_z, ul_diffuse, ll_diffuse, lr_diffuse,
              ur_diffuse, specular, specular, specular, specular);
}

void VertexBuffer::DefineBiTri(uint32_t start_index, float left, float top, float right, float bottom, float ul_z,
                               float ll_z, float lr_z, float ur_z, const Color &ul_diffuse, const Color &ll_diffuse,
                               const Color &lr_diffuse, const Color &ur_diffuse, const Color &ul_specular,
                               const Color &ll_specular, const Color &lr_specular, const Color &ur_specular) {
  DefineBiTriCCW(start_index, left, top, right, bottom, ul_z, ll_z, lr_z, ur_z, ul_diffuse, ll_diffuse, lr_diffuse,
                 ur_diffuse, ul_specular, ll_specular, lr_specular, ur_specular);
  Vertex *vb = normalized_vertex_buffer_ + (start_index * 6);
  Vertex temp = vb[2];
  vb[2] = vb[1];
  vb[1] = temp;

  temp = vb[5];
  vb[5] = vb[4];
  vb[4] = temp;
}

void VertexBuffer::DefineBiTri(uint32_t start_index, const vector_t ul, const vector_t ll, const vector_t lr,
                               const vector_t ur, const Color &ul_diffuse, const Color &ll_diffuse,
                               const Color &lr_diffuse, const Color &ur_diffuse, const Color &ul_specular,
                               const Color &ll_specular, const Color &lr_specular, const Color &ur_specular) {
  ASSERT(start_index <= (num_vertices_ - 6) && "Invalid start_index, need at least 6 vertices to define quad.");

  cache_valid_ = false;

  Vertex *vb = normalized_vertex_buffer_ + (start_index * 6);

  auto set = [vb](int index, float x, float y, float z, float u, float v, const Color &diffuse, const Color &specular) {
    vb[index].pos[0] = x;
    vb[index].pos[1] = y;
    vb[index].pos[2] = z;
    vb[index].pos[3] = 1.0f;

    vb[index].texcoord0[0] = u;
    vb[index].texcoord0[1] = v;
    vb[index].texcoord0[2] = 0.0f;
    vb[index].texcoord0[3] = 0.0f;

    vb[index].texcoord1[0] = u;
    vb[index].texcoord1[1] = v;
    vb[index].texcoord1[2] = 0.0f;
    vb[index].texcoord1[3] = 0.0f;

    vb[index].texcoord2[0] = u;
    vb[index].texcoord2[1] = v;
    vb[index].texcoord2[2] = 0.0f;
    vb[index].texcoord2[3] = 0.0f;

    vb[index].texcoord3[0] = u;
    vb[index].texcoord3[1] = v;
    vb[index].texcoord3[2] = 0.0f;
    vb[index].texcoord3[3] = 0.0f;

    vb[index].normal[0] = 0.0f;
    vb[index].normal[1] = 0.0f;
    vb[index].normal[2] = 1.0f;

    vb[index].diffuse[0] = diffuse.r;
    vb[index].diffuse[1] = diffuse.g;
    vb[index].diffuse[2] = diffuse.b;
    vb[index].diffuse[3] = diffuse.a;

    vb[index].specular[0] = specular.r;
    vb[index].specular[1] = specular.g;
    vb[index].specular[2] = specular.b;
    vb[index].specular[3] = specular.a;
  };

  set(0, ul[0], ul[1], ul[2], 0.0f, 0.0f, ul_diffuse, ul_specular);
  set(1, lr[0], lr[1], lr[2], 1.0f, 1.0f, lr_diffuse, lr_specular);
  set(2, ur[0], ur[1], ur[2], 1.0f, 0.0f, ur_diffuse, ur_specular);

  set(3, ul[0], ul[1], ul[2], 0.0f, 0.0f, ul_diffuse, ul_specular);
  set(4, ll[0], ll[1], ll[2], 0.0f, 1.0f, ll_diffuse, ll_specular);
  set(5, lr[0], lr[1], lr[2], 1.0f, 1.0f, lr_diffuse, lr_specular);
}

void VertexBuffer::SetDiffuse(uint32_t vertex_index, const Color &color) {
  cache_valid_ = false;
  ASSERT(vertex_index < num_vertices_ && "Invalid vertex_index.");
  normalized_vertex_buffer_[vertex_index].diffuse[0] = color.r;
  normalized_vertex_buffer_[vertex_index].diffuse[1] = color.g;
  normalized_vertex_buffer_[vertex_index].diffuse[2] = color.b;
  normalized_vertex_buffer_[vertex_index].diffuse[3] = color.a;
}

void VertexBuffer::SetSpecular(uint32_t vertex_index, const Color &color) {
  cache_valid_ = false;
  ASSERT(vertex_index < num_vertices_ && "Invalid vertex_index.");
  normalized_vertex_buffer_[vertex_index].specular[0] = color.r;
  normalized_vertex_buffer_[vertex_index].specular[1] = color.g;
  normalized_vertex_buffer_[vertex_index].specular[2] = color.b;
  normalized_vertex_buffer_[vertex_index].specular[3] = color.a;
}

std::shared_ptr<VertexBuffer> VertexBuffer::ConvertFromTriangleStripToTriangles() const {
  auto num_triangles = num_vertices_ - 2;
  auto ret = std::make_shared<VertexBuffer>(num_triangles * 3);

  auto src = normalized_vertex_buffer_;
  auto dst = ret->normalized_vertex_buffer_;

  memcpy(dst, src, sizeof(*dst) * 3);
  dst += 3;
  src += 2;

  auto copy_triangle = [&src, &dst](bool is_odd) {
    if (is_odd) {
      memcpy(dst++, src, sizeof(*dst));
      memcpy(dst++, src - 1, sizeof(*dst));
      ++src;
      memcpy(dst++, src, sizeof(*dst));
    } else {
      memcpy(dst++, src - 1, sizeof(*dst));
      memcpy(dst++, src, sizeof(*dst));
      ++src;
      memcpy(dst++, src, sizeof(*dst));
    }
  };

  for (auto i = 1; i < num_triangles; ++i) {
    copy_triangle(i & 0x01);
  }

  return ret;
}

void VertexBuffer::Translate(float x, float y, float z, float w) {
  auto vertex = Lock();
  for (auto i = 0; i < num_vertices_; ++i, ++vertex) {
    vertex->Translate(x, y, z, w);
  }
  Unlock();
}
