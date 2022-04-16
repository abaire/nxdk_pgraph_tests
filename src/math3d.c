// port of ooPo's ps2sdk math3d library

#include "math3d.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

unsigned long times(void *);
#define cpu_ticks() times(0)

// vector functions

void vector_apply(VECTOR output, const VECTOR input0, const MATRIX input1) {
  output[_X] =
      input0[_X] * input1[_11] + input0[_Y] * input1[_12] + input0[_Z] * input1[_13] + input0[_W] * input1[_14];
  output[_Y] =
      input0[_X] * input1[_21] + input0[_Y] * input1[_22] + input0[_Z] * input1[_23] + input0[_W] * input1[_24];
  output[_Z] =
      input0[_X] * input1[_31] + input0[_Y] * input1[_32] + input0[_Z] * input1[_33] + input0[_W] * input1[_34];
  output[_W] =
      input0[_X] * input1[_41] + input0[_Y] * input1[_42] + input0[_Z] * input1[_43] + input0[_W] * input1[_44];
}

void vector_clamp(VECTOR output, const VECTOR input0, float min, float max) {
  VECTOR work;

  // Copy the vector.
  vector_copy(work, input0);

  // Clamp the minimum values.
  if (work[_X] < min) {
    work[_X] = min;
  }
  if (work[_Y] < min) {
    work[_Y] = min;
  }
  if (work[_Z] < min) {
    work[_Z] = min;
  }
  if (work[_W] < min) {
    work[_W] = min;
  }

  // Clamp the maximum values.
  if (work[_X] > max) {
    work[_X] = max;
  }
  if (work[_Y] > max) {
    work[_Y] = max;
  }
  if (work[_Z] > max) {
    work[_Z] = max;
  }
  if (work[_W] > max) {
    work[_W] = max;
  }

  // Output the result.
  vector_copy(output, work);
}

void vector_copy(VECTOR output, const VECTOR input0) { memcpy(output, input0, sizeof(VECTOR)); }

float vector_dot(const VECTOR input0, const VECTOR input1) {
  return vector_innerproduct(input0, input1);
}

float vector_innerproduct(const VECTOR input0, const VECTOR input1) {
  VECTOR work0, work1;

  // Normalize the first vector.
  work0[_X] = (input0[_X] / input0[_W]);
  work0[_Y] = (input0[_Y] / input0[_W]);
  work0[_Z] = (input0[_Z] / input0[_W]);
  work0[_W] = 1.00f;

  // Normalize the second vector.
  work1[_X] = (input1[_X] / input1[_W]);
  work1[_Y] = (input1[_Y] / input1[_W]);
  work1[_Z] = (input1[_Z] / input1[_W]);
  work1[_W] = 1.00f;

  // Return the inner product.
  return (work0[_X] * work1[_X]) + (work0[_Y] * work1[_Y]) + (work0[_Z] * work1[_Z]);
}

void vector_multiply(VECTOR output, const VECTOR input0, const VECTOR input1) {
  output[_X] = input0[_X] * input1[_X];
  output[_Y] = input0[_Y] * input1[_Y];
  output[_Z] = input0[_Z] * input1[_Z];
  output[_W] = input0[_W] * input1[_W];
}

void vector_subtract(VECTOR output, const VECTOR a, const VECTOR b) {
  assert(a[_W] == 1.0f && b[_W] == 1.0f);
  output[_X] = a[_X] - b[_X];
  output[_Y] = a[_Y] - b[_Y];
  output[_Z] = a[_Z] - b[_Z];
  output[_W] = 1.0f;
}

void vector_add(VECTOR output, const VECTOR a, const VECTOR b) {
  assert(a[_W] == 1.0f && b[_W] == 1.0f);
  output[_X] = a[_X] * b[_X];
  output[_Y] = a[_Y] * b[_Y];
  output[_Z] = a[_Z] * b[_Z];
  output[_W] = 1.0f;
}

void vector_normalize(VECTOR vector) {
  float k;

  k = 1.0f / sqrtf(vector[_X] * vector[_X] + vector[_Y] * vector[_Y] + vector[_Z] * vector[_Z]);
  vector[_X] *= k;
  vector[_Y] *= k;
  vector[_Z] *= k;
}

void vector_normalize_into(VECTOR output, const VECTOR input0) {
  float k;

  k = 1.0f / sqrtf(input0[_X] * input0[_X] + input0[_Y] * input0[_Y] + input0[_Z] * input0[_Z]);
  output[_X] = input0[_X] * k;
  output[_Y] = input0[_Y] * k;
  output[_Z] = input0[_Z] * k;
}

