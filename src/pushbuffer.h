#ifndef PUSHBUFFER_H
#define PUSHBUFFER_H

#include <xboxkrnl/xboxdef.h>

#include <cstdint>

//! Manages pb_kit pushbuffers to prevent overflows and optimize resets.
//!
//! In particular, this class manages the underlying pbkit begin/end block to
//! transparently break large logical blocks into chunks under the max size
//! allowed by pbkit.
class Pushbuffer {
 public:
  //! Initializes the pushbuffer singleton.
  static void Initialize();

  //! Starts a logical pushbuffer block.
  static void Begin();

  //! Commits any outstanding pushbuffer messages.
  static void End(bool flush = false);

  //! Flushes and ressets the underlying pushbuffer.
  static void Flush();

  //! Pushes the given command and param to the given subchannel.
  static void PushTo(uint32_t subchannel, uint32_t command, uint32_t param1);

  //! Pushes the given command and params to the given subchannel.
  static void PushTo(uint32_t subchannel, uint32_t command, uint32_t param1, uint32_t param2);

  //! Pushes the given command and params to the given subchannel.
  static void PushTo(uint32_t subchannel, uint32_t command, uint32_t param1, uint32_t param2, uint32_t param3);

  //! Pushes the given command and params to the given subchannel.
  static void PushTo(uint32_t subchannel, uint32_t command, uint32_t param1, uint32_t param2, uint32_t param3,
                     uint32_t param4);

  //! Pushes the given command and floating point params to the given subchannel.
  static void PushTo(uint32_t subchannel, uint32_t command, float param1, float param2, float param3, float param4);

  //! Pushes the given command and floating point param to the subchannel assigned for 3D operations.
  static void PushF(uint32_t command, float param1);

  //! Pushes the given command and floating point params to the subchannel assigned for 3D operations.
  static void PushF(uint32_t command, float param1, float param2);

  //! Pushes the given command and floating point params to the subchannel assigned for 3D operations.
  static void PushF(uint32_t command, float param1, float param2, float param3);

  //! Pushes the given command and floating point params to the subchannel assigned for 3D operations.
  static void PushF(uint32_t command, float param1, float param2, float param3, float param4);

  //! Pushes the given command and param to the subchannel assigned for 3D operations.
  static void Push(uint32_t command, uint32_t param1);

  //! Pushes the given command and params to the subchannel assigned for 3D operations.
  static void Push(uint32_t command, uint32_t param1, uint32_t param2);

  //! Pushes the given command and params to the subchannel assigned for 3D operations.
  static void Push(uint32_t command, uint32_t param1, uint32_t param2, uint32_t param3);

  //! Pushes the given command and params to the subchannel assigned for 3D operations.
  static void Push(uint32_t command, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t param4);

  //! Pushes the given command and 2-element vector of floating point params to the subchannel assigned for 3D
  //! operations.
  static void Push2F(uint32_t command, const float *vector2);

  //! Pushes the given command and 3-element vector of floating point params to the subchannel assigned for 3D
  //! operations.
  static void Push3F(uint32_t command, const float *vector3);

  //! Pushes the given command and 4-element vector of floating point params to the subchannel assigned for 3D
  //! operations.
  static void Push4F(uint32_t command, const float *vector4);

  //! Pushes the given command and 2-element vector of uint32_t params to the subchannel assigned for 3D operations
  static void Push2(uint32_t command, const DWORD *vector2);

  //! Pushes the given command and 3-element vector of uint32_t params to the subchannel assigned for 3D operations
  static void Push3(uint32_t command, const DWORD *vector3);

  //! Pushes the given command and 4-element vector of uint32_t params to the subchannel assigned for 3D operations
  static void Push4(uint32_t command, const DWORD *vector4);

  //! Pushes an arbitrary number of values to the subchannel assigned for 3D operations.
  static void PushN(uint32_t command, uint32_t num_values, const DWORD *values);

  //! Pushes the given command and 4x4 floating point matrix to the subchannel assigned for 3D operations. The matrix is
  //! pushed in transposed order.
  static void PushTransposedMatrix(uint32_t command, const float *m);

  //! Pushes a 4 column x 3 row matrix (used to push the inverse model view matrix which intentionally omits the 4th row
  //! and is not transposed).
  static void Push4x3Matrix(uint32_t command, const float *m);

  //! Pushes a 4x4 matrix without transposing it.
  static void Push4x4Matrix(uint32_t command, const float *m);

 private:
  Pushbuffer() = default;

  //! Ensures that at least num_dwords values may be added to the pushbuffer.
  void Reserve(uint32_t num_dwords);

  uint32_t *head_ = nullptr;

  //! The number of dwords in the current begin/end block.
  uint32_t current_block_elements_ = 0;

  //! The total number of dwords submitted since the last flush/reset.
  uint32_t total_block_elements_ = 0;

  static Pushbuffer *singleton_;
};

#endif  // PUSHBUFFER_H
