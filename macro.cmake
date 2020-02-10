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
endmacro()

macro(link_exiv2)
    find_package(Exiv2 REQUIRED)
    include_directories(${EXIV2_INCLUDE_DIR})
endmacro()

macro(add_raw2dng)
    include(ExternalProject)
    externalproject_add(raw2dng
        CONFIGURE_COMMAND ${CMAKE_COMMAND} ${CMAKE_CURRENT_BINARY_DIR}/raw2dng_repo
        PREFIX           raw2dng
        GIT_REPOSITORY   https://github.com/brotherofken/raw2dng.git
        GIT_TAG          raw2dng_lib
        UPDATE_COMMAND   ""
        SOURCE_DIR       ${CMAKE_CURRENT_BINARY_DIR}/raw2dng_repo
        CMAKE_ARGS       -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        INSTALL_COMMAND  ""
        )
    ExternalProject_Get_property(raw2dng SOURCE_DIR)
    set(RAW2DNG_SOURCE_DIR ${SOURCE_DIR})
    message(STATUS "Source dir of RAW2DNG_SOURCE_DIR = ${RAW2DNG_SOURCE_DIR}")

    ExternalProject_Get_property(raw2dng BINARY_DIR)
    set(RAW2DNG_BINARY_DIR ${BINARY_DIR})
    message(STATUS "Source dir of RAW2DNG_BINARY_DIR = ${RAW2DNG_BINARY_DIR}")

    find_package(EXPAT REQUIRED)

    set(RAW2DNG_LIBRARIES raw2dng_lib dng-sdk xmp-sdk md5 dng ${EXPAT_LIBRARIES})
    set(RAW2DNG_INCLUDE_DIRS ${RAW2DNG_SOURCE_DIR})
    set(RAW2DNG_LINK_DIRS
        ${RAW2DNG_BINARY_DIR}/raw2dng
        ${RAW2DNG_BINARY_DIR}/libdng
        ${RAW2DNG_BINARY_DIR}/libdng/dng-sdk
        ${RAW2DNG_BINARY_DIR}/libdng/xmp-sdk
        ${RAW2DNG_BINARY_DIR}/libdng/md5)
    link_directories(${RAW2DNG_LINK_DIRS})
    message("${RAW2DNG_LINK_DIRS}")
endmacro()
