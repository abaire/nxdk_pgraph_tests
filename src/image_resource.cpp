#include "image_resource.h"

#include <SDL.h>
#include <SDL_image.h>
#include <pbkit/pbkit_dma.h>

#include "debug_output.h"
#include "xbox-swizzle/swizzle.h"

ImageResource::~ImageResource() {
  if (data) {
    MmFreeContiguousMemory(data);
  }
}

void ImageResource::LoadPNG(const std::string& source_path) {
  SDL_Surface* temp = IMG_Load(source_path.c_str());
  ASSERT(temp && "IMG_Load failed");
  SDL_Surface* test_image = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_BGRA32, 0);
  SDL_FreeSurface(temp);

  pitch = test_image->pitch;
  height = test_image->h;
  width = test_image->w;
  bytes_per_pixel = test_image->format->BytesPerPixel;

  uint32_t size = pitch * height;
  data =
      static_cast<uint8_t*>(MmAllocateContiguousMemoryEx(size, 0, MAXRAM, 0x1000, PAGE_WRITECOMBINE | PAGE_READWRITE));

  memcpy(data, test_image->pixels, size);
  SDL_FreeSurface(test_image);
}

void ImageResource::CopyTo(uint8_t* target) const { memcpy(target, data, pitch * height); }

void ImageResource::SwizzleTo(uint8_t* target) const {
  swizzle_rect(data, width, height, target, pitch, bytes_per_pixel);
}
