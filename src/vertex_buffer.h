#ifndef NXDK_PGRAPH_TESTS__VERTEX_BUFFER_H_
#define NXDK_PGRAPH_TESTS__VERTEX_BUFFER_H_

#include <cstdint>
#include <vector>

#define TO_ARGB(float_vals)                                                                      \
  (((uint32_t)((float_vals)[3] * 255.0f) << 24) + ((uint32_t)((float_vals)[0] * 255.0f) << 16) + \
   ((uint32_t)((float_vals)[1] * 255.0f) << 8) + ((uint32_t)((float_vals)[2] * 255.0f)))

#pragma pack(1)
typedef struct Vertex {
  float pos[4];
  float texcoord[2];
  float normal[4];
  float diffuse[4];
  float specular[4];

  inline void SetPosition(const float* value) { memcpy(pos, value, sizeof(pos)); }

  inline void SetPosition(float x, float y, float z, float w = 1.0f) {
    pos[0] = x;
    pos[1] = y;
    pos[2] = z;
    pos[3] = w;
  }

  inline void SetNormal(const float* value) { memcpy(normal, value, sizeof(normal)); }

  inline void SetNormal(float x, float y, float z, float w = 1.0f) {
    normal[0] = x;
    normal[1] = y;
    normal[2] = z;
    normal[3] = w;
  }

  inline void SetDiffuse(const float* value) { memcpy(diffuse, value, sizeof(diffuse)); }

  inline void SetDiffuse(float r, float g, float b, float a = 1.0f) {
    diffuse[0] = r;
    diffuse[1] = g;
    diffuse[2] = b;
    diffuse[3] = a;
  }

  inline void SetSpecular(const float* value) { memcpy(specular, value, sizeof(specular)); }

  inline void SetSpecular(float r, float g, float b, float a = 1.0f) {
    specular[0] = r;
    specular[1] = g;
    specular[2] = b;
    specular[3] = a;
  }

  inline void SetTexCoord(const float* value) { memcpy(texcoord, value, sizeof(texcoord)); }

  inline void SetTexCoord(const float u, const float v) {
    texcoord[0] = u;
    texcoord[1] = v;
  }

  inline void SetDiffuseGrey(float val) { SetDiffuse(val, val, val); }

  void SetDiffuseGrey(float val, float alpha) { SetDiffuse(val, val, val, alpha); }

  inline void SetSpecularGrey(float val) { SetSpecular(val, val, val); }

  void SetSpecularGrey(float val, float alpha) { SetSpecular(val, val, val, alpha); }

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

  uint32_t AsARGB() const {
    float vals[4] = {r, g, b, a};
    return TO_ARGB(vals);
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
  void DefineTriangleCCW(uint32_t start_index, const float* one, const float* two, const float* three);
  void DefineTriangleCCW(uint32_t start_index, const float* one, const float* two, const float* three,
                         const float* normal_one, const float* normal_two, const float* normal_three);
  void DefineTriangleCCW(uint32_t start_index, const float* one, const float* two, const float* three,
                         const Color& one_diffuse, const Color& two_diffuse, const Color& three_diffuse);
  void DefineTriangleCCW(uint32_t start_index, const float* one, const float* two, const float* three,
                         const float* normal_one, const float* normal_two, const float* normal_three,
                         const Color& diffuse_one, const Color& diffuse_two, const Color& diffuse_three);
  void DefineTriangle(uint32_t start_index, const float* one, const float* two, const float* three);
  void DefineTriangle(uint32_t start_index, const float* one, const float* two, const float* three,
                      const float* normal_one, const float* normal_two, const float* normal_three);
  void DefineTriangle(uint32_t start_index, const float* one, const float* two, const float* three,
                      const Color& diffuse_one, const Color& diffuse_two, const Color& diffuse_three);
  void DefineTriangle(uint32_t start_index, const float* one, const float* two, const float* three,
                      const float* normal_one, const float* normal_two, const float* normal_three,
                      const Color& diffuse_one, const Color& diffuse_two, const Color& diffuse_three);

  // Defines a quad made of two coplanar triangles (i.e., 6 vertices suitable for rendering as triangle primitives)
  void DefineBiTriCCW(uint32_t start_index, float left, float top, float right, float bottom);
  void DefineBiTriCCW(uint32_t start_index, float left, float top, float right, float bottom, float z);
  void DefineBiTriCCW(uint32_t start_index, float left, float top, float right, float bottom, float ul_z, float ll_z,
                      float lr_z, float ur_z);
  void DefineBiTriCCW(uint32_t start_index, float left, float top, float right, float bottom, float ul_z, float ll_z,
                      float lr_z, float ur_z, const Color& ul_diffuse, const Color& ll_diffuse, const Color& lr_diffuse,
                      const Color& ur_diffuse);
  void DefineBiTriCCW(uint32_t start_index, float left, float top, float right, float bottom, float ul_z, float ll_z,
                      float lr_z, float ur_z, const Color& ul_diffuse, const Color& ll_diffuse, const Color& lr_diffuse,
                      const Color& ur_diffuse, const Color& ul_specular, const Color& ll_specular,
                      const Color& lr_specular, const Color& ur_specular);

  void DefineBiTri(uint32_t start_index, float left, float top, float right, float bottom);
  void DefineBiTri(uint32_t start_index, float left, float top, float right, float bottom, float z);
  void DefineBiTri(uint32_t start_index, float left, float top, float right, float bottom, float ul_z, float ll_z,
                   float lr_z, float ur_z);
  void DefineBiTri(uint32_t start_index, float left, float top, float right, float bottom, float ul_z, float ll_z,
                   float lr_z, float ur_z, const Color& ul_diffuse, const Color& ll_diffuse, const Color& lr_diffuse,
                   const Color& ur_diffuse);
  void DefineBiTri(uint32_t start_index, float left, float top, float right, float bottom, float ul_z, float ll_z,
                   float lr_z, float ur_z, const Color& ul_diffuse, const Color& ll_diffuse, const Color& lr_diffuse,
                   const Color& ur_diffuse, const Color& ul_specular, const Color& ll_specular,
                   const Color& lr_specular, const Color& ur_specular);

  void SetDiffuse(uint32_t vertex_index, const Color& color);
  void SetSpecular(uint32_t vertex_index, const Color& color);

  inline void SetPositionIncludesW(bool enabled = true) { position_count_ = enabled ? 4 : 3; }

  inline void SetNormalIncludesW(bool enabled = true) { normal_count_ = enabled ? 4 : 3; }

 private:
  friend class TestHost;

  uint32_t num_vertices_;
  Vertex* linear_vertex_buffer_ = nullptr;      // texcoords 0 to kFramebufferWidth/kFramebufferHeight
  Vertex* normalized_vertex_buffer_ = nullptr;  // texcoords normalized 0 to 1

  // Number of components in the vertex position (3 or 4).
  uint32_t position_count_ = 3;
  // Number of components in the vertex normal (3 or 4).
  uint32_t normal_count_ = 3;

  bool cache_valid_{false};  // Indicates whether the HW should be forced to reload this buffer.
};

#endif  // NXDK_PGRAPH_TESTS__VERTEX_BUFFER_H_
