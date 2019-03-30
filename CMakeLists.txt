cmake_minimum_required(VERSION 2.8.11)
project(HDR_PLUS)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-O3 -v -g -Wall")
find_package(PNG REQUIRED)

set(HALIDE_ROOT_DIR "")


if(" ${HALIDE_ROOT_DIR}" STREQUAL " ")
    message( FATAL_ERROR "Specify HALIDE_ROOT_DIR variable in the cmake file." )
endif()

set(LibRaw_DIR "")


set(src_files
    src/HDRPlus.cpp
    src/align.cpp
    src/finish.cpp
    src/merge.cpp
    src/util.cpp
    )

include_directories(/usr/local/lib /usr/local/include ${HALIDE_ROOT_DIR}/bin ${HALIDE_ROOT_DIR}/lib ${HALIDE_ROOT_DIR}/include ${HALIDE_ROOT_DIR}/tools)

link_directories(/usr/local/lib /usr/local/include ${HALIDE_ROOT_DIR}/bin ${HALIDE_ROOT_DIR}/lib ${HALIDE_ROOT_DIR}/include ${HALIDE_ROOT_DIR}/tools)


add_executable(hdrplus ${src_files})

target_link_libraries(hdrplus Halide png)
