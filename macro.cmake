cmake_minimum_required(VERSION 3.10)

macro(link_halide)
    if("${HALIDE_DISTRIB_DIR}" STREQUAL " ")
        message(FATAL_ERROR "Specify HALIDE_DISTRIB_DIR variable in the cmake options.")
    endif()
    find_package(Threads) # fix dynamic linking for halide
    set(HALIDE_DISTRIB_USE_STATIC_LIBRARY OFF)
    include(${HALIDE_DISTRIB_DIR}/halide.cmake)
    include_directories(${HALIDE_DISTRIB_DIR}/include ${HALIDE_DISTRIB_DIR}/tools)
    link_directories(${HALIDE_DISTRIB_DIR}/lib ${HALIDE_DISTRIB_DIR}/bin)
endmacro()

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

macro(link_exiv2)
    find_package(Exiv2 REQUIRED)
    include_directories(${EXIV2_INCLUDE_DIR})
endmacro()
