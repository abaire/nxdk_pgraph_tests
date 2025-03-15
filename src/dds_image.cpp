// Partial implementation of DDS file loading.
//
// See https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide
// Use nvcompress from https://developer.nvidia.com/gpu-accelerated-texture-compression to create files

#include "dds_image.h"

#include <cstdio>

#include "debug_output.h"

#define FOURCC(a, b, c, d) (((a) & 0xFF) | (((b) & 0xFF) << 8) | (((c) & 0xFF) << 16) | (((d) & 0xFF) << 24))

constexpr uint32_t kDDSMagic = FOURCC('D', 'D', 'S', ' ');
constexpr uint32_t kDX10Magic = FOURCC('D', 'X', '1', '0');

static const uint32_t kDXT1Magic = FOURCC('D', 'X', 'T', '1');
// static const uint32_t kDXT2Magic = FOURCC('D', 'X', 'T', '2');
static const uint32_t kDXT3Magic = FOURCC('D', 'X', 'T', '3');
// static const uint32_t kDXT4Magic = FOURCC('D', 'X', 'T', '4');
static const uint32_t kDXT5Magic = FOURCC('D', 'X', 'T', '5');

enum DDH_FLAGS {
  DDSD_CAPS = 0x1,
  DDSD_HEIGHT = 0x2,
  DDSD_WIDTH = 0x4,
  DDSD_PITCH = 0x8,
  DDSD_PIXELFORMAT = 0x1000,
  DDSD_MIPMAPCOUNT = 0x20000,
  DDSD_LINEARSIZE = 0x80000,
  DDSD_DEPTH = 0x800000,
};

enum DDH_CAPS {
  DDSCAPS_COMPLEX = 0x08,
  DDSCAPS_TEXTURE = 0x1000,
  DDSCAPS_MIPMAP = 0x400000,
};

enum DDH_CAPS2 {
  DDSCAPS2_CUBEMAP = 0x200,
  DDSCAPS2_CUBEMAP_POSITIVEX = 0x400,
  DDSCAPS2_CUBEMAP_NEGATIVEX = 0x800,
  DDSCAPS2_CUBEMAP_POSITIVEY = 0x1000,
  DDSCAPS2_CUBEMAP_NEGATIVEY = 0x2000,
  DDSCAPS2_CUBEMAP_POSITIVEZ = 0x4000,
  DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x8000,
  DDSCAPS2_VOLUME = 0x200000,
};

enum DDPF_FLAGS {
  DDPF_ALPHAPIXELS = 0x1,
  DDPF_ALPHA = 0x2,
  DDPF_FOURCC = 0x4,
  DDPF_RGB = 0x40,
  DDPF_YUV = 0x200,
  DDPF_LUMINANCE = 0x20000,
};

#pragma pack(push)
#pragma pack(1)
struct DDS_PIXELFORMAT {
  uint32_t dwSize;
  uint32_t dwFlags;
  uint32_t dwFourCC;
  uint32_t dwRGBBitCount;
  uint32_t dwRBitMask;
  uint32_t dwGBitMask;
  uint32_t dwBBitMask;
  uint32_t dwABitMask;
};

struct DDS_HEADER {
  uint32_t dwSize;
  uint32_t dwFlags;
  uint32_t dwHeight;
  uint32_t dwWidth;
  uint32_t dwPitchOrLinearSize;
  uint32_t dwDepth;
  uint32_t dwMipMapCount;
  uint32_t dwReserved1[11];
  DDS_PIXELFORMAT ddspf;
  uint32_t dwCaps;
  uint32_t dwCaps2;
  uint32_t dwCaps3;
  uint32_t dwCaps4;
  uint32_t dwReserved2;
};

struct DDS_FILE {
  uint32_t dwMagic;
  DDS_HEADER header;
};
#pragma pack(pop)

static inline uint32_t compressed_size(uint32_t uncompressed_size) {
  uint32_t ret = (uncompressed_size + 3) / 4;
  if (ret < 1) {
    return 1;
  }
  return ret;
}

