cmake_minimum_required(VERSION 3.10)
project(HDR_PLUS)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include("macro.cmake")
set(CMAKE_CXX_STANDARD 11)

# Define dependencies
link_halide()
link_libtiff()

find_package(PNG REQUIRED)
find_library(LIBRAW_LIBRARY
    NAMES raw raw_r
    )

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

include_directories(/usr/local/lib /usr/local/include ${HALIDE_DISTRIB_DIR}/include ${HALIDE_DISTRIB_DIR}/tools)
link_directories(/usr/local/lib /usr/local/include)

add_executable(stack_frames src/stack_frames.cpp ${src_files})
target_link_libraries(stack_frames Halide png ${LIBRAW_LIBRARY} ${TIFF_LIBRARIES})

add_executable(hdrplus src/HDRPlus.cpp ${src_files})
target_link_libraries(hdrplus Halide png ${LIBRAW_LIBRARY})
