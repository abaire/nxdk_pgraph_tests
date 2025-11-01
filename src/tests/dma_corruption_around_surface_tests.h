#ifndef NXDK_PGRAPH_TESTS_DMA_CORRUPTION_AROUND_SURFACE_TESTS_H
#define NXDK_PGRAPH_TESTS_DMA_CORRUPTION_AROUND_SURFACE_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests the behavior of DMA on xemu's implementation of GPU-owned overlay buffers.
 *
 * These tests are generally specific to emulator testing and likely should not serve as examples of how to do PGRAPH
 * interactions, nor are they likely to illuminate unexpected behavior in HW.
 */
class DMACorruptionAroundSurfaceTests : public TestSuite {
 public:
  DMACorruptionAroundSurfaceTests(TestHost& host, std::string output_dir, const Config& config);
  void Initialize() override;

 private:
  void Test();

  //! Tests the behavior of a read from the HDD into the current render target surface.
  //!
  //! As of xemu 0.8.110, this ends up discarding the data read from the HDD when the color target is modified from the
  //! test surface, prompting a download from GPU memory and restoring stale data.
  //!
  //! Note: This only appears to happen with HDD reads (e.g., from the cache partition), reading from the ROM appears to
  //! work reliably.
  void TestReadFromFileIntoSurface();

  //! Tests the behavior of a read from the HDD into an existing texture.
  //!
  //! As of xemu 0.8.110, this ends up discarding the data read from the HDD when the color target is modified from the
  //! test surface, prompting a download from GPU memory and restoring stale data.
  //!
  //! Note: This only appears to happen with HDD reads (e.g., from the cache partition), reading from the ROM appears to
  //! work reliably.
  void TestReadFromFileIntoTexture();

  void Draw(uint32_t stage = 0) const;
  void NoOpDraw() const;
  void WaitForGPU() const;
};

#endif  // NXDK_PGRAPH_TESTS_DMA_CORRUPTION_AROUND_SURFACE_TESTS_H
