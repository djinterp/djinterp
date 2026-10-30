#*******************************************************************************
# vparse [cmake]                                             CompilerSetup.cmake
#
# The flags this build gives the code it owns, and the few it gives the code
# it only compiles.
#   vparse_apply_compiler_defaults() is for djinterp's foundation and in-tree
# vparse, for the workbench and for its tests: the language levels, the
# conformance flags and a warning set. With OWN it is this project's own
# sources that are being built, and the set is the style guide's whole
# baseline; without it, the set is the one djinterp's code is clean under.
#   vparse_apply_dependency_defaults() is for code this build compiles without
# owning it -- uxoxo's sources, and Dear ImGui's. It sets the language level
# and the source character set and nothing else. Which Windows macros a GUI
# framework is compiled under, and which warnings it is held to, are its own
# business: NOGDI or WIN32_LEAN_AND_MEAN imposed from here would take away
# declarations it may well use.
#   ONE WAY OF READING DJINTERP. Under a cl-style driver a handful of flags
# decide what a header means: the character set its bytes are in, which
# preprocessor expands its macros, which language rules its templates are
# held to. Every unit that includes djinterp's headers is given the same
# ones -- this project's, djinterp's, uxoxo's (READS_DJINTERP), djinterp's
# test leaves, and the unit configure compiles to learn whether uxoxo fits --
# so a header is not read one way where it is probed and another where it is
# built. They are the VPARSE_CL_READS lists below; Dear ImGui, which includes
# none of djinterp, is given the character set alone.
#
#   LEVELS. djinterp's C floor is C99, and the foundation and the argument
# parser are built at it. Its C++ faces and the in-tree vparse have a module
# floor of C++11. VPARSE_CXX_STANDARD picks the level they are built at --
# any of 11, 14, 17, 20 or 23 -- and defaults to 20, the level the framework
# targets. VPARSE_TOOL_CXX_STANDARD does the same for the tool's model and
# its command line, whose floor is C++17 (std::filesystem, djinterp's ui);
# UXOXO_CXX_STANDARD for uxoxo and the window, which
# build/cmake/config/ui/vparse builds with this build within it, and which must
# be 17 or higher as well: uxoxo's headers reach djinterp's free monad, whose
# floor is C++17. The levels may differ: no level changes what a shared type is.
#
#   UNDER A CL-STYLE DRIVER (cl, clang-cl):
#     /utf-8            required. The trees' sources are UTF-8 with no
#                       byte-order mark, as the style guide rules, and cl
#                       reads such a file in the system code page unless told
#                       otherwise. Some 360 djinterp headers carry non-ASCII
#                       characters, env/ and parse/ among them.
#     /Zc:preprocessor  kept, for cl. djinterp has a path for cl's
#                       traditional preprocessor, but the conforming one is
#                       what every other compiler has, and so what the
#                       framework has been verified under. clang-cl has no
#                       other, and is not given the flag.
#     /Zc:__cplusplus   kept, no longer required. env/env_lang.h now takes
#                       the larger of __cplusplus and _MSVC_LANG; the flag
#                       makes __cplusplus itself tell the truth to whatever
#                       else reads it.
#     /permissive-      the language as the standard has it, which is how
#                       every other compiler reads these headers. /std:c++20
#                       and later imply it; C++14 and C++17 do not.
#     /EHsc             exceptions on, as the headers assume. CMake's default
#                       flags have it too; it is here so that a build with
#                       its own CMAKE_CXX_FLAGS still does.
#
# path:      /build/cmake/config/djinterp/tools/vparse/cmake/CompilerSetup.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
#                                                            revised: 2026.10.26
#*******************************************************************************
include_guard(GLOBAL)

set(VPARSE_CXX_STANDARD 20 CACHE STRING
    "C++ level for djinterp's vparse and this project: 11, 14, 17, 20 or 23")
set_property(CACHE VPARSE_CXX_STANDARD PROPERTY STRINGS 11 14 17 20 23)

set(VPARSE_TOOL_CXX_STANDARD 20 CACHE STRING
    "C++ level for the tool's model and command line: 17, 20 or 23")
set_property(CACHE VPARSE_TOOL_CXX_STANDARD PROPERTY STRINGS 17 20 23)

set(UXOXO_CXX_STANDARD 20 CACHE STRING
    "C++ level for uxoxo and the window: 17, 20 or 23")
set_property(CACHE UXOXO_CXX_STANDARD PROPERTY STRINGS 17 20 23)

if(NOT VPARSE_CXX_STANDARD MATCHES "^(11|14|17|20|23)$")
    message(FATAL_ERROR
        "VPARSE_CXX_STANDARD is '${VPARSE_CXX_STANDARD}'; it must be one of "
        "11, 14, 17, 20 or 23. The module floor is C++11.")
endif()

if(NOT VPARSE_TOOL_CXX_STANDARD MATCHES "^(17|20|23)$")
    message(FATAL_ERROR
        "VPARSE_TOOL_CXX_STANDARD is '${VPARSE_TOOL_CXX_STANDARD}'; it must "
        "be 17, 20 or 23. The model reads and writes run folders through "
        "std::filesystem, and its heat map is djinterp's ui, both C++17.")
