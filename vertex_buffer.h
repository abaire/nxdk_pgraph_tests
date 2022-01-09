#ifndef NXDK_PGRAPH_TESTS__VERTEX_BUFFER_H_
#define NXDK_PGRAPH_TESTS__VERTEX_BUFFER_H_

#include <cstdint>
#include <vector>

#define TO_ARGB(float_vals)                                                                  \
  (((uint32_t)(float_vals[0] * 255.0f) << 24) + ((uint32_t)(float_vals[1] * 255.0f) << 16) + \
   ((uint32_t)(float_vals[2] * 255.0f) << 8) + ((uint32_t)(float_vals[3] * 255.0f)))

#pragma pack(1)
typedef struct Vertex {
  float pos[3];
  float texcoord[2];
  float normal[3];
  float diffuse[4];
  float specular[4];

  uint32_t GetDiffuseARGB() const { return TO_ARGB(diffuse); }

  uint32_t GetSpecularARGB() const { return TO_ARGB(specular); }
} Vertex;
#pragma pack()

struct Color {
  void SetRGB(float red, float green, float blue) {
    r = red;
    g = green;
    b = blue;
  }

  void SetRGBA(float red, float green, float blue, float alpha) {
    r = red;
    g = green;
    b = blue;
    a = alpha;
  }

  void SetGrey(float val) {
    r = val;
    g = val;
    b = val;
  }

  void SetGreyA(float val, float alpha) {
    r = val;
    g = val;
    b = val;
    a = alpha;
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

  void SetCacheValid(bool valid = true) { cache_valid_ = valid; }
  bool IsCacheValid() const { return cache_valid_; }

  void Linearize(float texture_width, float texture_height);

  // Defines a triangle with the give 3-element vertices.
  void DefineTriangle(uint32_t start_index, const float* one, const float* two, const float* three);
  void DefineTriangle(uint32_t start_index, const float* one, const float* two, const float* three,
                      const float* normal_one, const float* normal_two, const float* normal_three);
  void DefineTriangle(uint32_t start_index, const float* one, const float* two, const float* three,
                      const Color& one_diffuse, const Color& two_diffuse, const Color& three_diffuse);
  void DefineTriangle(uint32_t start_index, const float* one, const float* two, const float* three,
                      const float* normal_one, const float* normal_two, const float* normal_three,
                      const Color& diffuse_one, const Color& diffuse_two, const Color& diffuse_three);
  void DefineTriangleCW(uint32_t start_index, const float* one, const float* two, const float* three);
  void DefineTriangleCW(uint32_t start_index, const float* one, const float* two, const float* three,
                        const float* normal_one, const float* normal_two, const float* normal_three);
  void DefineTriangleCW(uint32_t start_index, const float* one, const float* two, const float* three,
                        const Color& diffuse_one, const Color& diffuse_two, const Color& diffuse_three);
  void DefineTriangleCW(uint32_t start_index, const float* one, const float* two, const float* three,
                        const float* normal_one, const float* normal_two, const float* normal_three,
                        const Color& diffuse_one, const Color& diffuse_two, const Color& diffuse_three);

  void DefineQuad(uint32_t start_index, float left, float top, float right, float bottom);
  void DefineQuad(uint32_t start_index, float left, float top, float right, float bottom, float z);
  void DefineQuad(uint32_t start_index, float left, float top, float right, float bottom, float ul_z, float ll_z,
                  float lr_z, float ur_z);
  void DefineQuad(uint32_t start_index, float left, float top, float right, float bottom, float ul_z, float ll_z,
                  float lr_z, float ur_z, const Color& ul_diffuse, const Color& ll_diffuse, const Color& lr_diffuse,
                  const Color& ur_diffuse);
  void DefineQuad(uint32_t start_index, float left, float top, float right, float bottom, float ul_z, float ll_z,
                  float lr_z, float ur_z, const Color& ul_diffuse, const Color& ll_diffuse, const Color& lr_diffuse,
                  const Color& ur_diffuse, const Color& ul_specular, const Color& ll_specular, const Color& lr_specular,
                  const Color& ur_specular);

  void DefineQuadCW(uint32_t start_index, float left, float top, float right, float bottom, float ul_z, float ll_z,
                    float lr_z, float ur_z, const Color& ul_diffuse, const Color& ll_diffuse, const Color& lr_diffuse,
                    const Color& ur_diffuse);
  void DefineQuadCW(uint32_t start_index, float left, float top, float right, float bottom, float ul_z, float ll_z,
                    float lr_z, float ur_z, const Color& ul_diffuse, const Color& ll_diffuse, const Color& lr_diffuse,
                    const Color& ur_diffuse, const Color& ul_specular, const Color& ll_specular,
                    const Color& lr_specular, const Color& ur_specular);

 private:
  friend class TestHost;

  uint32_t num_vertices_;
  Vertex* linear_vertex_buffer_ = nullptr;      // texcoords 0 to kFramebufferWidth/kFramebufferHeight
  Vertex* normalized_vertex_buffer_ = nullptr;  // texcoords normalized 0 to 1

  bool cache_valid_{false};  // Indicates whether the HW should be forced to reload this buffer.
};

#endif  // NXDK_PGRAPH_TESTS__VERTEX_BUFFER_H_
