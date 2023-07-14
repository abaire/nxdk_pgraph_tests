if (NOT TARGET NXDK::SDL2_Image)

    add_library(nxdk_sdl2_image STATIC IMPORTED)
    set_target_properties(
            nxdk_sdl2_image
            PROPERTIES
            IMPORTED_LOCATION "${NXDK_DIR}/lib/libSDL2_image.lib"
    )

    add_library(nxdk_jpeg STATIC IMPORTED)
    set_target_properties(
            nxdk_jpeg
            PROPERTIES
            IMPORTED_LOCATION "${NXDK_DIR}/lib/libjpeg.lib"
    )

    add_library(nxdk_png STATIC IMPORTED)
    set_target_properties(
            nxdk_png
            PROPERTIES
            IMPORTED_LOCATION "${NXDK_DIR}/lib/libpng.lib"
    )

    add_library(NXDK::SDL2_Image INTERFACE IMPORTED)
    target_link_libraries(
            NXDK::SDL2_Image
            INTERFACE
            nxdk_sdl2_image
            nxdk_jpeg
            nxdk_png
    )
    target_include_directories(
            NXDK::SDL2_Image
            SYSTEM INTERFACE
            "${NXDK_DIR}/lib/sdl/SDL2_image"
    )
endif ()
