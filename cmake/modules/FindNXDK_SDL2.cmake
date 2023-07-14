if(NOT TARGET NXDK::SDL2)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

    add_library(NXDK::SDL2 ALIAS PkgConfig::SDL2)
endif()
