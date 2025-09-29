#ifndef NXDK_PGRAPH_TESTS_TESTS_HOST_STUBS_NV2ASTATE_H_
#define NXDK_PGRAPH_TESTS_TESTS_HOST_STUBS_NV2ASTATE_H_

typedef int SDL_PixelFormatEnum;

namespace PBKitPlusPlus {

class NV2AState {
 public:
  NV2AState() = default;
  virtual ~NV2AState() = default;

  [[nodiscard]] float GetFramebufferWidthF() const { return 640.f; }
  [[nodiscard]] float GetFramebufferHeightF() const { return 480.f; }
};

}  // namespace PBKitPlusPlus

#endif  // NXDK_PGRAPH_TESTS_TESTS_HOST_STUBS_NV2ASTATE_H_
