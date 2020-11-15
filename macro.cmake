cmake_minimum_required(VERSION 3.10)

macro(link_libtiff)
    # Link as follows:
    # target_link_libraries(TARGET ${TIFF_LIBRARIES})
    find_package(TIFF REQUIRED)
    if (TIFF_FOUND)
        include_directories(${TIFF_INCLUDE_DIRS})
    endif()

    find_library(TIFFXX_LIBRARY NAMES tiffxx)
    message(STATUS "Found tiffxx: ${TIFFXX_LIBRARY}")
endmacro()
