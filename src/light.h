#ifndef LIGHT_H
#define LIGHT_H

#include <cstdint>
#include <cstring>

#include "xbox_math_vector.h"
class TestHost;

/**
 * Provides functionality to set up a hardware light source.
 */
class Light {
 public:
  virtual void Commit(TestHost& host) const;

  /**
   * Sets up nv2a registers for this Light.
   * @param host TestHost used to render this Light
   * @param look_direction Normalized forward vector of the camera
   */
  virtual void Commit(TestHost& host, const XboxMath::vector_t& look_direction) const;

  [[nodiscard]] uint32_t light_enable_mask() const { return light_enable_mask_; }

  void SetAmbient(const float* ambient) { memcpy(ambient_, ambient, sizeof(ambient_)); }

  void SetAmbient(float r, float g, float b) {
    ambient_[0] = r;
    ambient_[1] = g;
    ambient_[2] = b;
  }

  void SetDiffuse(const float* diffuse) { memcpy(diffuse_, diffuse, sizeof(diffuse_)); }

  void SetDiffuse(float r, float g, float b) {
    diffuse_[0] = r;
    diffuse_[1] = g;
    diffuse_[2] = b;
  }

  void SetSpecular(const float* specular) { memcpy(specular_, specular, sizeof(specular_)); }

  void SetSpecular(float r, float g, float b) {
    specular_[0] = r;
    specular_[1] = g;
    specular_[2] = b;
  }

  void SetBackAmbient(const float* ambient) { memcpy(back_ambient_, ambient, sizeof(back_ambient_)); }

  void SetBackAmbient(float r, float g, float b) {
    back_ambient_[0] = r;
    back_ambient_[1] = g;
    back_ambient_[2] = b;
  }

  void SetBackDiffuse(const float* diffuse) { memcpy(back_diffuse_, diffuse, sizeof(back_diffuse_)); }

  void SetBackDiffuse(float r, float g, float b) {
    back_diffuse_[0] = r;
    back_diffuse_[1] = g;
    back_diffuse_[2] = b;
  }

  void SetBackSpecular(const float* specular) { memcpy(back_specular_, specular, sizeof(back_specular_)); }

  void SetBackSpecular(float r, float g, float b) {
    back_specular_[0] = r;
    back_specular_[1] = g;
    back_specular_[2] = b;
  }

 protected:
  //! Constructs a Light.
  //! @param light_index - The hardware light to configure.
  //! @param enable_mask - The unshifted NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_x value specifying the type of the light.
  Light(uint32_t light_index, uint32_t enable_mask);

  virtual ~Light() = default;

 protected:
  uint32_t light_index_;
  uint32_t light_enable_mask_;

  float ambient_[3]{0.f, 0.f, 0.f};
  float diffuse_[3]{1.f, 1.f, 1.f};
  float specular_[3]{0.f, 0.f, 0.f};
  float back_ambient_[3]{0.f, 0.f, 0.f};
  float back_diffuse_[3]{1.f, 1.f, 1.f};
  float back_specular_[3]{0.f, 0.f, 0.f};
};

/**
 * Sets up a spotlight.
 */
class Spotlight : public Light {
 public:
  enum class FalloffFactor {
    ZERO,
    ONE_HALF,
    ONE,
    TWO,
    TEN,
  };

 public:
  Spotlight() = delete;

  /**
   * Constructs a new Spotlight instance.
   * @param light_index Hardware light index
   * @param position World coordinates of the spotlight
   * @param direction World coordinate direction of the spotlight
   * @param range World coordinate maximum range of the light
   * @param phi Penumbra (outer cone) angle in degrees
   * @param theta Umbra (inner, fully illuminated cone) angle in degrees
   * @param attenuation_constant Constant attenuation factor
   * @param attenuation_linear Linear attenuation factor
   * @param attenuation_quadratic Quadratic attenuation factor
   * @param falloff_1 Raw nv2a falloff factor, 1st component
   * @param falloff_2 Raw nv2a falloff factor, 2nd component
   * @param falloff_3 Raw nv2a falloff factor, 3rd component
   */
  Spotlight(uint32_t light_index, const XboxMath::vector_t& position, const XboxMath::vector_t& direction, float range,
            float phi, float theta, float attenuation_constant, float attenuation_linear, float attenuation_quadratic,
            float falloff_1, float falloff_2, float falloff_3);

  /**
   * Constructs a new Spotlight instance.
   * @param light_index Hardware light index
   * @param position World coordinates of the spotlight
   * @param direction World coordinate direction of the spotlight
   * @param range World coordinate maximum range of the light
   * @param phi Penumbra (outer cone) angle in degrees
   * @param theta Umbra (inner, fully illuminated cone) angle in degrees
   * @param attenuation_constant Constant attenuation factor
   * @param attenuation_linear Linear attenuation factor
   * @param attenuation_quadratic Quadratic attenuation factor
   * @param falloff Falloff factor
   *
   * Attenuation = param1 + (param2 * d) + (param3 * d^2) where d = distance from light to vertex.
   */
  Spotlight(uint32_t light_index, const XboxMath::vector_t& position, const XboxMath::vector_t& direction, float range,
            float phi, float theta, float attenuation_constant, float attenuation_linear, float attenuation_quadratic,
            FalloffFactor falloff);

  void Commit(TestHost& host) const override;

 private:
  XboxMath::vector_t position_;

  XboxMath::vector_t direction_;

  float range_;

  //! penumbra (outer cone) angle in degrees.
  float phi_;

  //! umbra (inner cone) angle in degrees.
  float theta_;

  //! Attenuation: {constant, linear (distance), quadratic (distance^2)}
  float attenuation_[3];

  //! Raw nv2a falloff coefficients.
  float falloff_[3];
};

/**
 * Sets up a directional light.
 */
class DirectionalLight : public Light {
 public:
  DirectionalLight() = delete;

  /**
   * Constructs a new DirectionalLight instance.
   * @param light_index Hardware light index
   * @param direction World coordinate direction of the spotlight
   */
  DirectionalLight(uint32_t light_index, const XboxMath::vector_t& direction);

  //! This must not be called for DirectionalLight instances.
  void Commit(TestHost& host) const override;

  void Commit(TestHost& host, const XboxMath::vector_t& look_dir) const override;

 private:
  XboxMath::vector_t direction_;
};

/**
 * Sets up a point light source.
 */
class PointLight : public Light {
 public:
  PointLight() = delete;

  /**
   * Constructs a new PointLight instance.
   * @param light_index Hardware light index
   * @param position World coordinates of the spotlight
   * @param range World coordinate maximum range of the light
   * @param attenuation_constant Constant attenuation factor
   * @param attenuation_linear Linear attenuation factor
   * @param attenuation_quadratic Quadratic attenuation factor
   *
   * Attenuation = param1 + (param2 * d) + (param3 * d^2) where d = distance from light to vertex.
   */
  PointLight(uint32_t light_index, const XboxMath::vector_t& position, float range, float attenuation_constant,
             float attenuation_linear, float attenuation_quadratic);

  void Commit(TestHost& host) const override;

 private:
  XboxMath::vector_t position_;

  float range_;

  //! Attenuation: {constant, linear (distance), quadratic (distance^2)}
  float attenuation_[3];
};

#endif  // LIGHT_H
