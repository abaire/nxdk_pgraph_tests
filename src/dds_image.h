#ifndef NXDK_PGRAPH_TESTS_DDS_IMAGE_H
#define NXDK_PGRAPH_TESTS_DDS_IMAGE_H

#include <cstdint>
#include <memory>
#include <vector>

class DDSImage {
 public:
  struct SubImage {
    enum class Format { NONE, DXT1, DXT3, DXT5 };

    uint32_t level;
    Format format;
    uint32_t width;
    uint32_t height;
    uint32_t compressed_width;
    uint32_t compressed_height;
    uint32_t depth;
    uint32_t pitch;
    uint32_t bytes_per_pixel;
    std::vector<uint8_t> data;
  };

 public:
  DDSImage() = default;

  bool LoadFile(const char *filename, bool load_mipmaps = false);

  inline std::shared_ptr<SubImage> GetPrimaryImage() const { return GetSubImage(0); }
  std::shared_ptr<SubImage> GetSubImage(uint32_t mipmap_level = 0) const;
  const std::vector<std::shared_ptr<SubImage>> &GetSubImages() const { return sub_images_; }

  uint32_t NumLevels() const { return sub_images_.size(); }

 private:
  bool loaded_{false};
  std::vector<std::shared_ptr<SubImage>> sub_images_{};
};

#endif  // NXDK_PGRAPH_TESTS_DDS_IMAGE_H
