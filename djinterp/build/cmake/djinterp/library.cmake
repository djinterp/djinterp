# =============================================================================
# djinterp - build/cmake/djinterp/library.cmake
#
#   djinterp_add_library: how every module's library is made (rulings 8 and
# 9 of 2026.11.17), so that each is made the same way.
#
#     djinterp_add_library(<target>
#         [API <NAME>]                the library's export macro is D_<NAME>_API
#         SOURCES <file>...
#         [LINKS <target or library>...])
#
#   Each source belongs to exactly one library. A library links what it
# needs, publicly, so a leaf or a program links the one library it uses.
#
#   Export macros. A library with an API name is built static or shared as
# BUILD_SHARED_LIBS says. Shared, it defines D_<NAME>_BUILDING privately and
# D_<NAME>_SHARED publicly, and its symbols are hidden unless marked: its
# public header declares them with D_<NAME>_API, made from D_EXPORT and
# D_IMPORT as inc/djinterp/env/c/env_vendor_attributes.h shows (6.1.4). A
# library without an API name has no marked symbols yet, so it stays static.
# =============================================================================

function(djinterp_add_library _target)

    cmake_parse_arguments(_arg "" "API" "SOURCES;LINKS" ${ARGN})

    if (_arg_API)
        add_library(${_target} ${_arg_SOURCES})
    else()
        add_library(${_target} STATIC ${_arg_SOURCES})
    endif()

    target_include_directories(${_target} PUBLIC "${DJINTERP_INC_ROOT}")
    target_compile_features(${_target} PRIVATE c_std_17)
    if (_arg_LINKS)
        target_link_libraries(${_target} PUBLIC ${_arg_LINKS})
    endif()

    set_target_properties(${_target} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        C_VISIBILITY_PRESET       hidden
        CXX_VISIBILITY_PRESET     hidden
        VISIBILITY_INLINES_HIDDEN ON
        FOLDER                    "djinterp/libraries"
    )

    if (_arg_API)
        get_target_property(_type ${_target} TYPE)
        if (_type STREQUAL "SHARED_LIBRARY")
            target_compile_definitions(${_target}
                PRIVATE D_${_arg_API}_BUILDING
                PUBLIC  D_${_arg_API}_SHARED)
        endif()
    endif()

    if (MSVC)
        target_compile_definitions(${_target} PRIVATE _UNICODE UNICODE)
        target_compile_options(${_target} PRIVATE
            /W3 /sdl /permissive- /Zc:preprocessor)
    endif()

endfunction()
