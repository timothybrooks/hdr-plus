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
    src/InputSource.cpp
    src/Burst.cpp
    src/LibRaw2DngConverter.cpp)

set(header_files
    src/InputSource.h
    src/Burst.h
    src/dngwriter.h
    src/LibRaw2DngConverter.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${HALIDE_DISTRIB_DIR}/include ${HALIDE_DISTRIB_DIR}/tools ${RAW2DNG_INCLUDE_DIRS})
include_directories(${CMAKE_BINARY_DIR}/genfiles)

set(HALIDE_TARGET "")
set(HALIDE_TARGET_FEATURES "")
halide_library(hdrplus_pipeline
    SRCS src/hdrplus_pipeline_generator.cpp src/align.cpp src/merge.cpp src/finish.cpp src/util.cpp
    # GENERATOR_DEPS  # We don't have any yet
    GENERATOR_NAME hdrplus_pipeline
    # GENERATOR_ARGS  # We don't have any yet
    FUNCTION_NAME hdrplus_pipeline
    # HALIDE_TARGET ${HALIDE_TARGET}  # TODO: add option with custom HALIDE_TARGET
    # HALIDE_TARGET_FEATURES ${HALIDE_TARGET_FEATURES}  # TODO: add option with custom HALIDE_TARGET
    # EXTRA_OUTPUTS "stmt;html;schedule") # uncomment for extra output
)

halide_library(align_and_merge
    SRCS src/align_and_merge_generator.cpp src/align.cpp src/merge.cpp src/util.cpp
    GENERATOR_NAME align_and_merge
    FUNCTION_NAME align_and_merge
    # HALIDE_TARGET ${HALIDE_TARGET}  # TODO: add option with custom HALIDE_TARGET
    # HALIDE_TARGET_FEATURES ${HALIDE_TARGET_FEATURES}  # TODO: add option with custom HALIDE_TARGET
    # EXTRA_OUTPUTS "stmt;html;schedule") # uncomment for extra output
)


add_executable(hdrplus bin/HDRPlus.cpp ${src_files})
add_dependencies(hdrplus hdrplus_pipeline)
target_link_libraries(hdrplus hdrplus_pipeline Halide png ${LIBRAW_LIBRARY} ${TIFF_LIBRARIES} ${TIFFXX_LIBRARY})

add_executable(stack_frames bin/stack_frames.cpp ${src_files})
add_dependencies(stack_frames align_and_merge)
target_link_libraries(stack_frames Halide align_and_merge ${LIBRAW_LIBRARY} ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${TIFF_LIBRARIES} ${TIFFXX_LIBRARY})
