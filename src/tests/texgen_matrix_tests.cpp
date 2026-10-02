#include "texgen_matrix_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "test_host.h"
#include "texture_format.h"
#include "texture_generator.h"
#include "vertex_buffer.h"
#include "xbox_math_matrix.h"
#include "xbox_math_types.h"

using namespace XboxMath;

static constexpr int kTextureWidth = 256;
static constexpr int kTextureHeight = 128;

static std::string TestNameForTexGenMode(TextureStage::TexGen mode);

static TextureStage::TexGen kTestModes[] = {
    TextureStage::TG_DISABLE,    TextureStage::TG_EYE_LINEAR,     TextureStage::TG_OBJECT_LINEAR,
    TextureStage::TG_SPHERE_MAP, TextureStage::TG_REFLECTION_MAP,
};

struct MatrixConfig {
  const char *name;
  bool matrix_enable{true};
  std::function<void(matrix4_t &)> setup;
};

static const MatrixConfig kMatrixConfigs[] = {
    {"MatrixOff", false,
     [](matrix4_t &m) {
       MatrixSetIdentity(m);
       vector_t scale = {2.0f, 2.0f, 2.0f, 1.0f};
       MatrixScale(m, scale);
     }},
    {"Identity", true, [](matrix4_t &m) { MatrixSetIdentity(m); }},
    {"Double", true,
     [](matrix4_t &m) {
       MatrixSetIdentity(m);
       vector_t scale = {2.0f, 2.0f, 2.0f, 1.0f};
       MatrixScale(m, scale);
     }},
    {"Half", true,
     [](matrix4_t &m) {
       MatrixSetIdentity(m);
       vector_t scale = {0.5f, 0.5f, 0.5f, 1.0f};
       MatrixScale(m, scale);
     }},
    {"ShiftHPlus", true,
     [](matrix4_t &m) {
       MatrixSetIdentity(m);
       vector_t translate = {0.5f, 0.0f, 0.0f, 0.0f};
       MatrixTranslate(m, translate);
     }},
    {"ShiftHMinus", true,
     [](matrix4_t &m) {
       MatrixSetIdentity(m);
       vector_t translate = {-0.5f, 0.0f, 0.0f, 0.0f};
       MatrixTranslate(m, translate);
     }},
    {"ShiftVPlus", true,
     [](matrix4_t &m) {
       MatrixSetIdentity(m);
       vector_t translate = {0.0f, 0.5f, 0.0f, 0.0f};
       MatrixTranslate(m, translate);
     }},
    {"ShiftVMinus", true,
     [](matrix4_t &m) {
       MatrixSetIdentity(m);
       vector_t translate = {0.0f, -0.5f, 0.0f, 0.0f};
       MatrixTranslate(m, translate);
     }},
    {"RotateX", true,
     [](matrix4_t &m) {
       MatrixSetIdentity(m);
       vector_t rot = {static_cast<float>(M_PI * 0.5), 0.0f, 0.0f, 0.0f};
       MatrixRotate(m, rot);
     }},
    {"RotateY", true,
     [](matrix4_t &m) {
       MatrixSetIdentity(m);
       vector_t rot = {0.0f, static_cast<float>(M_PI * 0.5), 0.0f, 0.0f};
       MatrixRotate(m, rot);
     }},
    {"RotateZ", true,
     [](matrix4_t &m) {
       MatrixSetIdentity(m);
       vector_t rot = {0.0f, 0.0f, static_cast<float>(M_PI * 0.5), 0.0f};
       MatrixRotate(m, rot);
     }},
    {"Negate", true,
     [](matrix4_t &m) {
       MatrixSetIdentity(m);
       vector_t scale = {-1.0f, -1.0f, 1.0f, 1.0f};
       MatrixScale(m, scale);
     }},
    {"Arbitrary", true,
     [](matrix4_t &m) {
       matrix4_t arbitrary = {
           {0.7089392f, 0.0f, 0.515f, 0.0f},
           {0.0f, 1.2603364f, 0.49f, 0.0f},
           {0.0f, 0.0f, 0.0f, 0.0f},
           {0.0f, 0.0f, 1.0f, 0.0f},
       };
       MatrixCopyMatrix(m, arbitrary);
     }},
};

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc Disabled_MatrixOff
 *  Tests disabled TexGen with texture matrix disabled under STAGE_2D_PROJECTIVE.
 * @tc Disabled_Identity
 *  Tests disabled TexGen with identity texture matrix under STAGE_2D_PROJECTIVE.
 * @tc Disabled_Double
 *  Tests disabled TexGen with 2x scaling texture matrix under STAGE_2D_PROJECTIVE.
 * @tc Disabled_Half
 *  Tests disabled TexGen with 0.5x scaling texture matrix under STAGE_2D_PROJECTIVE.
 * @tc Disabled_ShiftHPlus
 *  Tests disabled TexGen with +0.5 horizontal translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc Disabled_ShiftHMinus
 *  Tests disabled TexGen with -0.5 horizontal translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc Disabled_ShiftVPlus
 *  Tests disabled TexGen with +0.5 vertical translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc Disabled_ShiftVMinus
 *  Tests disabled TexGen with -0.5 vertical translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc Disabled_RotateX
 *  Tests disabled TexGen with 90 deg X-rotation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc Disabled_RotateY
 *  Tests disabled TexGen with 90 deg Y-rotation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc Disabled_RotateZ
 *  Tests disabled TexGen with 90 deg Z-rotation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc Disabled_Negate
 *  Tests disabled TexGen with -1.0 negation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc Disabled_Arbitrary
 *  Tests disabled TexGen with arbitrary 4x4 texture matrix under STAGE_2D_PROJECTIVE.
 *
 * @tc EyeLinear_MatrixOff
 *  Tests EyeLinear TexGen with texture matrix disabled under STAGE_2D_PROJECTIVE.
 * @tc EyeLinear_Identity
 *  Tests EyeLinear TexGen with identity texture matrix under STAGE_2D_PROJECTIVE.
 * @tc EyeLinear_Double
 *  Tests EyeLinear TexGen with 2x scaling texture matrix under STAGE_2D_PROJECTIVE.
 * @tc EyeLinear_Half
 *  Tests EyeLinear TexGen with 0.5x scaling texture matrix under STAGE_2D_PROJECTIVE.
 * @tc EyeLinear_ShiftHPlus
 *  Tests EyeLinear TexGen with +0.5 horizontal translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc EyeLinear_ShiftHMinus
 *  Tests EyeLinear TexGen with -0.5 horizontal translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc EyeLinear_ShiftVPlus
 *  Tests EyeLinear TexGen with +0.5 vertical translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc EyeLinear_ShiftVMinus
 *  Tests EyeLinear TexGen with -0.5 vertical translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc EyeLinear_RotateX
 *  Tests EyeLinear TexGen with 90 deg X-rotation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc EyeLinear_RotateY
 *  Tests EyeLinear TexGen with 90 deg Y-rotation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc EyeLinear_RotateZ
 *  Tests EyeLinear TexGen with 90 deg Z-rotation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc EyeLinear_Negate
 *  Tests EyeLinear TexGen with -1.0 negation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc EyeLinear_Arbitrary
 *  Tests EyeLinear TexGen with arbitrary 4x4 texture matrix under STAGE_2D_PROJECTIVE.
 *
 * @tc ObjectLinear_MatrixOff
 *  Tests ObjectLinear TexGen with texture matrix disabled under STAGE_2D_PROJECTIVE.
 * @tc ObjectLinear_Identity
 *  Tests ObjectLinear TexGen with identity texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ObjectLinear_Double
 *  Tests ObjectLinear TexGen with 2x scaling texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ObjectLinear_Half
 *  Tests ObjectLinear TexGen with 0.5x scaling texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ObjectLinear_ShiftHPlus
 *  Tests ObjectLinear TexGen with +0.5 horizontal translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ObjectLinear_ShiftHMinus
 *  Tests ObjectLinear TexGen with -0.5 horizontal translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ObjectLinear_ShiftVPlus
 *  Tests ObjectLinear TexGen with +0.5 vertical translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ObjectLinear_ShiftVMinus
 *  Tests ObjectLinear TexGen with -0.5 vertical translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ObjectLinear_RotateX
 *  Tests ObjectLinear TexGen with 90 deg X-rotation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ObjectLinear_RotateY
 *  Tests ObjectLinear TexGen with 90 deg Y-rotation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ObjectLinear_RotateZ
 *  Tests ObjectLinear TexGen with 90 deg Z-rotation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ObjectLinear_Negate
 *  Tests ObjectLinear TexGen with -1.0 negation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ObjectLinear_Arbitrary
 *  Tests ObjectLinear TexGen with arbitrary 4x4 texture matrix under STAGE_2D_PROJECTIVE.
 *
 * @tc SphereMap_MatrixOff
 *  Tests SphereMap TexGen with texture matrix disabled under STAGE_2D_PROJECTIVE.
 * @tc SphereMap_Identity
 *  Tests SphereMap TexGen with identity texture matrix under STAGE_2D_PROJECTIVE.
 * @tc SphereMap_Double
 *  Tests SphereMap TexGen with 2x scaling texture matrix under STAGE_2D_PROJECTIVE.
 * @tc SphereMap_Half
 *  Tests SphereMap TexGen with 0.5x scaling texture matrix under STAGE_2D_PROJECTIVE.
 * @tc SphereMap_ShiftHPlus
 *  Tests SphereMap TexGen with +0.5 horizontal translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc SphereMap_ShiftHMinus
 *  Tests SphereMap TexGen with -0.5 horizontal translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc SphereMap_ShiftVPlus
 *  Tests SphereMap TexGen with +0.5 vertical translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc SphereMap_ShiftVMinus
 *  Tests SphereMap TexGen with -0.5 vertical translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc SphereMap_RotateX
 *  Tests SphereMap TexGen with 90 deg X-rotation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc SphereMap_RotateY
 *  Tests SphereMap TexGen with 90 deg Y-rotation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc SphereMap_RotateZ
 *  Tests SphereMap TexGen with 90 deg Z-rotation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc SphereMap_Negate
 *  Tests SphereMap TexGen with -1.0 negation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc SphereMap_Arbitrary
 *  Tests SphereMap TexGen with arbitrary 4x4 texture matrix under STAGE_2D_PROJECTIVE.
 *
 * @tc ReflectionMap_MatrixOff
 *  Tests ReflectionMap TexGen with texture matrix disabled under STAGE_2D_PROJECTIVE.
 * @tc ReflectionMap_Identity
 *  Tests ReflectionMap TexGen with identity texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ReflectionMap_Double
 *  Tests ReflectionMap TexGen with 2x scaling texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ReflectionMap_Half
 *  Tests ReflectionMap TexGen with 0.5x scaling texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ReflectionMap_ShiftHPlus
 *  Tests ReflectionMap TexGen with +0.5 horizontal translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ReflectionMap_ShiftHMinus
 *  Tests ReflectionMap TexGen with -0.5 horizontal translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ReflectionMap_ShiftVPlus
 *  Tests ReflectionMap TexGen with +0.5 vertical translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ReflectionMap_ShiftVMinus
 *  Tests ReflectionMap TexGen with -0.5 vertical translation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ReflectionMap_RotateX
 *  Tests ReflectionMap TexGen with 90 deg X-rotation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ReflectionMap_RotateY
 *  Tests ReflectionMap TexGen with 90 deg Y-rotation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ReflectionMap_RotateZ
 *  Tests ReflectionMap TexGen with 90 deg Z-rotation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ReflectionMap_Negate
 *  Tests ReflectionMap TexGen with -1.0 negation texture matrix under STAGE_2D_PROJECTIVE.
 * @tc ReflectionMap_Arbitrary
 *  Tests ReflectionMap TexGen with arbitrary 4x4 texture matrix under STAGE_2D_PROJECTIVE.
 *
 * @tc PassThrough_Disabled_MatrixOff
 *  Tests disabled TexGen with texture matrix disabled under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_Disabled_Identity
 *  Tests disabled TexGen with identity texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_Disabled_Double
 *  Tests disabled TexGen with 2x scaling texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_Disabled_Half
 *  Tests disabled TexGen with 0.5x scaling texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_Disabled_ShiftHPlus
 *  Tests disabled TexGen with +0.5 horizontal translation texture matrix under STAGE_PASS_THROUGH summed with a
 * positive texture.
 * @tc PassThrough_Disabled_ShiftHMinus
 *  Tests disabled TexGen with -0.5 horizontal translation texture matrix under STAGE_PASS_THROUGH summed with a
 * positive texture, evaluating whether negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_Disabled_ShiftVPlus
 *  Tests disabled TexGen with +0.5 vertical translation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 * @tc PassThrough_Disabled_ShiftVMinus
 *  Tests disabled TexGen with -0.5 vertical translation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture, evaluating whether negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_Disabled_RotateX
 *  Tests disabled TexGen with 90 deg X-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_Disabled_RotateY
 *  Tests disabled TexGen with 90 deg Y-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_Disabled_RotateZ
 *  Tests disabled TexGen with 90 deg Z-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_Disabled_Negate
 *  Tests disabled TexGen with -1.0 negation texture matrix under STAGE_PASS_THROUGH summed with a positive texture,
 *  evaluating whether all-negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_Disabled_Arbitrary
 *  Tests disabled TexGen with arbitrary 4x4 texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 *
 * @tc PassThrough_EyeLinear_MatrixOff
 *  Tests EyeLinear TexGen with texture matrix disabled under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_EyeLinear_Identity
 *  Tests EyeLinear TexGen with identity texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_EyeLinear_Double
 *  Tests EyeLinear TexGen with 2x scaling texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_EyeLinear_Half
 *  Tests EyeLinear TexGen with 0.5x scaling texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_EyeLinear_ShiftHPlus
 *  Tests EyeLinear TexGen with +0.5 horizontal translation texture matrix under STAGE_PASS_THROUGH summed with a
 * positive texture.
 * @tc PassThrough_EyeLinear_ShiftHMinus
 *  Tests EyeLinear TexGen with -0.5 horizontal translation texture matrix under STAGE_PASS_THROUGH summed with a
 * positive texture, evaluating whether negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_EyeLinear_ShiftVPlus
 *  Tests EyeLinear TexGen with +0.5 vertical translation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 * @tc PassThrough_EyeLinear_ShiftVMinus
 *  Tests EyeLinear TexGen with -0.5 vertical translation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture, evaluating whether negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_EyeLinear_RotateX
 *  Tests EyeLinear TexGen with 90 deg X-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 * @tc PassThrough_EyeLinear_RotateY
 *  Tests EyeLinear TexGen with 90 deg Y-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 * @tc PassThrough_EyeLinear_RotateZ
 *  Tests EyeLinear TexGen with 90 deg Z-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 * @tc PassThrough_EyeLinear_Negate
 *  Tests EyeLinear TexGen with -1.0 negation texture matrix under STAGE_PASS_THROUGH summed with a positive texture,
 *  evaluating whether all-negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_EyeLinear_Arbitrary
 *  Tests EyeLinear TexGen with arbitrary 4x4 texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 *
 * @tc PassThrough_ObjectLinear_MatrixOff
 *  Tests ObjectLinear TexGen with texture matrix disabled under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_ObjectLinear_Identity
 *  Tests ObjectLinear TexGen with identity texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_ObjectLinear_Double
 *  Tests ObjectLinear TexGen with 2x scaling texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_ObjectLinear_Half
 *  Tests ObjectLinear TexGen with 0.5x scaling texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_ObjectLinear_ShiftHPlus
 *  Tests ObjectLinear TexGen with +0.5 horizontal translation texture matrix under STAGE_PASS_THROUGH summed with a
 * positive texture.
 * @tc PassThrough_ObjectLinear_ShiftHMinus
 *  Tests ObjectLinear TexGen with -0.5 horizontal translation texture matrix under STAGE_PASS_THROUGH summed with a
 * positive texture, evaluating whether negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_ObjectLinear_ShiftVPlus
 *  Tests ObjectLinear TexGen with +0.5 vertical translation texture matrix under STAGE_PASS_THROUGH summed with a
 * positive texture.
 * @tc PassThrough_ObjectLinear_ShiftVMinus
 *  Tests ObjectLinear TexGen with -0.5 vertical translation texture matrix under STAGE_PASS_THROUGH summed with a
 * positive texture, evaluating whether negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_ObjectLinear_RotateX
 *  Tests ObjectLinear TexGen with 90 deg X-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 * @tc PassThrough_ObjectLinear_RotateY
 *  Tests ObjectLinear TexGen with 90 deg Y-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 * @tc PassThrough_ObjectLinear_RotateZ
 *  Tests ObjectLinear TexGen with 90 deg Z-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 * @tc PassThrough_ObjectLinear_Negate
 *  Tests ObjectLinear TexGen with -1.0 negation texture matrix under STAGE_PASS_THROUGH summed with a positive texture,
 *  evaluating whether all-negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_ObjectLinear_Arbitrary
 *  Tests ObjectLinear TexGen with arbitrary 4x4 texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 *
 * @tc PassThrough_SphereMap_MatrixOff
 *  Tests SphereMap TexGen with texture matrix disabled under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_SphereMap_Identity
 *  Tests SphereMap TexGen with identity texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_SphereMap_Double
 *  Tests SphereMap TexGen with 2x scaling texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_SphereMap_Half
 *  Tests SphereMap TexGen with 0.5x scaling texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_SphereMap_ShiftHPlus
 *  Tests SphereMap TexGen with +0.5 horizontal translation texture matrix under STAGE_PASS_THROUGH summed with a
 * positive texture.
 * @tc PassThrough_SphereMap_ShiftHMinus
 *  Tests SphereMap TexGen with -0.5 horizontal translation texture matrix under STAGE_PASS_THROUGH summed with a
 * positive texture, evaluating whether negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_SphereMap_ShiftVPlus
 *  Tests SphereMap TexGen with +0.5 vertical translation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 * @tc PassThrough_SphereMap_ShiftVMinus
 *  Tests SphereMap TexGen with -0.5 vertical translation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture, evaluating whether negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_SphereMap_RotateX
 *  Tests SphereMap TexGen with 90 deg X-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 * @tc PassThrough_SphereMap_RotateY
 *  Tests SphereMap TexGen with 90 deg Y-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 * @tc PassThrough_SphereMap_RotateZ
 *  Tests SphereMap TexGen with 90 deg Z-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 * @tc PassThrough_SphereMap_Negate
 *  Tests SphereMap TexGen with -1.0 negation texture matrix under STAGE_PASS_THROUGH summed with a positive texture,
 *  evaluating whether all-negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_SphereMap_Arbitrary
 *  Tests SphereMap TexGen with arbitrary 4x4 texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 *
 * @tc PassThrough_ReflectionMap_MatrixOff
 *  Tests ReflectionMap TexGen with texture matrix disabled under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_ReflectionMap_Identity
 *  Tests ReflectionMap TexGen with identity texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_ReflectionMap_Double
 *  Tests ReflectionMap TexGen with 2x scaling texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_ReflectionMap_Half
 *  Tests ReflectionMap TexGen with 0.5x scaling texture matrix under STAGE_PASS_THROUGH summed with a positive texture.
 * @tc PassThrough_ReflectionMap_ShiftHPlus
 *  Tests ReflectionMap TexGen with +0.5 horizontal translation texture matrix under STAGE_PASS_THROUGH summed with a
 * positive texture.
 * @tc PassThrough_ReflectionMap_ShiftHMinus
 *  Tests ReflectionMap TexGen with -0.5 horizontal translation texture matrix under STAGE_PASS_THROUGH summed with a
 * positive texture, evaluating whether negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_ReflectionMap_ShiftVPlus
 *  Tests ReflectionMap TexGen with +0.5 vertical translation texture matrix under STAGE_PASS_THROUGH summed with a
 * positive texture.
 * @tc PassThrough_ReflectionMap_ShiftVMinus
 *  Tests ReflectionMap TexGen with -0.5 vertical translation texture matrix under STAGE_PASS_THROUGH summed with a
 * positive texture, evaluating whether negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_ReflectionMap_RotateX
 *  Tests ReflectionMap TexGen with 90 deg X-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 * @tc PassThrough_ReflectionMap_RotateY
 *  Tests ReflectionMap TexGen with 90 deg Y-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 * @tc PassThrough_ReflectionMap_RotateZ
 *  Tests ReflectionMap TexGen with 90 deg Z-rotation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 * @tc PassThrough_ReflectionMap_Negate
 *  Tests ReflectionMap TexGen with -1.0 negation texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture, evaluating whether all-negative coordinates clamp to zero or darken below the positive texture.
 * @tc PassThrough_ReflectionMap_Arbitrary
 *  Tests ReflectionMap TexGen with arbitrary 4x4 texture matrix under STAGE_PASS_THROUGH summed with a positive
 * texture.
 */
