#*******************************************************************************
# bitwisedom [cmake]                                              Djinterp.cmake
#
# djinterp, as bitwisedom uses it: its headers, and its C and C++ lexer.
#
#     djinterp::headers   the include path: the environment layer (the
#                         language-level tests bitwisedom's floor stands
#                         on), the root's kit (D_NODISCARD and its
#                         siblings) and re_std (the fixed-width integers)
#     djinterp::lex       the C/C++ scanner and literal decoder, compiled as
#                         C at djinterp's C99 floor: bitwisedom reads macro
#                         bodies, imported headers and environments with it
#
#   The include directory is a SYSTEM one: djinterp is a dependency, and its
# headers are not this project's to hold to the style guide's whole
# baseline; nor are its sources, which are compiled with no warnings asked
# for. A parent that already defines either target keeps its own.
#   The lexer is nine sources of djinterp's parse layer: the scanner, its
# dialect tables and token kinds, its Unicode tables, the literal decoder,
# the C and C++ dialects, the source reader, and the diagnostic sink they
# report into. A djinterp tree without them predates the consolidated
# djinterp of 2026.10.07, and is refused by name.
#
# path:      /build/djinterp/tools/bitwisedom/cmake/Djinterp.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
#                                                            revised: 2026.10.09
#*******************************************************************************
include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/ProjectRoots.cmake")

if(NOT TARGET djinterp::headers)
    add_library(bitwisedom_djinterp_headers INTERFACE)
    add_library(djinterp::headers ALIAS bitwisedom_djinterp_headers)

    target_include_directories(bitwisedom_djinterp_headers SYSTEM INTERFACE
        "${DJINTERP_ROOT}/inc")
endif()


#-------------------------------------------------------------------------------
# the lexer
#-------------------------------------------------------------------------------
if(NOT TARGET djinterp::lex)
    set(_djinterp_parse "${DJINTERP_ROOT}/src/djinterp/parse")
    set(_djinterp_lex_sources
        "${_djinterp_parse}/lex/lex_dialect.c"
        "${_djinterp_parse}/lex/lex_literal.c"
        "${_djinterp_parse}/lex/lex_scan.c"
        "${_djinterp_parse}/lex/lex_token.c"
        "${_djinterp_parse}/lex/lex_unicode.c"
        "${_djinterp_parse}/lang/c/lex_c.c"
        "${_djinterp_parse}/lang/cpp/lex_cpp.c"
        "${_djinterp_parse}/source/source_reader.c"
        "${_djinterp_parse}/c/diagnostic.c")

    set(_djinterp_lex_missing "")

    foreach(_source ${_djinterp_lex_sources})
        if(NOT EXISTS "${_source}")
            file(RELATIVE_PATH _relative "${DJINTERP_ROOT}" "${_source}")
            list(APPEND _djinterp_lex_missing "${_relative}")
        endif()
    endforeach()

    if(_djinterp_lex_missing)
        string(REPLACE ";" "\n    " _djinterp_lex_missing
               "${_djinterp_lex_missing}")

        message(FATAL_ERROR
            "djinterp at ${DJINTERP_ROOT} has no C/C++ lexer; these are "
            "missing:\n    ${_djinterp_lex_missing}\n"
            "bitwisedom reads C with djinterp's lexer. Extract the "
            "consolidated djinterp (2026.10.07 or later) over DJINTERP_ROOT, "
            "or set -DDJINTERP_ROOT=<path> to a tree that has it.")
    endif()

    add_library(bitwisedom_djinterp_lex STATIC ${_djinterp_lex_sources})
    add_library(djinterp::lex ALIAS bitwisedom_djinterp_lex)

    target_link_libraries(bitwisedom_djinterp_lex PUBLIC djinterp::headers)

    # C at djinterp's floor, as vparse builds djinterp's C layer
    set_target_properties(bitwisedom_djinterp_lex PROPERTIES
        C_STANDARD                99
        C_STANDARD_REQUIRED       ON
        C_EXTENSIONS              OFF
        POSITION_INDEPENDENT_CODE ON
        FOLDER                    "djinterp")

    if(MSVC)
        target_compile_options(bitwisedom_djinterp_lex PRIVATE /utf-8)
        target_compile_definitions(bitwisedom_djinterp_lex PRIVATE
            _CRT_SECURE_NO_WARNINGS)

        if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
            target_compile_options(bitwisedom_djinterp_lex PRIVATE
                /Zc:preprocessor)
        endif()
    endif()
endif()
