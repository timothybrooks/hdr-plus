include(ExternalProject)

ExternalProject_Add(raw2dng
    PREFIX ${HIDAPI_ROOT}
    GIT_REPOSITORY  "https://github.com/Fimagena/raw2dng"
    GIT_TAG         "master"
    UPDATE_COMMAND  ""
    PATCH_COMMAND   ""
    SOURCE_SUBDIR   "libdng"
    )

ExternalProject_Get_Property(raw2dng install_dir)
add_library(dng_sdk STATIC IMPORTED)

set_property(TARGET dng_sdk PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libluajit-5.1.a)

add_dependencies(dng_sdk raw2dng)

