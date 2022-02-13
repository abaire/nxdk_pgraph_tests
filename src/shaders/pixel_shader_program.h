#ifndef NXDK_PGRAPH_TESTS_PIXEL_SHADER_PROGRAM_H
#define NXDK_PGRAPH_TESTS_PIXEL_SHADER_PROGRAM_H

class PixelShaderProgram {
 public:
  static void LoadTexturedPixelShader();
  static void LoadUntexturedPixelShader();
  static void DisablePixelShader();
};

#endif  // NXDK_PGRAPH_TESTS_PIXEL_SHADER_PROGRAM_H
