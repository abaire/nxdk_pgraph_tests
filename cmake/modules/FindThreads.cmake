if(NOT TARGET Threads::Threads)
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(Threads_FOUND TRUE)
    add_library(Threads::Threads INTERFACE IMPORTED)
endif()
