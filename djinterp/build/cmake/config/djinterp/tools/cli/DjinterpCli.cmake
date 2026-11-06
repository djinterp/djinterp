#*******************************************************************************
# djinterp [tools]                                             DjinterpCli.cmake
#
# djinterp's command-line kit, for a program built with it.
#   Include it from a project that has C enabled. It defines, once:
#
#     djinterp::arg_parser   the argument parser: its C sources as C99, the
#                            sinks it writes through, and re_std's one C
#                            source
#     djinterp::cli          the kit: its C sources as C99, and filesys, which
#                            reads and writes its files; it links
#                            djinterp::arg_parser and the dynamic loader
#
# both static by default, position-independent so that a module can link
# them, with `inc/` on their consumers' include path as a system directory
# -- a dependency's headers are not judged by its consumers' warnings -- and,
# under cl, /Zc:preprocessor for them and for every unit that includes the
# kit's headers: djinterp's variadic macros need a conforming preprocessor,
# which Visual Studio 2022, the version djinterp requires, has (ruling cli
# b). What a module holds is a build option, DJINTERP_CLI_LINKAGE (ruling
# cli-taken 13): SELF_CONTAINED, the default, links the libraries into the
# program and, a copy of its own, into each module, the two meeting only at
# the plugin interface; SHARED_CORE makes the two one shared library, the
# core, which the program and its modules both link -- one copy, and a
# program that is no longer one file. There is no run-time choice: linking
# is fixed when the program is built. And the
# function
#
#     d_cli_add_plugin(<target> SOURCES <source>...)
#
# which makes a module a program loads for one of its commands (GRAMMAR.md,
# section 7): a MODULE library with no `lib` prefix, linking djinterp::cli,
# whose only exported symbol is its entry point, d_cli_plugin_entry --
# hidden visibility, the static libraries' symbols kept in (GNU ld and lld's
# --exclude-libs), and on Apple's linker that one symbol exported; under
# SHARED_CORE, the core linked as the program links it. Name it, and place
# it, as the program looks for it: `PROGRAM-COMMAND`, beside the program or
# in `../lib/PROGRAM/` (D_CFG_CLI_PLUGIN_SEARCH). And the function
#
#     d_cli_add_test(<name> COMMAND <program> [<argument>...]
#                    STATUS <status> [MATCHES <regex>]
#                    [WORKING_DIRECTORY <directory>])
#
# which makes a test of a run: it passes when the program ends with the
# status given (GRAMMAR.md, section 8) and, with MATCHES, says something the
# expression matches, on its output or its errors -- both held at once,
# where a PASS_REGULAR_EXPRESSION of ctest's passes whatever the status.
# DjinterpCliTest.cmake, beside this file, runs the program for it.
#   DJINTERP_ROOT, the repository root -- the directory with inc/ and src/ --
# defaults to the tree this file is in. A project that has made the two
# libraries its own way -- the kit's tests build them in testing mode --
# defines djinterp::arg_parser and djinterp::cli first, from the lists
# DjinterpCliSources.cmake gives, and this file then makes neither and
# gives d_cli_add_plugin alone.
#
# path:      /build/cmake/config/djinterp/tools/cli/DjinterpCli.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
#                                                            revised: 2026.11.04
#*******************************************************************************
include_guard(GLOBAL)

if(NOT DJINTERP_ROOT)
    get_filename_component(DJINTERP_ROOT
                           "${CMAKE_CURRENT_LIST_DIR}/../../../../../.."
                           ABSOLUTE)
endif()

# the libraries are C: an enclosing project that did not enable it would
# otherwise fail far from the cause
get_property(_djinterp_cli_languages GLOBAL PROPERTY ENABLED_LANGUAGES)

if(NOT "C" IN_LIST _djinterp_cli_languages)
    message(FATAL_ERROR
            "djinterp's command-line kit is C: the enclosing project() must "
            "enable it (LANGUAGES C)")
endif()

# cl's conforming preprocessor came with Visual Studio 2019 16.5; djinterp
# requires Visual Studio 2022
if( (CMAKE_C_COMPILER_ID STREQUAL "MSVC") AND
    (CMAKE_C_COMPILER_VERSION VERSION_LESS 19.25) )
    message(FATAL_ERROR
            "djinterp's command-line kit needs cl's conforming preprocessor "
            "(/Zc:preprocessor), which cl ${CMAKE_C_COMPILER_VERSION} does "
            "not have: build with Visual Studio 2022 (cl 19.30) or later")
