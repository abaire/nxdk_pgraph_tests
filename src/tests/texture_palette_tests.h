#ifndef NXDK_PGRAPH_TESTS_TEXTURE_PALETTE_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_PALETTE_TESTS_H

#include "test_suite.h"

/**
 * Tests texture palette loading, palette table swapping, and palette updates.
 */
class TexturePaletteTests : public TestSuite {
 public:
  TexturePaletteTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  //! Tests swapping palette lookup tables and updating palette colors between draws.
  void TestPaletteSwapping();

  //! Tests the behavior of swapping palettes with only the data after the first palette_size / 4 bytes being modified.
  void TestXemu2646();
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_PALETTE_TESTS_H