endif()

if(NOT UXOXO_CXX_STANDARD MATCHES "^(17|20|23)$")
    message(FATAL_ERROR
        "UXOXO_CXX_STANDARD is '${UXOXO_CXX_STANDARD}'; it must be 17, 20 or "
        "23. uxoxo's headers include djinterp/core/functional/free.hpp, which "
        "is empty below C++17.")
endif()

# a level this compiler cannot select is better refused here, by name, than
# met as an unknown flag in the middle of a build
foreach(_level ${VPARSE_CXX_STANDARD}
               ${VPARSE_TOOL_CXX_STANDARD}
               ${UXOXO_CXX_STANDARD})
    if(NOT "cxx_std_${_level}" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        message(FATAL_ERROR
            "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} cannot "
            "select C++${_level}. Choose another VPARSE_CXX_STANDARD or "
            "UXOXO_CXX_STANDARD.")
    endif()
endforeach()


# VPARSE_CL_READS, VPARSE_CL_READS_CXX
#   lists: how a cl-style driver is told to read a source that includes
# djinterp's headers -- the first for C and C++ alike, the second for C++
# besides. Both are empty under any other driver. See the banner.
#   Cache entries, not variables: the window's build, build/cmake/config/ui/vparse, adds
# this build within it and applies these functions to its own targets and
# uxoxo's in its own directory, where a variable set here is not seen.
set(_vparse_reads     "")
set(_vparse_reads_cxx "")

if(MSVC)  # true for cl and for clang-cl
    list(APPEND _vparse_reads     /utf-8)
    list(APPEND _vparse_reads_cxx /Zc:__cplusplus /permissive- /EHsc)

    # cl alone: clang-cl has no other preprocessor, and reports the flag as
    # an unused argument, which warnings as errors would turn into a failed
    # build
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND _vparse_reads /Zc:preprocessor)
    endif()
endif()

set(VPARSE_CL_READS "${_vparse_reads}" CACHE INTERNAL
    "How a cl-style driver reads djinterp's headers")
set(VPARSE_CL_READS_CXX "${_vparse_reads_cxx}" CACHE INTERNAL
    "How a cl-style driver reads djinterp's headers, for C++ besides")


# vparse_set_levels(<target> <c++ level>)
#   The language levels of one target: C at djinterp's floor, C++ at the level
# given, both without vendor extensions. Set per target rather than globally,
# so a dependency added with add_subdirectory() keeps whatever it asks for.
function(vparse_set_levels _target _cxx)
    set_target_properties(${_target} PROPERTIES
        C_STANDARD                99
        C_STANDARD_REQUIRED       ON
        C_EXTENSIONS              OFF
        CXX_STANDARD              ${_cxx}
        CXX_STANDARD_REQUIRED     ON
        CXX_EXTENSIONS            OFF
        VISIBILITY_INLINES_HIDDEN ON)
endfunction()


# vparse_apply_charset(<target> <scope>)
#   Tells a cl-style driver the sources are UTF-8, and nothing else: for code
# that includes none of djinterp. See the banner.
function(vparse_apply_charset _target _scope)
    if(MSVC)  # true for cl and for clang-cl
        target_compile_options(${_target} ${_scope} /utf-8)
    endif()
endfunction()


# vparse_apply_reading(<target> <scope>)
#   Tells a cl-style driver how to read sources that include djinterp's
# headers: the VPARSE_CL_READS lists, the C++ ones for C++ sources alone,
# since some of them warn on a .c. Nothing under any other driver.
function(vparse_apply_reading _target _scope)
    if(VPARSE_CL_READS)
        target_compile_options(${_target} ${_scope} ${VPARSE_CL_READS})
    endif()

    foreach(_flag IN LISTS VPARSE_CL_READS_CXX)
        target_compile_options(${_target} ${_scope}
            $<$<COMPILE_LANGUAGE:CXX>:${_flag}>)
    endforeach()
endfunction()


