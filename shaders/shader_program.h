#ifndef NXDK_PGRAPH_TESTS_SHADER_PROGRAM_H
#define NXDK_PGRAPH_TESTS_SHADER_PROGRAM_H

#include <cstdint>

class ShaderProgram {
 public:
  virtual void Activate() = 0;
  virtual void PrepareDraw() = 0;

 protected:
  static void LoadShaderProgram(const uint32_t *shader, uint32_t shader_size);
};

#endif  // NXDK_PGRAPH_TESTS_SHADER_PROGRAM_H