void vector_crossproduct(VECTOR output, const VECTOR input0, const VECTOR input1) {
  assert(input0[_W] == 1.0f && input1[_W] == 1.0f);
  vector_outerproduct(output, input0, input1);
  output[_W] = 1.0f;
}

void vector_outerproduct(VECTOR output, const VECTOR input0, const VECTOR input1) {
  assert(input0[_W] == 1.0f && input1[_W] == 1.0f);
  output[_X] = input0[_Y] * input1[_Z] - input0[_Z] * input1[_Y];
  output[_Y] = input0[_Z] * input1[_X] - input0[_X] * input1[_Z];
  output[_Z] = input0[_X] * input1[_Y] - input0[_Y] * input1[_X];
  output[_W] = 1.0f;
}

// matrices function

void matrix_copy(MATRIX output, const MATRIX input0) { memcpy(output, input0, sizeof(MATRIX)); }

void matrix_inverse(MATRIX output, const MATRIX input0) {
  MATRIX work;

  // Calculate the inverse of the matrix.
  matrix_transpose(work, input0);
  work[_14] = 0.00f;
  work[_24] = 0.00f;
  work[_34] = 0.00f;
  work[_41] = -(input0[_41] * work[_11] + input0[_42] * work[_21] + input0[_43] * work[_31]);
  work[_42] = -(input0[_41] * work[_12] + input0[_42] * work[_22] + input0[_43] * work[_32]);
  work[_43] = -(input0[_41] * work[_13] + input0[_42] * work[_23] + input0[_43] * work[_33]);
  work[_44] = 1.00f;

  // Output the result.
  matrix_copy(output, work);
}

void matrix_multiply(MATRIX output, const MATRIX input0, const MATRIX input1) {
  MATRIX work;

  work[_11] =
      input0[_11] * input1[_11] + input0[_12] * input1[_21] + input0[_13] * input1[_31] + input0[_14] * input1[_41];
  work[_12] =
      input0[_11] * input1[_12] + input0[_12] * input1[_22] + input0[_13] * input1[_32] + input0[_14] * input1[_42];
  work[_13] =
      input0[_11] * input1[_13] + input0[_12] * input1[_23] + input0[_13] * input1[_33] + input0[_14] * input1[_43];
  work[_14] =
      input0[_11] * input1[_14] + input0[_12] * input1[_24] + input0[_13] * input1[_34] + input0[_14] * input1[_44];
  work[_21] =
      input0[_21] * input1[_11] + input0[_22] * input1[_21] + input0[_23] * input1[_31] + input0[_24] * input1[_41];
  work[_22] =
      input0[_21] * input1[_12] + input0[_22] * input1[_22] + input0[_23] * input1[_32] + input0[_24] * input1[_42];
  work[_23] =
      input0[_21] * input1[_13] + input0[_22] * input1[_23] + input0[_23] * input1[_33] + input0[_24] * input1[_43];
  work[_24] =
      input0[_21] * input1[_14] + input0[_22] * input1[_24] + input0[_23] * input1[_34] + input0[_24] * input1[_44];
  work[_31] =
      input0[_31] * input1[_11] + input0[_32] * input1[_21] + input0[_33] * input1[_31] + input0[_34] * input1[_41];
  work[_32] =
      input0[_31] * input1[_12] + input0[_32] * input1[_22] + input0[_33] * input1[_32] + input0[_34] * input1[_42];
  work[_33] =
      input0[_31] * input1[_13] + input0[_32] * input1[_23] + input0[_33] * input1[_33] + input0[_34] * input1[_43];
  work[_34] =
      input0[_31] * input1[_14] + input0[_32] * input1[_24] + input0[_33] * input1[_34] + input0[_34] * input1[_44];
  work[_41] =
      input0[_41] * input1[_11] + input0[_42] * input1[_21] + input0[_43] * input1[_31] + input0[_44] * input1[_41];
  work[_42] =
      input0[_41] * input1[_12] + input0[_42] * input1[_22] + input0[_43] * input1[_32] + input0[_44] * input1[_42];
  work[_43] =
      input0[_41] * input1[_13] + input0[_42] * input1[_23] + input0[_43] * input1[_33] + input0[_44] * input1[_43];
  work[_44] =
      input0[_41] * input1[_14] + input0[_42] * input1[_24] + input0[_43] * input1[_34] + input0[_44] * input1[_44];

  // Output the result.
  matrix_copy(output, work);
}

