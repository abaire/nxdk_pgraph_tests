#include "pushbuffer.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"

// Maximum number of DWORDS per begin/end block, as enforced by pbkit with some headroom.
static constexpr uint32_t kMaxElementsPerBlock = 96;
// Maximum number of DWORDs in the pushbuffer, as defined by pbkit with some headroom.
static constexpr uint32_t kMaxElements = PBKIT_PUSHBUFFER_SIZE / 5;

Pushbuffer *Pushbuffer::singleton_ = nullptr;

void Pushbuffer::Initialize() {
  if (!singleton_) {
    singleton_ = new Pushbuffer();
  }
}

void Pushbuffer::Begin() {
  ASSERT(singleton_);
  ASSERT(!singleton_->head_ && "Nested pushbuffer blocks are not supported");

  singleton_->head_ = pb_begin();
  singleton_->current_block_elements_ = 0;
}

void Pushbuffer::End(bool flush) {
  ASSERT(singleton_->head_ && "End must not be called without Begin");

  pb_end(singleton_->head_);

  singleton_->current_block_elements_ = 0;
  singleton_->head_ = nullptr;

  if (flush) {
    Flush();
  }
}

void Pushbuffer::Flush() {
  ASSERT(!singleton_->head_ && "Flush must not be called within a pushbuffer block");

  while (pb_busy()) {
    /* Wait for completion... */
  }

  pb_reset();

  singleton_->current_block_elements_ = 0;
  singleton_->total_block_elements_ = 0;
}

void Pushbuffer::Reserve(uint32_t num_dwords) {
  total_block_elements_ += num_dwords;
  current_block_elements_ += num_dwords;

  if (total_block_elements_ >= kMaxElements) {
    End(true);
    Begin();
    return;
  }

  if (current_block_elements_ >= kMaxElementsPerBlock) {
    End();
    Begin();
  }
}

void Pushbuffer::PushTo(uint32_t subchannel, uint32_t command, uint32_t param1) {
  singleton_->Reserve(2);
  singleton_->head_ = pb_push1_to(subchannel, singleton_->head_, command, param1);
}

//! Pushes the given command and params to the given subchannel, returning a pointer to the next pushbuffer index to
//! facilitate chaining.
void Pushbuffer::PushTo(uint32_t subchannel, uint32_t command, uint32_t param1, uint32_t param2) {
  singleton_->Reserve(3);
  singleton_->head_ = pb_push2_to(subchannel, singleton_->head_, command, param1, param2);
}

void Pushbuffer::PushTo(uint32_t subchannel, uint32_t command, uint32_t param1, uint32_t param2, uint32_t param3) {
  singleton_->Reserve(4);
  singleton_->head_ = pb_push3_to(subchannel, singleton_->head_, command, param1, param2, param3);
}

void Pushbuffer::PushTo(uint32_t subchannel, uint32_t command, uint32_t param1, uint32_t param2, uint32_t param3,
                        uint32_t param4) {
  singleton_->Reserve(5);
  singleton_->head_ = pb_push4_to(subchannel, singleton_->head_, command, param1, param2, param3, param4);
}

void Pushbuffer::PushTo(uint32_t subchannel, uint32_t command, float param1, float param2, float param3, float param4) {
  singleton_->Reserve(5);
  singleton_->head_ = pb_push4f_to(subchannel, singleton_->head_, command, param1, param2, param3, param4);
}

void Pushbuffer::PushF(uint32_t command, float param1) {
  singleton_->Reserve(2);
  singleton_->head_ = pb_push1f(singleton_->head_, command, param1);
}

void Pushbuffer::PushF(uint32_t command, float param1, float param2) {
  singleton_->Reserve(3);
  singleton_->head_ = pb_push2f(singleton_->head_, command, param1, param2);
}

void Pushbuffer::PushF(uint32_t command, float param1, float param2, float param3) {
  singleton_->Reserve(4);
  singleton_->head_ = pb_push3f(singleton_->head_, command, param1, param2, param3);
}

void Pushbuffer::PushF(uint32_t command, float param1, float param2, float param3, float param4) {
  singleton_->Reserve(5);
  singleton_->head_ = pb_push4f(singleton_->head_, command, param1, param2, param3, param4);
}

void Pushbuffer::Push(uint32_t command, uint32_t param1) {
  singleton_->Reserve(2);
  singleton_->head_ = pb_push1(singleton_->head_, command, param1);
}

void Pushbuffer::Push(uint32_t command, uint32_t param1, uint32_t param2) {
  singleton_->Reserve(3);
  singleton_->head_ = pb_push2(singleton_->head_, command, param1, param2);
}

void Pushbuffer::Push(uint32_t command, uint32_t param1, uint32_t param2, uint32_t param3) {
  singleton_->Reserve(4);
  singleton_->head_ = pb_push3(singleton_->head_, command, param1, param2, param3);
}

void Pushbuffer::Push(uint32_t command, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t param4) {
  singleton_->Reserve(5);
  singleton_->head_ = pb_push4(singleton_->head_, command, param1, param2, param3, param4);
}

void Pushbuffer::Push2F(uint32_t command, const float *vector2) {
  singleton_->Reserve(3);
  singleton_->head_ = pb_push2fv(singleton_->head_, command, vector2);
}

void Pushbuffer::Push3F(uint32_t command, const float *vector3) {
  singleton_->Reserve(4);
  singleton_->head_ = pb_push3fv(singleton_->head_, command, vector3);
}

void Pushbuffer::Push4F(uint32_t command, const float *vector4) {
  singleton_->Reserve(5);
  singleton_->head_ = pb_push4fv(singleton_->head_, command, vector4);
}

void Pushbuffer::Push2(uint32_t command, const DWORD *vector2) {
  singleton_->Reserve(3);
  singleton_->head_ = pb_push2v(singleton_->head_, command, vector2);
}

void Pushbuffer::Push3(uint32_t command, const DWORD *vector3) {
  singleton_->Reserve(4);
  singleton_->head_ = pb_push3v(singleton_->head_, command, vector3);
}

void Pushbuffer::Push4(uint32_t command, const DWORD *vector4) {
  singleton_->Reserve(5);
  singleton_->head_ = pb_push4v(singleton_->head_, command, vector4);
}

void Pushbuffer::PushN(uint32_t command, uint32_t num_values, const DWORD *values) {
  singleton_->Reserve(num_values + 1);
  pb_push(singleton_->head_++, command, num_values);
  memcpy(singleton_->head_, values, num_values * 4);
  singleton_->head_ += num_values;
}

void Pushbuffer::PushTransposedMatrix(uint32_t command, const float *m) {
  singleton_->Reserve(17);
  singleton_->head_ = pb_push_transposed_matrix(singleton_->head_, command, m);
}

void Pushbuffer::Push4x3Matrix(uint32_t command, const float *m) {
  singleton_->Reserve(13);
  singleton_->head_ = pb_push_4x3_matrix(singleton_->head_, command, m);
}

void Pushbuffer::Push4x4Matrix(uint32_t command, const float *m) {
  singleton_->Reserve(17);
  singleton_->head_ = pb_push_4x4_matrix(singleton_->head_, command, m);
}
