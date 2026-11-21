#ifndef NXDK_PGRAPH_TESTS_VERTEX_SHADER_EXCEPTIONAL_VALUE_TESTS_H
#define NXDK_PGRAPH_TESTS_VERTEX_SHADER_EXCEPTIONAL_VALUE_TESTS_H

#include <cstdint>
#include <string>

#include "test_host.h"
#include "test_suite.h"

//! One output component of a vertex shader op on exceptional inputs, as measured on hardware.
struct VshExcRecord {
  const char *label;
  //! Operands A, B and C, loaded into c[96], c[97] and c[98]. A is also supplied as the v1 attribute.
  uint32_t a[4];
  uint32_t b[4];
  uint32_t c[4];
  //! Lane of the test case shader's result that holds this component.
  uint8_t lane;
  //! Hardware result bits.
  uint32_t hw;
};

//! One registered test, drawn as a single screenshot: up to 14 records rendered with the same shader. An op with more
//! records is split into several tests, such as MUL_1 and MUL_2.
struct VshExcTestCase {
  const char *test_name;
  const char *title;
  //! Shader computing the result into r10.
  const uint32_t *shader;
  uint32_t shader_size;
  const VshExcRecord *records;
  uint32_t count;
};

//! An ARL input and what MOV oD0, c[A0 + 0] read on hardware.
struct VshExcArlRecord {
  const char *label;
  //! The ARL input, supplied as v9 (texcoord 0).
  uint32_t a[4];
  //! True if hardware read all zeros instead of one of the marker constants.
  bool reads_zero;
  //! Index of the marker constant read on hardware, when reads_zero is false.
  uint32_t index;
};

//! One registered ARL test, drawn as a single screenshot: up to 14 records.
struct VshExcArlTestCase {
  const char *test_name;
  const VshExcArlRecord *records;
  uint32_t count;
};

//! Draws one test of exceptional value records. Shared by the "Vertex shader exceptional" suites.
class VshExceptionalValueRenderer {
 public:
  explicit VshExceptionalValueRenderer(TestHost &host) : host_(host) {}

  void Initialize();

  //! Renders `test_case`. The caller is responsible for calling FinishDraw.
  void Draw(const VshExcTestCase &test_case);

  //! Renders `test_case`. The caller is responsible for calling FinishDraw.
  void Draw(const VshExcArlTestCase &test_case);

 private:
  void SetShader(const uint32_t *shader, uint32_t size);
  void DrawComparatorRow();

 private:
  TestHost &host_;
};

/**
 * Renders MUL, ADD, MAD, MIN, MAX, SLT, SGE, DP3, DPH, DP4 and DST results on exceptional inputs (+-0, +-inf, NaN
 * encodings, denormals and FLT_MAX). Expected values were measured bit-exactly on hardware by reading results back from
 * constant registers (the "VSH Exceptional" tests in nxdk_low_level_nv2a_tests). The "Vertex shader exceptional" suites
 * route the same computations to visible outputs so that the GPU shader translation used by emulators is exercised.
 *
 * Each test uses one shader that computes the op into r10, then compares all four lanes of r10 with the expected bits
 * in c[100]: lane i of SGE(r, e) * SGE(e, r) is 1 exactly when the bits of lane i match under the hardware's
 * sign-magnitude ordering. A DP4 with a one-hot constant picks the lane of the record. Only those exact 0 or 1 values
 * pass through the DP4, which would otherwise flush -0, denormals and NaN payloads.
 *
 * Each test renders one row per result component, labeled on the left. The cells in each row are:
 *  - hw: green if the result has the same bits as the hardware result, otherwise red.
 *  - rf: the expected value compared with itself. Always green on hardware. An emulator that compares IEEE style draws
 *    this red when the expected value is a NaN, meaning that its hw cell cannot judge the row.
 *
 * The bottom row draws raw SGE results (green = 1): SGE(NaN, NaN), SGE(-0, +0) and SGE(+0, denormal). Hardware gives
 * 1, 0, 0. An IEEE comparison gives 0, 1, 0 and one that also treats denormals as zero gives 0, 1, 1.
 *
 * Hardware values are verified for constant register readback. The rendered output still has to be confirmed on
 * hardware.
 */
class VertexShaderExceptionalMACTests : public TestSuite {
 public:
  VertexShaderExceptionalMACTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  static constexpr const char *kSuiteName = "Vertex shader exceptional MAC";
  VshExceptionalValueRenderer renderer_;
};

/**
 * Renders MOV results, with negation and swizzles, from a constant, a temporary and the v1 attribute on exceptional
 * inputs. The layout is described in VertexShaderExceptionalMACTests.
 */
class VertexShaderExceptionalMOVTests : public TestSuite {
 public:
  VertexShaderExceptionalMOVTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  static constexpr const char *kSuiteName = "Vertex shader exceptional MOV";
  VshExceptionalValueRenderer renderer_;
};

/**
 * Renders RCP, RCC, RSQ, EXPP, LOGP and LIT results on exceptional inputs, and the constant ARL addresses for them.
 * The layout is described in VertexShaderExceptionalMACTests.
 */
class VertexShaderExceptionalILUTests : public TestSuite {
 public:
  VertexShaderExceptionalILUTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  static constexpr const char *kSuiteName = "Vertex shader exceptional ILU";
  VshExceptionalValueRenderer renderer_;
};

/**
 * Renders MAC results with operand A from the v1 attribute (Attr_ tests) or a constant (Const_ tests), and the fog
 * distance chain rcp(P.w), P * rcp(P.w), d2 = DP3, rsq(d2), d2 * rsq(d2) with P from v1 or a constant. Hardware gives
 * identical results on both paths. The layout is described in VertexShaderExceptionalMACTests.
 */
class VertexShaderExceptionalPathsTests : public TestSuite {
 public:
  VertexShaderExceptionalPathsTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  static constexpr const char *kSuiteName = "Vertex shader exceptional paths";
  VshExceptionalValueRenderer renderer_;
};

#endif  // NXDK_PGRAPH_TESTS_VERTEX_SHADER_EXCEPTIONAL_VALUE_TESTS_H
