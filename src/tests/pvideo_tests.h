#pragma once

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests the PVIDEO subsystem.
 */
class PvideoTests : public TestSuite {
 public:
  PvideoTests(TestHost &host, std::string output_dir, const Config &config);
  void Initialize() override;
  void Deinitialize() override;

 private:
  //! Tests stopping the PVIDEO overlay and tearing down overlay registers.
  void TestStopBehavior();

  //! Tests stopping the overlay via PMC enable registers.
  void TestAlternateStopBehavior();

  //! Tests maximum input size configuration with unity scale deltas.
  void TestSizeInMaxUnityDeltas();

  //! Tests maximum input size configuration with downscaling deltas.
  void TestSizeInMaxLargeDelta();

  //! Tests maximum input size configuration with upscaling deltas.
  void TestSizeInMaxSmallDelta();

  //! Tests max input size to small output rectangle with unity deltas.
  void TestSizeMaxOutSmallUnityDeltas();

  //! Tests max input size to small output rectangle with proportional scale deltas.
  void TestSizeMaxOutSmallCorrectDeltas();

  //! Tests scaling PAL video resolution into NTSC display overlay dimensions.
  void TestPALIntoNTSC();

  //! Tests downscaling where input size exceeds output size with unity deltas.
  void TestSizeInLargerThanSizeOutUnityDeltas();

  //! Tests downscaling where input size exceeds output size with proportional deltas.
  void TestSizeInLargerThanSizeOutCorrectDeltas();

  //! Tests upscaling where input size is smaller than output size with unity deltas.
  void TestSizeInSmallerThanSizeOutUnityDeltas();

  //! Tests upscaling where input size is smaller than output size with proportional deltas.
  void TestSizeInSmallerThanSizeOutCorrectDeltas();

  //! Tests video overlay buffer pitch smaller than compact line width.
  void TestPitchLessThanCompact();

  //! Tests video overlay buffer pitch larger than compact line width.
  void TestPitchLargerThanCompact();

  //! Tests various overlay pitch configurations against a ladder pattern.
  void TestPitch();

  //! Tests overlay color keying against background framebuffer pixels.
  void TestColorKey();

  //! Tests basic fullscreen overlay rendering on channel 0.
  void TestSimpleFullscreenOverlay0();

  //! Tests basic overlay rendering on channel 1.
  void TestOverlay1();

  //! Tests concurrent rendering of overlapping overlay channels 0 and 1.
  void TestOverlappedOverlays();

  //! Tests input video start coordinate offset (NV_PVIDEO_POINT_IN).
  void TestInPoint();

  //! Tests input video dimension configurations (NV_PVIDEO_SIZE_IN).
  void TestInSize();

  //! Tests output screen coordinate positioning (NV_PVIDEO_POINT_OUT).
  void TestOutPoint();

  //! Tests output screen dimension configurations (NV_PVIDEO_SIZE_OUT).
  void TestOutSize();

  //! Tests various input-to-output video aspect ratio scaling factors.
  void TestRatios();

  void DrawFullscreenOverlay();

 private:
  uint8_t *video_{nullptr};
  uint8_t *video2_{nullptr};
};
