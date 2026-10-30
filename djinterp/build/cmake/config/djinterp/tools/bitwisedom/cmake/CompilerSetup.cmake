#*******************************************************************************
# bitwisedom [cmake]                                         CompilerSetup.cmake
#
# The flags this build gives the code it owns, and the few it gives the code
# it only compiles.
#   bitwisedom_apply_compiler_defaults() is for this project's own sources --
# the library, the command line, the window and the tests -- and with OWN holds
# them to the style guide's whole baseline: -Wall -Wextra -Wshadow
# -Wcast-qual -Wconversion under -pedantic-errors and -Werror=undef, and
# under Clang -Wreserved-identifier and -Wdocumentation besides. Every
# dependency's headers -- djinterp's, uxoxo's, ImGui's, GLFW's -- are system
# headers to these targets, and so not judged by their warnings.
#   bitwisedom_apply_dependency_defaults() is for code this build compiles
# without owning it: the level and the source character set, and, under cl,
# the conforming preprocessor, since that code reads djinterp's headers too.
#   LEVELS. bitwisedom's floor is C++17 -- the floor of the djinterp
# headers it stands on, and of uxoxo's, which build/cmake/config/ui adds for the
# window -- and BITWISEDOM_CXX_STANDARD picks 17, 20 or
# 23, defaulting to 20. Below the floor every bitwisedom header is empty, as
# djinterp's are below theirs.
#   UNDER A CL-STYLE DRIVER (cl, clang-cl): /utf-8, because the trees are
# UTF-8 without a byte-order mark; /Zc:__cplusplus and /Zc:preprocessor, as
# vparse gives them; /permissive- and /EHsc; and /bigobj.
#
# path:      /build/cmake/config/djinterp/tools/bitwisedom/cmake/CompilerSetup.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
#                                                            revised: 2026.10.22
#*******************************************************************************
include_guard(GLOBAL)

set(BITWISEDOM_CXX_STANDARD 20 CACHE STRING
    "C++ level for bitwisedom and uxoxo: 17, 20 or 23")
set_property(CACHE BITWISEDOM_CXX_STANDARD PROPERTY STRINGS 17 20 23)

if(NOT BITWISEDOM_CXX_STANDARD MATCHES "^(17|20|23)$")
    message(FATAL_ERROR
        "BITWISEDOM_CXX_STANDARD is '${BITWISEDOM_CXX_STANDARD}'; it must be "
        "17, 20 or 23. bitwisedom's floor, and uxoxo's, is C++17.")
endif()

if(NOT "cxx_std_${BITWISEDOM_CXX_STANDARD}" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    message(FATAL_ERROR
        "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} cannot select "
        "C++${BITWISEDOM_CXX_STANDARD}. Choose another "
        "BITWISEDOM_CXX_STANDARD.")
endif()


# bitwisedom_set_level(<target> <c++ level>)
#   One target's language level, without vendor extensions.
function(bitwisedom_set_level _target _cxx)
    set_target_properties(${_target} PROPERTIES
        CXX_STANDARD              ${_cxx}
        CXX_STANDARD_REQUIRED     ON
        CXX_EXTENSIONS            OFF
        VISIBILITY_INLINES_HIDDEN ON)

    if(MSVC)
        target_compile_options(${_target} PRIVATE /utf-8)
    endif()
endfunction()


# bitwisedom_apply_compiler_defaults(<target> [CXX_STANDARD <level>] [OWN])
#   For this project's code: the level, the conformance flags and the
# warnings. OWN adds the rest of the style guide's baseline.
function(bitwisedom_apply_compiler_defaults _target)
    cmake_parse_arguments(PARSE_ARGV 1 _arg "OWN" "CXX_STANDARD" "")

    if(NOT _arg_CXX_STANDARD)
        set(_arg_CXX_STANDARD ${BITWISEDOM_CXX_STANDARD})
    endif()

    bitwisedom_set_level(${_target} ${_arg_CXX_STANDARD})

    if(MSVC)
        # /bigobj: the interface's units hold hundreds of lambdas in
        # std::function, the shape that runs a Debug object past cl's
        # 65,279 sections (C1128)
        target_compile_options(${_target} PRIVATE
            /Zc:__cplusplus /permissive- /EHsc /Zc:inline /bigobj)
        target_compile_definitions(${_target} PRIVATE
            _CRT_SECURE_NO_WARNINGS)

        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${_target} PRIVATE
                /W4 -Wshadow -Wcast-qual -Wno-nonportable-include-path)

            if(_arg_OWN)
                target_compile_options(${_target} PRIVATE
                    -Wconversion -Wreserved-identifier -Wdocumentation)
            endif()
        else()
            target_compile_options(${_target} PRIVATE
                /Zc:preprocessor /MP /diagnostics:caret /W4)
        endif()

        if(BITWISEDOM_WARNINGS_AS_ERRORS)
            target_compile_options(${_target} PRIVATE /WX)
        endif()

        return()
    endif()

    target_compile_options(${_target} PRIVATE
        -Wall -Wextra -Wshadow -Wcast-qual
        -pedantic-errors -Werror=undef)

    if(_arg_OWN)
        target_compile_options(${_target} PRIVATE -Wconversion)

        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${_target} PRIVATE
                -Wreserved-identifier -Wdocumentation)
        endif()
    endif()

    if(BITWISEDOM_WARNINGS_AS_ERRORS)
        target_compile_options(${_target} PRIVATE -Werror)
    endif()
endfunction()


# bitwisedom_apply_dependency_defaults(<target>)
#   For code this build compiles and does not own: the level and the source
# character set, and under cl /Zc:preprocessor -- that code reads djinterp's
# headers, which every unit of this build reads with the conforming
# preprocessor, not cl's traditional one. Nothing else: no warnings.
function(bitwisedom_apply_dependency_defaults _target)
    bitwisedom_set_level(${_target} ${BITWISEDOM_CXX_STANDARD})

    if( (MSVC) AND (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang") )
        target_compile_options(${_target} PRIVATE /Zc:preprocessor)
    endif()
endfunction()
