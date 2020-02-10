cmake_minimum_required(VERSION 3.10)
project(HDR_PLUS)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})
include("macro.cmake")
set(CMAKE_CXX_STANDARD 11)

# Define dependencies
add_raw2dng()
link_halide()
link_libtiff()
link_exiv2()

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
    src/Burst.cpp)

set(header_files
    src/InputSource.h
    src/Burst.h
    src/dngwriter.h)

include_directories(${HALIDE_DISTRIB_DIR}/include ${HALIDE_DISTRIB_DIR}/tools ${RAW2DNG_INCLUDE_DIRS})

add_executable(hdrplus src/HDRPlus.cpp ${src_files})
target_link_libraries(hdrplus Halide png ${LIBRAW_LIBRARY}) # ${ZLIB_LIBRARIES})

add_executable(stack_frames src/stack_frames.cpp ${src_files})
target_link_libraries(stack_frames Halide ${LIBRAW_LIBRARY} ${PNG_LIBRARIES} ${TIFF_LIBRARIES} ${JPEG_LIBRARIES} ${EXIV2_LIBRARIES} ${RAW2DNG_LIBRARIES} ${ZLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

#set(RAW2DNG_LIBRARIES raw2dng_lib)
#set(RAW2DNG_INCLUDE_DIRS ${RAW2DNG_SOURCE_DIR})
#set(RAW2DNG_LINK_DIRS ${RAW2DNG_BINARY_DIR}/raw2dng)
