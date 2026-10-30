#*******************************************************************************
# vparse [cmake]                                                  Djinterp.cmake
#
# Makes djinterp available as imported-style targets:
#
#     djinterp::headers            its include path alone, for a consumer that
#                                  needs its headers and none of its objects
#     djinterp::c_base             the two C sources of the root the libraries
#                                  below share: re_std's dstdint.c, what the
#                                  fixed-width integer header declares, and
#                                  c/util/sink_common.c, the byte sinks
#     djinterp::parse_foundation   the parse/parsegen C foundation, compiled as
#                                  C at the C99 floor; the C++ faces above it
#                                  are headers
#     djinterp::arg_parser         the argument parser, parse/cli/arg_parser,
#                                  CLI11 in C, compiled as C at the C99 floor:
#                                  what vparse's command line and its window
#                                  read their arguments with
#     djinterp::vparse             the in-tree vparse: PEG machine, generator,
#                                  notation, carrier adapter, LR family, EBNF,
#                                  and the session the workbench drives
#
#   They are built here, from the sources at DJINTERP_ROOT. djinterp's own
# CMake, under build/cmake/config/, is a test tree: it builds one C runtime
# library and recurses into its test leaves, and defines none of these
# three. So it is not added unless DJINTERP_USE_OWN_CMAKE says to; when it
# is, whatever it exports under a known name is used in place of a build
# here.
#   The C layer is compiled AS C on purpose: it is the C foundation, and
# building it through a C++ compiler would stop proving that. The two root
# sources are a library of their own, so that the foundation and the
# argument parser, which both use them, link one copy and not two.
#   These are the production configuration. The suites need the testing one,
# which changes a struct's layout, so they build a library of their own; see
# tests/CMakeLists.txt beside the CMakeLists this module serves.
#
# path:      /build/cmake/config/djinterp/tools/vparse/cmake/Djinterp.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
#                                                            revised: 2026.10.16
#*******************************************************************************
include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/ProjectRoots.cmake")

if(TARGET djinterp::parse_foundation)
    return()
endif()

option(DJINTERP_USE_OWN_CMAKE
       "Add djinterp's own CMakeLists, if it has one, before building here"
       OFF)


#-------------------------------------------------------------------------------
# the include path, on its own
#-------------------------------------------------------------------------------
#   Defined before anything below, because it is just a directory and is
# always correct. uxoxo consumes djinterp headers too, so it needs this
# without linking the parse foundation's objects -- and anything else that
# grows into this build will need the same.
add_library(djinterp_headers INTERFACE)
add_library(djinterp::headers ALIAS djinterp_headers)

target_include_directories(djinterp_headers INTERFACE "${DJINTERP_ROOT}/inc")


#-------------------------------------------------------------------------------
# djinterp's own build, when asked for
#-------------------------------------------------------------------------------
# djinterp keeps its CMake under build/cmake/config/ rather than at the root
# -- its root list file, the djinterp_add_test_executable helper beside it,
# and the test leaves below -- so that is a candidate, with the places
# nearer the root one would look first.
set(_djinterp_entry "")

foreach(_candidate "${DJINTERP_ROOT}"
                   "${DJINTERP_ROOT}/build/cmake/config"
                   "${DJINTERP_ROOT}/build/cmake"
                   "${DJINTERP_ROOT}/build")
    if(EXISTS "${_candidate}/CMakeLists.txt")
        set(_djinterp_entry "${_candidate}")
        break()
    endif()
endforeach()

if(_djinterp_entry AND NOT DJINTERP_USE_OWN_CMAKE)
    message(STATUS
        "djinterp: it has a CMakeLists at ${_djinterp_entry}, which is not "
        "added (DJINTERP_USE_OWN_CMAKE is OFF)")