elseif( (CMAKE_C_COMPILER_ID STREQUAL "MSVC") AND
        (CMAKE_C_COMPILER_VERSION VERSION_LESS 19.30) )
    message(WARNING
            "cl ${CMAKE_C_COMPILER_VERSION} is older than Visual Studio "
            "2022's, the version djinterp requires; the kit is not tested "
            "with it")
endif()

# DJINTERP_CLI_LINKAGE
#   cache: what a program's modules hold of the kit -- SELF_CONTAINED, a
# copy each, or SHARED_CORE, the one shared library the program links too.
set(DJINTERP_CLI_LINKAGE "SELF_CONTAINED" CACHE STRING
    "What a program built with djinterp's kit and its modules link: \
SELF_CONTAINED (each its own copy) or SHARED_CORE (one shared library)")
set_property(CACHE DJINTERP_CLI_LINKAGE PROPERTY
             STRINGS SELF_CONTAINED SHARED_CORE)

if(NOT DJINTERP_CLI_LINKAGE MATCHES "^(SELF_CONTAINED|SHARED_CORE)$")
    message(FATAL_ERROR
            "DJINTERP_CLI_LINKAGE is '${DJINTERP_CLI_LINKAGE}'; it must be "
            "SELF_CONTAINED or SHARED_CORE")
endif()


#-------------------------------------------------------------------------------
# the sources
#-------------------------------------------------------------------------------
# DJINTERP_ARG_PARSER_SOURCES and DJINTERP_CLI_SOURCES, which a test build
# that compiles them its own way includes alone
include("${CMAKE_CURRENT_LIST_DIR}/DjinterpCliSources.cmake")

# the script a test of a run runs, kept as a global property: a project may
# include this file in one directory and make its tests in another, where a
# variable set here is not seen
set_property(GLOBAL PROPERTY D_CLI_TEST_SCRIPT
             "${CMAKE_CURRENT_LIST_DIR}/DjinterpCliTest.cmake")


#-------------------------------------------------------------------------------
# the libraries
#-------------------------------------------------------------------------------

# djinterp_cli_c_target
#   function: a library of djinterp's C at its floor: C99 with no
# extensions, position-independent, with the POSIX names a strict ISO mode
# hides where the platform hides them, and cl reading the sources as the
# UTF-8 they are, with its conforming preprocessor -- which every unit that
# includes the library's headers is given too (ruling cli b).
function(djinterp_cli_c_target _target)
    set_target_properties(${_target} PROPERTIES
        C_STANDARD                99
        C_STANDARD_REQUIRED       ON
        C_EXTENSIONS              OFF
        POSITION_INDEPENDENT_CODE ON
        FOLDER                    "djinterp")

    # a dependency's headers, to a consumer: not judged by its warnings
    target_include_directories(${_target} SYSTEM PUBLIC "${DJINTERP_ROOT}/inc")

    # cl's own, not clang-cl's, whose preprocessor conforms already
    target_compile_options(${_target} PUBLIC
        "$<$<COMPILE_LANG_AND_ID:C,MSVC>:/Zc:preprocessor>"
        "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/Zc:preprocessor>")

    if(MSVC)
        target_compile_options(${_target} PRIVATE /utf-8)
        target_compile_definitions(${_target} PRIVATE
            _CRT_SECURE_NO_WARNINGS)
    elseif(UNIX AND NOT APPLE)
        target_compile_definitions(${_target} PRIVATE _XOPEN_SOURCE=700)
    endif()
endfunction()

if( (DJINTERP_CLI_LINKAGE STREQUAL "SHARED_CORE") AND
    (NOT TARGET djinterp::arg_parser) AND
    (NOT TARGET djinterp::cli) )
    # one shared library, the core, for the program and its modules: every
    # function exported -- on Windows by CMake's list of them, until the
    # core's own export macro is agreed (ruling cli-taken 13, lane A)
    add_library(djinterp_cli SHARED
        ${DJINTERP_ARG_PARSER_SOURCES}
        ${DJINTERP_CLI_SOURCES})
    add_library(djinterp::cli ALIAS djinterp_cli)
    add_library(djinterp::arg_parser ALIAS djinterp_cli)
    djinterp_cli_c_target(djinterp_cli)

    set_target_properties(djinterp_cli PROPERTIES
        OUTPUT_NAME               "djinterp-cli"
        C_VISIBILITY_PRESET       default
        WINDOWS_EXPORT_ALL_SYMBOLS ON)

    target_link_libraries(djinterp_cli PUBLIC ${CMAKE_DL_LIBS})
