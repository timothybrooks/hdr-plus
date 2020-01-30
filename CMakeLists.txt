cmake_minimum_required(VERSION 3.10)
project(HDR_PLUS)

include("macro.cmake")
set(CMAKE_CXX_STANDARD 11)

# Define dependencies
link_halide()

find_package(PNG REQUIRED)
find_library(LIBRAW_LIBRARY
    NAMES raw raw_r
    )

set(src_files
    src/HDRPlus.cpp
    src/align.cpp
    src/finish.cpp
    src/merge.cpp
    src/util.cpp
    src/InputSource.cpp
    src/InputSource.h
    src/Burst.cpp
    src/Burst.h)

include_directories(/usr/local/lib /usr/local/include ${HALIDE_DISTRIB_DIR}/include ${HALIDE_DISTRIB_DIR}/tools)
link_directories(/usr/local/lib /usr/local/include)


add_executable(hdrplus ${src_files})

target_link_libraries(hdrplus Halide png ${LIBRAW_LIBRARY})
