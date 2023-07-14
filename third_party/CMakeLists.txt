# Fast png compression.
add_library(
        fpng
        EXCLUDE_FROM_ALL
        fpng/src/fpng.cpp
        fpng/src/fpng.h
)
target_compile_options(
        fpng
        PRIVATE
        -O3
        -Wno-everything
        -DFPNG_NO_STDIO=1
        -DFPNG_NO_SSE=1
)
target_link_options(fpng PRIVATE "/debug:none")

# Full featured printf.
add_library(
        printf
        EXCLUDE_FROM_ALL
        printf/printf.c
        printf/printf.h
)
target_compile_options(
        printf
        PRIVATE
        -O3
        -Wno-everything
)
target_link_options(printf PRIVATE "/debug:none")

# Texture swizzling routines.
add_library(
        swizzle
        EXCLUDE_FROM_ALL
        swizzle.c
        swizzle.h
)
target_compile_options(
        swizzle
        PRIVATE
        -O3
        -Wno-everything
)
target_link_options(swizzle PRIVATE "/debug:none")