endif()

if(NOT TARGET djinterp::arg_parser)
    add_library(djinterp_arg_parser STATIC ${DJINTERP_ARG_PARSER_SOURCES})
    add_library(djinterp::arg_parser ALIAS djinterp_arg_parser)
    djinterp_cli_c_target(djinterp_arg_parser)
endif()

if(NOT TARGET djinterp::cli)
    add_library(djinterp_cli STATIC ${DJINTERP_CLI_SOURCES})
    add_library(djinterp::cli ALIAS djinterp_cli)
    djinterp_cli_c_target(djinterp_cli)

    # the loader of the modules, where the platform keeps it apart
    target_link_libraries(djinterp_cli PUBLIC
        djinterp::arg_parser
        ${CMAKE_DL_LIBS})
endif()


#-------------------------------------------------------------------------------
# modules
#-------------------------------------------------------------------------------

# d_cli_add_plugin(<target> SOURCES <source>...)
#   function: a module a program loads for one of its commands, exporting
# its entry point and nothing else.
function(d_cli_add_plugin _target)
    cmake_parse_arguments(PARSE_ARGV 1 _arg "" "" "SOURCES")

    add_library(${_target} MODULE ${_arg_SOURCES})

    set_target_properties(${_target} PROPERTIES
        PREFIX                    ""
        C_VISIBILITY_PRESET       hidden
        CXX_VISIBILITY_PRESET     hidden
        VISIBILITY_INLINES_HIDDEN ON
        POSITION_INDEPENDENT_CODE ON)

    target_link_libraries(${_target} PRIVATE djinterp::cli)

    # what the static libraries bring in stays inside; a shared core is the
    # program's, and nothing of it is the module's to export
    get_target_property(_kind djinterp::cli TYPE)

    if(APPLE)
        target_link_options(${_target} PRIVATE
            "LINKER:-exported_symbol,_d_cli_plugin_entry")
    elseif( (NOT WIN32)                                       AND
            (CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang)$")     AND
            (_kind STREQUAL "STATIC_LIBRARY") )
        target_link_options(${_target} PRIVATE "LINKER:--exclude-libs,ALL")
    endif()
endfunction()


#-------------------------------------------------------------------------------
# tests of a run
#-------------------------------------------------------------------------------

# d_cli_add_test(<name> COMMAND <program> [<argument>...] STATUS <status>
#                [MATCHES <regex>] [WORKING_DIRECTORY <directory>])
#   function: a test that runs a program as a user would, and passes when it
# ends with the status given and, with MATCHES, says something the
# expression matches, on its output or its errors. A program that is a
# target is run as add_test runs one: its file, through the emulator a cross
# build names. An argument cannot hold a semicolon.
function(d_cli_add_test _name)
    cmake_parse_arguments(PARSE_ARGV 1 _arg ""
        "STATUS;MATCHES;WORKING_DIRECTORY" "COMMAND")

    if( (NOT _arg_COMMAND) OR ("${_arg_STATUS}" STREQUAL "") )
        message(FATAL_ERROR
                "d_cli_add_test(${_name}): COMMAND and STATUS are required")
    endif()

    list(GET _arg_COMMAND 0 _program)
    list(REMOVE_AT _arg_COMMAND 0)

    set(_emulator "")

    if(TARGET ${_program})
        get_target_property(_emulator ${_program} CROSSCOMPILING_EMULATOR)

        if(NOT _emulator)
            set(_emulator "")
        endif()

        set(_program "$<TARGET_FILE:${_program}>")
    endif()

    get_property(_script GLOBAL PROPERTY D_CLI_TEST_SCRIPT)

    set(_where "")

    if(_arg_WORKING_DIRECTORY)
        set(_where WORKING_DIRECTORY "${_arg_WORKING_DIRECTORY}")
    endif()

    set(_run "${_emulator};${_program};${_arg_COMMAND}")

    add_test(NAME ${_name}
             COMMAND "${CMAKE_COMMAND}"
                     "-DD_CLI_TEST_COMMAND=${_run}"
                     "-DD_CLI_TEST_STATUS=${_arg_STATUS}"
                     "-DD_CLI_TEST_MATCHES=${_arg_MATCHES}"
                     -P "${_script}"
             ${_where})
endfunction()