elseif(_djinterp_entry)
    message(STATUS "djinterp: adding its own CMakeLists at ${_djinterp_entry}")

    add_subdirectory("${_djinterp_entry}" "${CMAKE_BINARY_DIR}/_deps/djinterp")

    # Its target names are its own; alias whatever it exports to the names
    # this build uses. Extend this list as djinterp's CMake tree grows.
    foreach(_pair "djinterp_parse_foundation:parse_foundation"
                  "djinterp_arg_parser:arg_parser"
                  "djinterp_vparse:vparse")
        string(REPLACE ":" ";" _parts "${_pair}")
        list(GET _parts 0 _actual)
        list(GET _parts 1 _alias)

        if(TARGET ${_actual} AND NOT TARGET djinterp::${_alias})
            add_library(djinterp::${_alias} ALIAS ${_actual})
        endif()
    endforeach()

    if(TARGET djinterp::parse_foundation)
        return()
    endif()

    message(STATUS
        "djinterp: its CMakeLists did not define the foundation target; "
        "building it from sources instead")
endif()


#-------------------------------------------------------------------------------
# the foundation, from sources
#-------------------------------------------------------------------------------
message(STATUS "djinterp: building from sources at ${DJINTERP_ROOT}")

set(_djinterp_src "${DJINTERP_ROOT}/src/djinterp")

# CONFIGURE_DEPENDS so a newly added .c file re-globs on the next build rather
# than being silently left out until someone reconfigures by hand.
file(GLOB _djinterp_c_sources CONFIGURE_DEPENDS
    "${_djinterp_src}/c/modules/parse/*.c"
    "${_djinterp_src}/c/modules/parsegen/*.c")

if(NOT _djinterp_c_sources)
    message(FATAL_ERROR
        "No C sources under:\n"
        "    ${_djinterp_src}/c/modules/parse/\n"
        "    ${_djinterp_src}/c/modules/parsegen/\n"
        "This does not look like a djinterp checkout that has the "
        "parse/parsegen foundation. Set -DDJINTERP_ROOT=<path> to one that "
        "does.")
endif()

# The fixed-width integers come from re_std, and what its C header declares is
# defined in this one source. A tree without it predates the down-port.
set(_djinterp_dstdint "${DJINTERP_ROOT}/src/re_std/c/cstdint/dstdint.c")

if(NOT EXISTS "${_djinterp_dstdint}")
    message(FATAL_ERROR
        "${_djinterp_dstdint}\nwas not found. The foundation's sources "
        "include re_std/cstdint/dstdint.h, so this build needs the down-ported "
        "djinterp tree (round 3, 2026.10.03, or later) at DJINTERP_ROOT.")
endif()

# The C layer once sat directly under parse/ and parsegen/, and a tree that
# was only ever unzipped over still has those copies: an unzip adds and
# overwrites, and never deletes. Nothing here builds them -- the globs above
# take c/ alone -- but each defines the functions its successor under c/
# does, so anything that globs either directory links every function twice.
# Say so once, where it will be read, with the way out.
set(_djinterp_stale "")

foreach(_name charset diagnostic machine pool program storage)
    if(EXISTS "${_djinterp_src}/parse/${_name}.c")
        list(APPEND _djinterp_stale "src/djinterp/parse/${_name}.c")
    endif()
endforeach()

foreach(_name analysis feature grammar registry)
    if(EXISTS "${_djinterp_src}/parsegen/${_name}.c")
        list(APPEND _djinterp_stale "src/djinterp/parsegen/${_name}.c")
    endif()
endforeach()

if(_djinterp_stale)
    string(REPLACE ";" "\n    " _djinterp_stale "${_djinterp_stale}")

    # the script that retires them comes with the parse/parsegen suites
    set(_djinterp_retire
        "${DJINTERP_ROOT}/_retired/parse_leftovers_2026.10.06/retire.cmake")

    if(EXISTS "${_djinterp_retire}")
        set(_djinterp_remedy
            "Retire them, and the other parse and parsegen files the "
            "repository has retired, with:\n"
            "    cmake -P \"${_djinterp_retire}\"")
    else()
        set(_djinterp_remedy
            "Delete them, or retire them as the tree's _retired/ does.")
    endif()

    string(CONCAT _djinterp_remedy ${_djinterp_remedy})

    message(WARNING
        "djinterp has stale copies of the parse and parsegen C layer, from "
        "before it moved into c/:\n    ${_djinterp_stale}\n"
        "Their successors are under src/djinterp/parse/c/ and "
        "src/djinterp/parsegen/c/. This build does not compile them, but "
        "they define the same functions.\n${_djinterp_remedy}")
