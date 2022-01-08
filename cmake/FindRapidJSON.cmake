# FindRapidJSON.cmake
#
# Finds the rapidjson library
#
# This will define the following variables
#
#    RapidJSON_FOUND
#    RapidJSON_INCLUDE_DIRS
#
# and the following imported targets
#
#     RapidJSON

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_RapidJSON QUIET RapidJSON)
endif()

find_path(RapidJSON_INCLUDE_DIR
    NAMES rapidjson.h
    PATHS ${PC_RapidJSON_INCLUDE_DIRS}
    PATH_SUFFIXES rapidjson
)

set(RapidJSON_VERSION ${PC_RapidJSON_VERSION})

mark_as_advanced(RapidJSON_FOUND RapidJSON_INCLUDE_DIR RapidJSON_VERSION)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RapidJSON
    REQUIRED_VARS RapidJSON_INCLUDE_DIR
    VERSION_VAR RapidJSON_VERSION
)

if(RapidJSON_FOUND)
    set(RapidJSON_INCLUDE_DIRS ${RapidJSON_INCLUDE_DIR})
endif()

if(RapidJSON_FOUND AND NOT TARGET RapidJSON)
    add_library(RapidJSON INTERFACE IMPORTED)
    set_target_properties(RapidJSON PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${RapidJSON_INCLUDE_DIR}"
    )
endif()
