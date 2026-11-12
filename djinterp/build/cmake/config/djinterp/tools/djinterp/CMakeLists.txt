#*******************************************************************************
# djinterp [tools]                                              CMakeLists.txt
#
# The djinterp program's leaf: its 0.8 preview, on the command-line kit, and
# the runs that test it (docs/djinterp/interp/DESIGN.md, 6.4).
#   The program is its two sources under src/djinterp/tools/djinterp/ and
# the djin records (c/modules/interp/djin.c), linked to the kit as
# DjinterpCli.cmake makes it -- the argument parser, the kit, filesys and
# the option core the run's settings are kept in. Each test runs the program
# as a user would, through d_cli_add_test, with a home of its own, so that
# no user's settings file is read or written: the `set` runs write theirs
# and the runs after them read it.
#   Configure it on its own, or add it from a project that has C enabled:
#
#     cmake -S build/cmake/config/djinterp/tools/djinterp -B build/djinterp
#     cmake --build build/djinterp && ctest --test-dir build/djinterp
#
# path:      /build/cmake/config/djinterp/tools/djinterp/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
#                                                            revised: 2026.11.13
#*******************************************************************************
cmake_minimum_required(VERSION 3.16)

project(djinterp_program
    VERSION     0.8.0
    DESCRIPTION "The djinterp language's command line: its djins and their settings"
    LANGUAGES   C)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    enable_testing()
endif()

option(DJINTERP_PROGRAM_BUILD_TESTS "Build the djinterp program's tests" ON)

if(NOT DJINTERP_ROOT)
    get_filename_component(DJINTERP_ROOT
                           "${CMAKE_CURRENT_LIST_DIR}/../../../../../.."
                           ABSOLUTE)
endif()

# the kit, and the argument parser it is made on
include("${DJINTERP_ROOT}/build/cmake/config/djinterp/tools/cli/DjinterpCli.cmake")


#-------------------------------------------------------------------------------
# the program
#-------------------------------------------------------------------------------
set(_djinterp_sources "${DJINTERP_ROOT}/src/djinterp")

add_executable(djinterp
    "${_djinterp_sources}/tools/djinterp/djinterp_main.c"
    "${_djinterp_sources}/tools/djinterp/djinterp_commands.c"
    "${_djinterp_sources}/c/modules/interp/djin.c")
target_link_libraries(djinterp PRIVATE djinterp::cli)
set_target_properties(djinterp PROPERTIES
    C_STANDARD          99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS        OFF
    FOLDER              "djinterp")

if(MSVC)
    target_compile_options(djinterp PRIVATE /utf-8)
    target_compile_definitions(djinterp PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(djinterp PRIVATE -Wall -Wextra -pedantic)

    if(UNIX AND NOT APPLE)
        target_compile_definitions(djinterp PRIVATE _XOPEN_SOURCE=700)
    endif()
endif()


#-------------------------------------------------------------------------------
# the runs
#-------------------------------------------------------------------------------

# djinterp_program_test
#   function: a run of the program, given a home of its own, `_home`: the
# runs of one home share the user's settings file, in the order ctest runs
# them, which FIXTURES keeps.
function(djinterp_program_test _name _home)
    cmake_parse_arguments(PARSE_ARGV 2 _arg ""
        "STATUS;MATCHES;SETUP;NEEDS" "COMMAND")

    d_cli_add_test(${_name}
                   COMMAND djinterp ${_arg_COMMAND}
                   STATUS  ${_arg_STATUS}
                   MATCHES "${_arg_MATCHES}")

    set(_folder "${CMAKE_CURRENT_BINARY_DIR}/homes/${_home}")

    set_tests_properties(${_name} PROPERTIES
        ENVIRONMENT "XDG_CONFIG_HOME=${_folder};APPDATA=${_folder};HOME=${_folder};DJINTERP_CLI_PLUGIN_STRICT_VERSION=;DJINTERP_PLUGIN_PATH=")

    if(_arg_SETUP)
        set_tests_properties(${_name} PROPERTIES
            FIXTURES_SETUP "${_arg_SETUP}")
    endif()

    if(_arg_NEEDS)
        set_tests_properties(${_name} PROPERTIES
            FIXTURES_REQUIRED "${_arg_NEEDS}")
    endif()
endfunction()

if(DJINTERP_PROGRAM_BUILD_TESTS)
    # each test's home starts empty
    file(REMOVE_RECURSE "${CMAKE_CURRENT_BINARY_DIR}/homes")

    djinterp_program_test(djinterp_version plain
        COMMAND --version STATUS 0
        MATCHES "^djinterp [0-9]")
    djinterp_program_test(djinterp_list plain
        COMMAND list STATUS 0
        MATCHES "cli +- +settings only")
    djinterp_program_test(djinterp_describe plain
        COMMAND describe cli STATUS 0
        MATCHES "cli[.]plugin_strict_version +bool +true +D_CFG_CLI_PLUGIN_STRICT_VERSION")
    djinterp_program_test(djinterp_describe_none plain
        COMMAND describe nosuch STATUS 2
        MATCHES "describe: no djin is named 'nosuch'")
    djinterp_program_test(djinterp_get plain
        COMMAND get cli.plugin_strict_version STATUS 0
        MATCHES "^cli[.]plugin_strict_version = true\n$")
    djinterp_program_test(djinterp_get_every plain
        COMMAND get STATUS 0
        MATCHES "cli[.]plugin_strict_version = true")
    djinterp_program_test(djinterp_get_origin plain
        COMMAND --cli.plugin_strict_version=false get --show-origin
                cli.plugin_strict_version
        STATUS 0
        MATCHES "= false  [(]command line[)]\n    default    true   [(]D_CFG_CLI_PLUGIN_STRICT_VERSION[)]")
    djinterp_program_test(djinterp_get_none plain
        COMMAND get cli.nosuch STATUS 2
        MATCHES "error: no setting is named 'cli.nosuch'")
    djinterp_program_test(djinterp_set_refused plain
        COMMAND set cli.plugin_strict_version=maybe STATUS 2
        MATCHES "'maybe' is not a value 'cli.plugin_strict_version' takes")
    djinterp_program_test(djinterp_set_shape plain
        COMMAND set nothing STATUS 2
        MATCHES "set: 'nothing' is not NAME=VALUE")

    # set, then read back from the file; then set again over it
    djinterp_program_test(djinterp_set written
        COMMAND set cli.plugin_strict_version=false STATUS 0
        SETUP djinterp_written)
    djinterp_program_test(djinterp_set_read written
        COMMAND get --show-origin cli.plugin_strict_version STATUS 0
        MATCHES "^cli[.]plugin_strict_version = false  [(]file[)]"
        NEEDS djinterp_written SETUP djinterp_read)
    djinterp_program_test(djinterp_set_again written
        COMMAND set cli.plugin_strict_version=true STATUS 0
        NEEDS djinterp_read SETUP djinterp_again)
    djinterp_program_test(djinterp_set_replaced written
        COMMAND get --show-origin cli.plugin_strict_version STATUS 0
        MATCHES "^cli[.]plugin_strict_version = true  [(]file[)]\n    default"
        NEEDS djinterp_again)

    # the user's file under the command line
    djinterp_program_test(djinterp_set_under written
        COMMAND --no-cli.plugin_strict_version get --show-origin
                cli.plugin_strict_version
        STATUS 0
        MATCHES "= false  [(]command line[)]\n    under it   true   [(]file[)]"
        NEEDS djinterp_again)
endif()
