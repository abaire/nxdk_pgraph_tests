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
if (NOT NO_OPT)
    target_compile_options(
            swizzle
            PRIVATE
            -O3
            -Wno-everything
    )
    target_link_options(swizzle PRIVATE "/debug:none")
endif ()

# 3D math routines
add_library(
        xbox_math3d
        EXCLUDE_FROM_ALL
        xbox_math3d/src/xbox_math_d3d.cpp
        xbox_math3d/src/xbox_math_d3d.h
        xbox_math3d/src/xbox_math_matrix.cpp
        xbox_math3d/src/xbox_math_matrix.h
        xbox_math3d/src/xbox_math_types.cpp
        xbox_math3d/src/xbox_math_types.h
        xbox_math3d/src/xbox_math_vector.cpp
        xbox_math3d/src/xbox_math_vector.h
)
target_include_directories(
        xbox_math3d
        INTERFACE
        xbox_math3d/src
)
if (NOT NO_OPT)
    target_compile_options(
            xbox_math3d
            PRIVATE
            -O3
            -Wno-everything
    )
    target_link_options(xbox_math3d PRIVATE "/debug:none")
endif ()
