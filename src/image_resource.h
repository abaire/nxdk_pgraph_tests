#ifndef NXDK_PGRAPH_TESTS_SRC_IMAGERESOURCE_H_
#define NXDK_PGRAPH_TESTS_SRC_IMAGERESOURCE_H_

#include <cstdint>
#include <memory>

struct ImageResource {
  uint8_t *data{nullptr};
  uint32_t width{0};
  uint32_t height{0};
  uint32_t pitch{0};

  ImageResource() = default;

  explicit ImageResource(const std::string &source_path) { LoadPNG(source_path); }

  virtual ~ImageResource();

  //! Loads a PNG file from the filesystem.
  //!
  //! source_path - Windows style path to the PNG file to load (e.g., "D:\\image_blit\\TestImage.png")
  void LoadPNG(const std::string &source_path);

  //! Copies the image data to the given target, which must be allocated and sufficiently large.
  void CopyTo(uint8_t *target) const;
};

#endif  // NXDK_PGRAPH_TESTS_SRC_IMAGERESOURCE_H_