void matrix_rotate(MATRIX output, const MATRIX input0, const VECTOR input1) {
  MATRIX work;

  // Apply the z-axis rotation.
  matrix_unit(work);
  work[_11] = cosf(input1[2]);
  work[_12] = sinf(input1[2]);
  work[_21] = -sinf(input1[2]);
  work[_22] = cosf(input1[2]);
  matrix_multiply(output, input0, work);

  // Apply the y-axis rotation.
  matrix_unit(work);
  work[_11] = cosf(input1[1]);
  work[_13] = -sinf(input1[1]);
  work[_31] = sinf(input1[1]);
  work[_33] = cosf(input1[1]);
  matrix_multiply(output, output, work);

  // Apply the x-axis rotation.
  matrix_unit(work);
  work[_22] = cosf(input1[0]);
  work[_23] = sinf(input1[0]);
  work[_32] = -sinf(input1[0]);
  work[_33] = cosf(input1[0]);
  matrix_multiply(output, output, work);
}

void matrix_scale(MATRIX output, const MATRIX input0, const VECTOR input1) {
  MATRIX work;

  // Apply the scaling.
  matrix_unit(work);
  work[_11] = input1[_X];
  work[_22] = input1[_Y];
  work[_33] = input1[_Z];
  matrix_multiply(output, input0, work);
}

void matrix_translate(MATRIX output, const MATRIX input0, const VECTOR input1) {
  MATRIX work;

  // Apply the translation.
  matrix_unit(work);
  work[_41] = input1[_X];
  work[_42] = input1[_Y];
  work[_43] = input1[_Z];
  matrix_multiply(output, input0, work);
}

void matrix_transpose(MATRIX output, const MATRIX input0) {
  MATRIX work;

  // Transpose the matrix.
  work[_11] = input0[_11];
  work[_12] = input0[_21];
  work[_13] = input0[_31];
  work[_14] = input0[_41];
  work[_21] = input0[_12];
  work[_22] = input0[_22];
  work[_23] = input0[_32];
  work[_24] = input0[_42];
  work[_31] = input0[_13];
  work[_32] = input0[_23];
  work[_33] = input0[_33];
  work[_34] = input0[_43];
  work[_41] = input0[_14];
  work[_42] = input0[_24];
  work[_43] = input0[_34];
  work[_44] = input0[_44];

  // Output the result.
  matrix_copy(output, work);
}

void matrix_unit(MATRIX output) {
  // Create a unit matrix.
  memset(output, 0, sizeof(MATRIX));
  output[_11] = 1.00f;
  output[_22] = 1.00f;
  output[_33] = 1.00f;
  output[_44] = 1.00f;
}

// creation functions

void create_local_world(MATRIX local_world, VECTOR translation, VECTOR rotation) {
  // Create the local_world matrix.
  matrix_unit(local_world);
  matrix_rotate(local_world, local_world, rotation);
  matrix_translate(local_world, local_world, translation);
}

void create_local_light(MATRIX local_light, VECTOR rotation) {
  // Create the local_light matrix.
  matrix_unit(local_light);
  matrix_rotate(local_light, local_light, rotation);
}

void create_world_view(MATRIX world_view, const VECTOR translation, const VECTOR rotation) {
  VECTOR work0, work1;

  // Reverse the translation.
  work0[_X] = -translation[_X];
  work0[_Y] = -translation[_Y];
  work0[_Z] = -translation[_Z];
  work0[_W] = translation[_W];

  // Reverse the rotation.
  work1[_X] = -rotation[_X];
  work1[_Y] = -rotation[_Y];
  work1[_Z] = -rotation[_Z];
  work1[_W] = rotation[_W];

  // Create the world_view matrix.
  matrix_unit(world_view);
  matrix_translate(world_view, world_view, work0);
  matrix_rotate(world_view, world_view, work1);
}