bool DDSImage::LoadFile(const char *filename, bool load_mipmaps) {
  FILE *f = fopen(filename, "rb");
  if (!f) {
    return false;
  }

  DDS_FILE file{};
  if (fread(&file, sizeof(file), 1, f) != 1) {
    fclose(f);
    return false;
  }

  ASSERT(file.dwMagic == kDDSMagic && "Invalid DDS file - bad fourcc");
  if (file.header.dwDepth == 0) {
    file.header.dwDepth = 1;
  }

  const DDS_HEADER &header = file.header;
  ASSERT(header.dwSize == 124);
  ASSERT(header.dwCaps2 == 0 && "Multidimensional textures not supported.");

  const DDS_PIXELFORMAT &pixelformat = header.ddspf;

  if (pixelformat.dwFlags & DDPF_FOURCC) {
    ASSERT((pixelformat.dwFourCC != kDX10Magic) && "DX10 format not supported.");

    uint32_t block_size = 0;
    SubImage::Format format = SubImage::Format::NONE;
    uint32_t bytes_per_pixel = 0;
    uint32_t compressed_width = compressed_size(header.dwWidth);
    uint32_t compressed_height = compressed_size(header.dwHeight);

    switch (pixelformat.dwFourCC) {
      case kDXT1Magic:
        block_size = 8;
        bytes_per_pixel = 3;
        format = SubImage::Format::DXT1;
        break;

      case kDXT3Magic:
        block_size = 16;
        bytes_per_pixel = 4;
        format = SubImage::Format::DXT3;
        break;

      case kDXT5Magic:
        block_size = 16;
        bytes_per_pixel = 4;
        format = SubImage::Format::DXT5;
        break;

      default:
        PrintMsg("Unsupported FourCC format: 0x%X\n", pixelformat.dwFourCC);
        ASSERT(!"Unsupported FourCC format");
        fclose(f);
        return false;
    }

    auto read_image = [block_size, format, &f, bytes_per_pixel](
                          uint32_t width, uint32_t height, uint32_t depth, uint32_t compressed_width,
                          uint32_t compressed_height) -> std::shared_ptr<SubImage> {
      uint32_t pitch = compressed_width * block_size;
      uint32_t compressed_bytes = pitch * compressed_height * depth;

      std::shared_ptr<SubImage> ret = std::make_shared<SubImage>();
      ret->level = 0;
      ret->format = format;
      ret->width = width;
      ret->height = height;
      ret->compressed_width = compressed_width;
      ret->compressed_height = compressed_height;
      ret->depth = depth;
      ret->pitch = pitch;
      ret->bytes_per_pixel = bytes_per_pixel;

      ret->data.resize(compressed_bytes);
      if (fread(ret->data.data(), compressed_bytes, 1, f) != 1) {
        return {};
      }

      return ret;
    };

    auto primary_image =
        read_image(header.dwWidth, header.dwHeight, header.dwDepth, compressed_width, compressed_height);
    if (!primary_image) {
      ASSERT(!"Failed to read image data.");
      fclose(f);
      return false;
    }
    sub_images_.push_back(primary_image);

    if (load_mipmaps) {
      uint32_t width = header.dwWidth;
      uint32_t height = header.dwHeight;

      for (auto i = 1; i < header.dwMipMapCount; ++i) {
        width >>= 1;
        if (width < 1) {
          width = 1;
        }
        height >>= 1;
        if (height < 1) {
          height = 1;
        }

        auto subimage = read_image(width, height, header.dwDepth, compressed_size(width), compressed_size(height));
        if (!subimage) {
          ASSERT(!"Failed to read mipmap data.");
          fclose(f);
          return false;
        }

        sub_images_.push_back(subimage);
      }
    }
  } else {
    ASSERT((header.dwFlags & DDSD_PITCH) && (pixelformat.dwFlags & DDPF_RGB) && "Texture not flagged as uncompressed");
    // TODO: Implement if necessary.
    ASSERT(!"Uncompressed texture formats not implemented");
  }

  fclose(f);
  loaded_ = true;
  return true;
}

std::shared_ptr<DDSImage::SubImage> DDSImage::GetSubImage(uint32_t mipmap_level) const {
  ASSERT(mipmap_level < sub_images_.size() && "Mipmap level > number of loaded mipmaps");
  if (mipmap_level < sub_images_.size()) {
    return sub_images_[mipmap_level];
  }

  return {};
}
