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

# Higher level wrappers around pbkit
if (IS_TARGET_BUILD)
    add_subdirectory(pbkitplusplus)
endif ()

# tiny-json
add_library(
        tiny-json
        EXCLUDE_FROM_ALL
        tiny-json/tiny-json.c
        tiny-json/tiny-json.h
)
if (NOT NO_OPT)
    target_compile_options(
            tiny-json
            PRIVATE
            -O3
            -Wno-everything
    )
    target_link_options(tiny-json PRIVATE "/debug:none")
endif ()
