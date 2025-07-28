#
# FindJSONC.cmake
#
# Find the JSON-C library.
#
# The following variables can be set to influcence this module's behavior:
#   JSONC_PATH - Directory containing json-c
#
# On success this module defines following variables:
#   JSONC_FOUND - True if jscon-c has been found
#   JSONC_INCLUDE_DIRS - Include directories for jscon-c
#   JSONC_LIBRARIES - Libraries to link against jscon-c
#   JSONC_VERSION - jscon-c version
#   JSONC_VERSION_MAJOR - jscon-c major version
#   JSONC_VERSION_MINOR - jscon-c minor version
#   JSONC_VERSION_REVISION - jscon-c revision version
#
# This module also defines an imported target:
#   JSONC::JSONC          - The json-c library target
#
# This file is under Unlicense.
# Source: <https://gist.github.com/m-pasik>
# For full license text and more information, refer to: <https://unlicense.org>
#

# XXX: There's probably a bunch of situations where this won't work properly.

if(WIN32) # Windows
    find_path(JSONC_INCLUDE_DIR
        NAMES
            json-c/json.h
        HINTS
            "${JSONC_PATH}/include"
        PATHS
            "C:/json-c/include"
            "$ENV{PROGRAMFILES}/json-c/include"
        DOC
            "Path to the directory with json-c/json.h"
    )

    find_library(GLFW_LIBRARY
        NAMES
            json-c
        HINTS
            "${JSONC_PATH}/lib"
        PATHS
            "C:/json-c/lib"
            "$ENV{PROGRAMFILES}/json-c/lib"
        DOC
            "The json-c library"
    )
else() # Assume *NIX
    find_path(JSONC_INCLUDE_DIR
        NAMES
            json-c/json.h
        HINTS
            "${JSONC_PATH}/include"
        PATHS
            "/usr/include"
            "/usr/local/include"
        DOC
            "Path to the directory with json-c/json.h"
    )

    find_library(JSONC_LIBRARY
        NAMES
            json-c
        HINTS
            "${JSONC_PATH}/lib64"
            "${JSONC_PATH}/lib"
        PATHS
            "/usr/lib64"
            "/usr/lib"
            "/usr/local/lib64"
            "/usr/local/lib"
        DOC
            "The json-c library"
    )
endif()

if(EXISTS "${JSONC_INCLUDE_DIR}/json-c/json_c_version.h")
    file(READ "${JSONC_INCLUDE_DIR}/json-c/json_c_version.h" _contents)

    if (_contents)
        string(REGEX REPLACE ".*#define JSON_C_MAJOR_VERSION[ \t]+([0-9]+).*" "\\1" JSONC_VERSION_MAJOR "${_contents}")
        string(REGEX REPLACE ".*#define JSON_C_MINOR_VERSION[ \t]+([0-9]+).*" "\\1" JSONC_VERSION_MINOR "${_contents}")
        string(REGEX REPLACE ".*#define JSON_C_MICRO_VERSION[ \t]+([0-9]+).*" "\\1" JSONC_VERSION_REVISION "${_contents}")
        set(JSONC_VERSION "${JSONC_VERSION_MAJOR}.${JSONC_VERSION_MINOR}.${JSONC_VERSION_REVISION}")
    endif()

    unset(_contents)
endif()

find_package_handle_standard_args(JSONC
    REQUIRED_VARS
        JSONC_INCLUDE_DIR
        JSONC_LIBRARY
    VERSION_VAR
        JSONC_VERSION
)

if(JSONC_FOUND)
    set(JSONC_LIBRARIES ${JSONC_LIBRARY})
    set(JSONC_INCLUDE_DIRS ${JSONC_INCLUDE_DIR})

    if(NOT TARGET JSONC::JSONC)
        add_library(JSONC::JSONC UNKNOWN IMPORTED)
        set_target_properties(JSONC::JSONC PROPERTIES
            IMPORTED_LOCATION "${JSONC_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${JSONC_INCLUDE_DIR}"
        )
    endif()

    mark_as_advanced(JSONC_INCLUDE_DIR JSONC_LIBRARY)
endif()
