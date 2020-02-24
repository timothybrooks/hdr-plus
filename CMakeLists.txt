cmake_minimum_required(VERSION 3.10)
project(HDR_PLUS)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})
include("macro.cmake")
set(CMAKE_CXX_STANDARD 11)

# Define dependencies
link_halide()
link_libtiff()

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_library(LIBRAW_LIBRARY NAMES raw raw_r)

set(src_files
    src/align.cpp
    src/finish.cpp
    src/merge.cpp
    src/util.cpp
    src/InputSource.cpp
    src/Burst.cpp
    src/LibRaw2DngConverter.cpp)

set(header_files
    src/InputSource.h
    src/Burst.h
    src/dngwriter.h
    src/LibRaw2DngConverter.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${HALIDE_DISTRIB_DIR}/include ${HALIDE_DISTRIB_DIR}/tools ${RAW2DNG_INCLUDE_DIRS})

add_executable(hdrplus bin/HDRPlus.cpp ${src_files})
target_link_libraries(hdrplus Halide png ${LIBRAW_LIBRARY} ${TIFF_LIBRARIES} ${TIFFXX_LIBRARY})

add_executable(stack_frames bin/stack_frames.cpp ${src_files})
target_link_libraries(stack_frames Halide ${LIBRAW_LIBRARY} ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${TIFF_LIBRARIES} ${TIFFXX_LIBRARY})