endif()

# the byte sinks the argument parser writes through
set(_djinterp_sinks "${_djinterp_src}/c/util/sink_common.c")

if(NOT EXISTS "${_djinterp_sinks}")
    message(FATAL_ERROR
        "${_djinterp_sinks}\nwas not found: djinterp's C root has its byte "
        "sinks there, which the argument parser writes through.")
endif()

add_library(djinterp_c_base STATIC
    "${_djinterp_dstdint}"
    "${_djinterp_sinks}")
add_library(djinterp::c_base ALIAS djinterp_c_base)

target_link_libraries(djinterp_c_base PUBLIC djinterp::headers)

vparse_apply_compiler_defaults(djinterp_c_base)

set_target_properties(djinterp_c_base PROPERTIES
    FOLDER                    "djinterp"
    POSITION_INDEPENDENT_CODE ON)

add_library(djinterp_parse_foundation STATIC ${_djinterp_c_sources})
add_library(djinterp::parse_foundation ALIAS djinterp_parse_foundation)

# PUBLIC: every consumer includes djinterp headers by the same
# "djinterp/..." spelling the framework's sources use.
target_link_libraries(djinterp_parse_foundation PUBLIC djinterp::c_base)

vparse_apply_compiler_defaults(djinterp_parse_foundation)

set_target_properties(djinterp_parse_foundation PROPERTIES
    FOLDER                    "djinterp"
    POSITION_INDEPENDENT_CODE ON)


#-------------------------------------------------------------------------------
# the argument parser
#-------------------------------------------------------------------------------
# CONFIGURE_DEPENDS, as for the foundation: the parser is being written, and
# a source it gains is to be built, not left out until someone reconfigures.
# Its knobs keep their defaults (config/parse/cli/arg_parser): every one on.
file(GLOB _djinterp_arg_parser_sources CONFIGURE_DEPENDS
    "${_djinterp_src}/c/modules/parse/cli/arg_parser/*.c")

if(_djinterp_arg_parser_sources)
    add_library(djinterp_arg_parser STATIC ${_djinterp_arg_parser_sources})
    add_library(djinterp::arg_parser ALIAS djinterp_arg_parser)

    target_link_libraries(djinterp_arg_parser PUBLIC djinterp::c_base)

    vparse_apply_compiler_defaults(djinterp_arg_parser)

    set_target_properties(djinterp_arg_parser PROPERTIES
        FOLDER                    "djinterp"
        POSITION_INDEPENDENT_CODE ON)
else()
    message(STATUS
        "djinterp: no argument parser under "
        "src/djinterp/parse/cli/arg_parser/c/; djinterp::arg_parser will "
        "not be defined")
endif()


#-------------------------------------------------------------------------------
# the in-tree vparse
#-------------------------------------------------------------------------------
file(GLOB _djinterp_vparse_sources CONFIGURE_DEPENDS
    "${_djinterp_src}/cpp/modules/parsegen/vparse/*.cpp")

if(_djinterp_vparse_sources)
    add_library(djinterp_vparse STATIC ${_djinterp_vparse_sources})
    add_library(djinterp::vparse ALIAS djinterp_vparse)

    # Its headers reach one another by relative path and its sources reach
    # them through inc/, so djinterp's include path is all it needs. It does
    # not use the C foundation yet -- the port onto it is still to come -- so
    # the headers alone are its dependency.
    target_link_libraries(djinterp_vparse PUBLIC djinterp::headers)

    vparse_apply_compiler_defaults(djinterp_vparse)

    set_target_properties(djinterp_vparse PROPERTIES
        FOLDER                    "djinterp"
        POSITION_INDEPENDENT_CODE ON)
else()
    message(STATUS
        "djinterp: no in-tree vparse sources found; djinterp::vparse "
        "will not be defined")
endif()