# vparse_apply_compiler_defaults(<target> [PUBLIC|PRIVATE|INTERFACE]
#                                [CXX_STANDARD <level>] [OWN])
#   For code this build owns: the levels, the conformance flags and the
# warning set. The scope defaults to PRIVATE -- flags are a build detail, not
# something a consumer inherits -- and the level to VPARSE_CXX_STANDARD.
#   OWN marks this project's own sources, which are held to the style guide's
# whole baseline. djinterp's are held to the set they are clean under: its
# in-tree vparse has 23 sign conversions that -Wconversion reports under
# Clang, and those are djinterp's to fix, not this build's to trip over.
function(vparse_apply_compiler_defaults _target)
    cmake_parse_arguments(PARSE_ARGV 1 _arg "OWN" "CXX_STANDARD" "")

    set(_scope "PRIVATE")

    if(_arg_UNPARSED_ARGUMENTS)
        list(GET _arg_UNPARSED_ARGUMENTS 0 _scope)
    endif()

    if(NOT _arg_CXX_STANDARD)
        set(_arg_CXX_STANDARD ${VPARSE_CXX_STANDARD})
    endif()

    get_target_property(_type ${_target} TYPE)

    if(_type STREQUAL "INTERFACE_LIBRARY")
        set(_scope "INTERFACE")
    else()
        vparse_set_levels(${_target} ${_arg_CXX_STANDARD})
    endif()

    vparse_apply_reading(${_target} ${_scope})

    # This project's sources take the whole baseline, and djinterp's headers
    # are not written to all of it: re_std's dstdint.h, for one, defines the
    # platform's own request macros (__STDC_LIMIT_MACROS and its siblings),
    # which -Wreserved-identifier reports. So to this project's sources
    # djinterp is a dependency, and its headers are system headers -- found
    # in the same place, and not judged by a consumer's warnings.
    if(_arg_OWN)
        target_include_directories(${_target} SYSTEM ${_scope}
            "${DJINTERP_ROOT}/inc")
    endif()

    #---------------------------------------------------------------------------
    # under a cl-style driver
    #---------------------------------------------------------------------------
    if(MSVC)
        # how the sources are read is settled above; this one is about what
        # is emitted, and is C++'s alone
        target_compile_options(${_target} ${_scope}
            $<$<COMPILE_LANGUAGE:CXX>:/Zc:inline>)

        # the C library is not deprecated here
        target_compile_definitions(${_target} ${_scope}
            _CRT_SECURE_NO_WARNINGS)

        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # clang-cl: /W4 is clang's -Wall -Wextra; the rest by name
            target_compile_options(${_target} ${_scope}
                /W4
                -Wshadow
                -Wcast-qual
                # the MSVC headers' own constructs, not this project's code
                -Wno-nonportable-include-path)

            if(_arg_OWN)
                target_compile_options(${_target} ${_scope}
                    -Wconversion
                    -Wreserved-identifier
                    -Wdocumentation)
            endif()
        else()
            # cl alone takes this. clang-cl's operator new always throws, and
            # it reports the flag as an unused argument, which warnings as
            # errors would turn into a failed build
            target_compile_options(${_target} ${_scope}
                $<$<COMPILE_LANGUAGE:CXX>:/Zc:throwingNew>)

            # /MP parallelises within a target; caret diagnostics are
            # readable in a terminal in a way the default is not
            target_compile_options(${_target} ${_scope}
                /MP
                /diagnostics:caret
                # C4200: zero-sized array in a struct -- a deliberate idiom
                # in the C layer's flexible trailing members
                /wd4200)

            # cl's /W4 has never been run over djinterp, so its code keeps
            # the level the compiler defaults to; this project's own takes
            # the higher one
            if(_arg_OWN)
                target_compile_options(${_target} ${_scope} /W4)
            else()
                target_compile_options(${_target} ${_scope} /W3)
            endif()
        endif()

        if(VPARSE_WARNINGS_AS_ERRORS)
            target_compile_options(${_target} ${_scope} /WX)
        endif()

        return()
    endif()

    #---------------------------------------------------------------------------
    # under GCC or Clang
    #---------------------------------------------------------------------------
    # the diagnostics djinterp's own ladder builds with -- a construct the
    # level lacks is an error, and so is an #if on a macro nobody defined --
    # with the usual warnings beside them
    target_compile_options(${_target} ${_scope}
        -Wall -Wextra -Wshadow -Wcast-qual
        -pedantic-errors -Werror=undef)

    if(_arg_OWN)
        target_compile_options(${_target} ${_scope} -Wconversion)

        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${_target} ${_scope}
                -Wreserved-identifier
                -Wdocumentation)
        endif()
    endif()

    # a strict ISO mode hides the POSIX and XSI names the framework uses on
    # POSIX systems; the ladder defines this for the same reason
    if(UNIX AND NOT APPLE)
        target_compile_definitions(${_target} ${_scope} _XOPEN_SOURCE=700)
    endif()

    if(VPARSE_WARNINGS_AS_ERRORS)
        target_compile_options(${_target} ${_scope} -Werror)
    endif()
endfunction()


# vparse_apply_dependency_defaults(<target> [CXX_STANDARD <level>]
#                                  [READS_DJINTERP])
#   For code this build compiles and does not own: the levels and the source
# character set, and nothing else -- no warning set, and no Windows macro.
# The level defaults to UXOXO_CXX_STANDARD.
#   READS_DJINTERP marks a dependency whose sources include djinterp's
# headers, as uxoxo's do. It is then given the flags every other unit here
# reads those headers under, in place of the character set alone: see the
# banner.
function(vparse_apply_dependency_defaults _target)
    cmake_parse_arguments(PARSE_ARGV 1 _arg "READS_DJINTERP" "CXX_STANDARD" "")

    if(NOT _arg_CXX_STANDARD)
        set(_arg_CXX_STANDARD ${UXOXO_CXX_STANDARD})
    endif()

    vparse_set_levels(${_target} ${_arg_CXX_STANDARD})

    if(_arg_READS_DJINTERP)
        vparse_apply_reading(${_target} PRIVATE)
    else()
        vparse_apply_charset(${_target} PRIVATE)
    endif()
endfunction()
