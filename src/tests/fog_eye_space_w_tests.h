#ifndef NXDK_PGRAPH_TESTS_FOG_EYE_SPACE_W_TESTS_H
#define NXDK_PGRAPH_TESTS_FOG_EYE_SPACE_W_TESTS_H

#include <string>

#include "test_suite.h"

class TestHost;

/**
 * Tests fixed function fog generation when the eye-space position has a w component other than 1.0.
 *
 * A model view matrix whose w column is not (0, 0, 0, 1) (e.g., a planar projected shadow matrix folded into the world
 * transform) or a vertex position with w != 1 produces a homogeneous eye-space position. The screen position is
 * unaffected since the composite matrix and perspective divide cancel the scale, but the fog distance may or may not
 * be computed from the dehomogenized position.
 *
 * Reproduces https://github.com/xemu-project/xemu/issues/3135 (Hunter: The Reckoning - Redeemer shadows rendered in the
 * fog color).
 *
 * Each test draws a series of horizontal strips. Every strip covers the same screen area at the same eye-space depths
 * (3.0 at the left edge to 40.0 at the right) and the composite matrix is never altered, so only the eye-space
 * position used for fog generation differs between strips. The first strip is the w = 1 reference. Strips blend from
 * a dark diffuse color toward a yellow fog color.
 *
 * Observations from HW:
 * - The eye-space position is divided by its w component before fog generation in planar, absolute planar, and radial
 *   modes. Uniformly scaling the model view matrix or the vertex position (including by a negative value) has no
 *   effect on fog, while altering only the eye-space w changes the fog distance to (eye.xyz / eye.w).
 * - Eye-space w of +0 and -0 behave identically. For planar and absolute planar generation the result matches a very
 *   large negative distance (no fog in linear and exp modes, full fog in exp2). For radial generation the result
 *   matches a distance of 0 (no fog in every mode).
 * - An eye-space position of (0, 0, 0, 0) produces no fog in every mode.
 * - Absolute planar generation does not discard the sign of a tiny negative w (-1e-6 matches planar, not radial).
 */
class FogEyeSpaceWTests : public TestSuite {
 public:
  enum class Variant {
    //! The model view matrix is uniformly scaled, so eye-space position = k * (x, y, z, 1).
    MV_SCALE,
    //! Vertices are submitted as k * (x, y, z, 1) with the default matrices.
    VERTEX_W,
    //! The model view w column is modified so that eye-space w = 1 + a * eye_x, leaving xyz unchanged.
    MV_PROJECTIVE,
    //! The model view w column is replaced with (0, 0, 0, w) for w at or near zero, leaving xyz unchanged.
    MV_ZERO_W,
  };

  FogEyeSpaceWTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void Test(const std::string& name, Variant variant, uint32_t fog_gen_mode, uint32_t fog_mode);
};

#endif  // NXDK_PGRAPH_TESTS_FOG_EYE_SPACE_W_TESTS_H
