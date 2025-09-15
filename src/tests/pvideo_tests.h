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
  void TestStopBehavior();
  void TestAlternateStopBehavior();

  void TestSizeInMaxUnityDeltas();
  void TestSizeInMaxLargeDelta();
  void TestSizeInMaxSmallDelta();
  void TestSizeMaxOutSmallUnityDeltas();
  void TestSizeMaxOutSmallCorrectDeltas();

  void TestPALIntoNTSC();
  void TestSizeInLargerThanSizeOutUnityDeltas();
  void TestSizeInLargerThanSizeOutCorrectDeltas();

  void TestSizeInSmallerThanSizeOutUnityDeltas();
  void TestSizeInSmallerThanSizeOutCorrectDeltas();

  void TestPitchLessThanCompact();
  void TestPitchLargerThanCompact();

  void TestColorKey();

  void TestSimpleFullscreenOverlay0();
  void TestOverlay1();
  void TestOverlappedOverlays();

  void DrawFullscreenOverlay();

 private:
  uint8_t *video_{nullptr};
  uint8_t *video2_{nullptr};
};
