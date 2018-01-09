cmake_minimum_required(VERSION 2.8.11)
project(HDR_PLUS)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-O3 -g -Wall")

set(HALIDE_ROOT_DIR "")

if(" ${HALIDE_ROOT_DIR}" STREQUAL " ")
    message( FATAL_ERROR "Specify HALIDE_ROOT_DIR variable in the cmake file." )
endif()

set(src_files
    src/HDRPlus.cpp
    src/align.cpp
    src/finish.cpp
    src/merge.cpp
    src/util.cpp
    )

include_directories(${HALIDE_ROOT_DIR}/include ${HALIDE_ROOT_DIR}/tools)
link_directories(${HALIDE_ROOT_DIR}/bin)

add_executable(hdrplus ${src_files})

target_link_libraries(hdrplus Halide png)