TexgenMatrixTests::TexgenMatrixTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texgen with texture matrix", config) {
  for (auto mode : kTestModes) {
    std::string name = TestNameForTexGenMode(mode);

    // STAGE_2D_PROJECTIVE tests
    for (const auto &cfg : kMatrixConfigs) {
      std::string test_name = name + "_" + cfg.name;
      tests_[test_name] = [this, test_name, mode, &cfg]() {
        matrix4_t matrix;
        cfg.setup(matrix);
        Test(test_name, matrix, mode, cfg.matrix_enable, TestHost::STAGE_2D_PROJECTIVE);
      };
    }

    // STAGE_PASS_THROUGH tests
    for (const auto &cfg : kMatrixConfigs) {
      std::string test_name = "PassThrough_" + name + "_" + cfg.name;
      tests_[test_name] = [this, test_name, mode, &cfg]() {
        matrix4_t matrix;
        cfg.setup(matrix);
        Test(test_name, matrix, mode, cfg.matrix_enable, TestHost::STAGE_PASS_THROUGH);
      };
    }
  }
}

void TexgenMatrixTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, false);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  // Stage 0: 2D projective gradient texture (used by STAGE_2D_PROJECTIVE tests)
  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R8G8B8A8), 0);
  {
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetBorderColor(0xFF7F007F);
    texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
  }

  SDL_Surface *gradient_surface;
  int update_texture_result = GenerateSurface(&gradient_surface, kTextureWidth, kTextureHeight);
  ASSERT(!update_texture_result && "Failed to generate SDL surface");
  update_texture_result = host_.SetTexture(gradient_surface, 0);
  SDL_FreeSurface(gradient_surface);
  ASSERT(!update_texture_result && "Failed to set texture 0");

  // Stage 1: Positive checkerboard texture (used by STAGE_PASS_THROUGH tests)
  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R8G8B8A8), 1);
  {
    auto &texture_stage = host_.GetTextureStage(1);
    texture_stage.SetBorderColor(0xFF7F007F);
    texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    texture_stage.SetTexgenS(TextureStage::TG_DISABLE);
    texture_stage.SetTexgenT(TextureStage::TG_DISABLE);
    texture_stage.SetTexgenR(TextureStage::TG_DISABLE);
    texture_stage.SetTextureMatrixEnable(false);
  }

  GenerateSwizzledRGBACheckerboard(host_.GetTextureMemoryForStage(1), 0, 0, kTextureWidth, kTextureHeight,
                                   kTextureWidth * 4, 0xFF606060, 0xFF909090);

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
}