void create_view_screen(MATRIX view_screen, float aspect, float left, float right, float bottom, float top, float near,
                        float far) {
  /* We want to create a matrix that transforms
     field of view frustum (a truncated pyramid)
     into a normalized cuboid (for fast hardware clipping):
      w,  0,  0,          0,
      0,  -h, 0,          0,
      0,  0,  (f+n) / (f-n),      -1,
      0,  0,  (2*f*n) / (f-n),    0
     (w:kFramebufferWidth,h:kFramebufferHeight,n:z near,f:z far)
  */

  // Apply the aspect ratio adjustment.
  left = (left * aspect);
  right = (right * aspect);

  // Create the view_screen matrix.
  /*  matrix_unit(view_screen);
      view_screen[_11] = (2 * near) / (right - left);
      view_screen[_22] = (2 * near) / (top - bottom);
      view_screen[_31] = (right + left) / (right - left);
      view_screen[_32] = (top + bottom) / (top - bottom);
      view_screen[_33] = (far + near) / (far - near);
      view_screen[_34] = -1.00f;
      view_screen[_43] = (2 * far * near) / (far - near);
      view_screen[_44] = 0.00f;

      //This is good for ps2 clipping, where pixel is considered visible if:
      //-w < x < w
      //-w < y < w
      //-w < z < w
      //It's not automatic, it's done by using 'clipw' and testing flags in vu1
     code
      //Result of the test allows to exclude entire triangle
  */

  // For xbox1 clipping, pixel is considered visible if:
  //-w < x < w
  //-w < y < w
  // 0 < z < w
  // It's automatic and verified for each pixel before pixel shader is called

  // so we need this :
  matrix_unit(view_screen);
  view_screen[_11] = (2 * near) / (right - left);
  view_screen[_22] = (2 * near) / (top - bottom);
  view_screen[_31] = -(right + left) / (right - left);
  view_screen[_32] = -(top + bottom) / (top - bottom);
  view_screen[_33] = -far / (far - near);
  view_screen[_34] = -1.00f;
  view_screen[_43] = near * far / (far - near);
  view_screen[_44] = 0.00f;
}

void create_local_screen(MATRIX local_screen, MATRIX local_world, MATRIX world_view, MATRIX view_screen) {
  // Create the local_screen matrix.
  matrix_unit(local_screen);
  matrix_multiply(local_screen, local_screen, local_world);
  matrix_multiply(local_screen, local_screen, world_view);
  matrix_multiply(local_screen, local_screen, view_screen);
}

void create_d3d_look_at_lh(MATRIX ret, const VECTOR eye, const VECTOR at, const VECTOR up) {
  // https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
  VECTOR z_axis;
  vector_subtract(z_axis, at, eye);
  vector_normalize(z_axis);

  VECTOR x_axis;
  vector_crossproduct(x_axis, up, z_axis);
  vector_normalize(x_axis);

  VECTOR y_axis;
  vector_crossproduct(y_axis, z_axis, x_axis);
  vector_normalize(y_axis);

  ret[_11] = x_axis[_X];
  ret[_12] = y_axis[_X];
  ret[_13] = z_axis[_X];
  ret[_14] = 0.0f;

  ret[_21] = x_axis[_Y];
  ret[_22] = y_axis[_Y];
  ret[_23] = z_axis[_Y];
  ret[_24] = 0.0f;

  ret[_31] = x_axis[_Z];
  ret[_32] = y_axis[_Z];
  ret[_33] = z_axis[_Z];
  ret[_34] = 0.0f;

  ret[_41] = -1.0f * vector_dot(x_axis, eye);
  ret[_42] = -1.0f * vector_dot(y_axis, eye);
  ret[_43] = -1.0f * vector_dot(z_axis, eye);
  ret[_44] = 1.0f;
}

void create_d3d_perspective_fov_lh(MATRIX ret, float fov_y, float aspect, float z_near, float z_far) {
  float y_scale = 1.0f / tanf(fov_y * 0.5f);
  float x_scale = y_scale / aspect;

  float z_adjustment = z_far / (z_far - z_near);

  matrix_unit(ret);
  ret[_11] = x_scale;
  ret[_22] = y_scale;
  ret[_33] = z_adjustment;
  ret[_34] = 1.0f;
  ret[_43] = -1.0f * z_near * z_adjustment;
  ret[_44] = 0.0f;
}

void create_d3d_viewport(MATRIX ret, float width, float height, float max_depthbuffer_value, float z_min, float z_max) {
  matrix_unit(ret);

  ret[_11] = width * 0.5f;
  ret[_41] = ret[_11];
  ret[_42] = height * 0.5f;
  ret[_22] = -1.0f * ret[_42];
  ret[_33] = max_depthbuffer_value * (z_max - z_min);
  ret[_43] = max_depthbuffer_value * z_min;
}

void create_d3d_standard_viewport_16(MATRIX ret, float width, float height) {
  create_d3d_viewport(ret, width, height, (float)0xFFFF, 0.0f, 1.0f);
}

void create_d3d_standard_viewport_24(MATRIX ret, float width, float height) {
  create_d3d_viewport(ret, width, height, (float)0x00FFFFFF, 0.0f, 1.0f);
}
