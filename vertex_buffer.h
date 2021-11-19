#ifndef NXDK_PGRAPH_TESTS__VERTEX_BUFFER_H_
#define NXDK_PGRAPH_TESTS__VERTEX_BUFFER_H_

#include <cstdint>

#pragma pack(1)
typedef struct Vertex {
  float pos[3];
  float texcoord[2];
  float normal[3];
  float diffuse[4];
} Vertex;
#pragma pack()

struct Color {
  void SetRGB(float red, float green, float blue) {
    r = red;
    g = green;
    b = blue;
  }

  void SetGrey(float val) {
    r = val;
    g = val;
    b = val;
  }

  float r{0.0f};
  float g{0.0f};
  float b{0.0};
  float a{1.0};
};

class TestHost;

class VertexBuffer {
 public:
  explicit VertexBuffer(uint32_t num_vertices);
  ~VertexBuffer();

  Vertex* Lock();
  void Unlock();

  uint32_t GetNumVertices() const { return num_vertices_; }

  void Linearize(float texture_width, float texture_height);

  void DefineQuad(uint32_t start_index, float left, float top, float right, float bottom);
  void DefineQuad(uint32_t start_index, float left, float top, float right, float bottom, float z);
  void DefineQuad(uint32_t start_index, float left, float top, float right, float bottom, float ul_z, float ll_z,
                  float lr_z, float ur_z);
  void DefineQuad(uint32_t start_index, float left, float top, float right, float bottom, float ul_z, float ll_z,
                  float lr_z, float ur_z, const Color& ul_diffuse, const Color& ll_diffuse, const Color& lr_diffuse,
                  const Color& ur_diffuse);
  void DefineQuadCW(uint32_t start_index, float left, float top, float right, float bottom, float ul_z, float ll_z,
                    float lr_z, float ur_z, const Color& ul_diffuse, const Color& ll_diffuse, const Color& lr_diffuse,
                    const Color& ur_diffuse);

 private:
  friend class TestHost;

  uint32_t num_vertices_;
  Vertex* linear_vertex_buffer_ = nullptr;      // texcoords 0 to kFramebufferWidth/kFramebufferHeight
  Vertex* normalized_vertex_buffer_ = nullptr;  // texcoords normalized 0 to 1
};

#endif  // NXDK_PGRAPH_TESTS__VERTEX_BUFFER_H_