void TexgenMatrixTests::CreateGeometry() {
  static constexpr float left = -2.75f;
  static constexpr float right = 2.75f;
  static constexpr float top = 1.0f;
  static constexpr float bottom = -2.5f;

  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6);

  buffer->DefineBiTri(0, left, top, right, bottom);
}

void TexgenMatrixTests::Test(const std::string &test_name, const matrix4_t &matrix, TextureStage::TexGen gen_mode,
                             bool matrix_enable, TestHost::ShaderStageProgram stage_program) {
  host_.PrepareDraw(0xFE202020);

  if (stage_program == TestHost::STAGE_PASS_THROUGH) {
    host_.SetTextureStageEnabled(1, true);
    host_.SetShaderStageProgram(TestHost::STAGE_PASS_THROUGH, TestHost::STAGE_2D_PROJECTIVE);

    host_.SetCombinerControl(1, true, true);
    // Combiner stage 0:
    // AB = Tex1 (positive texture) * 1.0
    // CD = SIGNED_IDENTITY(Tex0) (pass-through coordinates) * 1.0
    // Sum = AB + CD = Tex1 + SIGNED_IDENTITY(Tex0)
    host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_TEX1, TestHost::MAP_UNSIGNED_IDENTITY),
                                TestHost::ColorInput(TestHost::SRC_ZERO, TestHost::MAP_UNSIGNED_INVERT),
                                TestHost::ColorInput(TestHost::SRC_TEX0, TestHost::MAP_SIGNED_IDENTITY),
                                TestHost::ColorInput(TestHost::SRC_ZERO, TestHost::MAP_UNSIGNED_INVERT));
    host_.SetOutputColorCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0, false, false,
                                 TestHost::SM_SUM);
    host_.SetInputAlphaCombiner(0, TestHost::ZeroInput(), TestHost::ZeroInput());
    host_.SetOutputAlphaCombiner(0, TestHost::DST_DISCARD);

    host_.SetFinalCombiner0Just(TestHost::SRC_R0);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  } else {
    host_.SetTextureStageEnabled(1, false);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

    host_.SetCombinerControl(1, true, true);
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  }

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetTexgenS(gen_mode);
  texture_stage.SetTexgenT(gen_mode);
  if (gen_mode == TextureStage::TG_SPHERE_MAP) {
    texture_stage.SetTexgenR(TextureStage::TG_DISABLE);
  } else {
    texture_stage.SetTexgenR(gen_mode);
  }
  texture_stage.SetTextureMatrixEnable(matrix_enable);
  MatrixCopyMatrix(texture_stage.GetTextureMatrix(), matrix);
  if (!matrix_enable) {
    Pushbuffer::Begin();
    Pushbuffer::Push4x4Matrix(NV097_SET_TEXTURE_MATRIX, matrix[0]);
    Pushbuffer::End();
  }

  host_.SetupTextureStages();
  host_.DrawArrays();

  pb_print("%s\n", test_name.c_str());
  if (matrix_enable) {
    const float *val = texture_stage.GetTextureMatrix()[0];
    for (auto i = 0; i < 4; ++i, val += 4) {
      pb_print_with_floats("%.3f %.3f %.3f %.3f\n", val[0], val[1], val[2], val[3]);
    }
  } else {
    pb_print("Matrix: Disabled\n");
  }
  pb_draw_text_screen();

  FinishDraw(test_name);
}

static std::string TestNameForTexGenMode(TextureStage::TexGen mode) {
  switch (mode) {
    case TextureStage::TG_DISABLE:
      return "Disabled";

    case TextureStage::TG_EYE_LINEAR:
      return "EyeLinear";

    case TextureStage::TG_OBJECT_LINEAR:
      return "ObjectLinear";

    case TextureStage::TG_SPHERE_MAP:
      return "SphereMap";

    case TextureStage::TG_REFLECTION_MAP:
      return "ReflectionMap";

    default:
      return "Unsupported mode";
  }
}
