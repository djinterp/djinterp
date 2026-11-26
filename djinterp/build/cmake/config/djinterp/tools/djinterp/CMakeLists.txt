#*******************************************************************************
# djinterp [tools]                                              CMakeLists.txt
#
# The djinterp program's leaf: its 0.8 preview, on the command-line kit, and
# the runs that test it (docs/djinterp/interp/DESIGN.md, 6.4).
#   The program is its two sources under src/djinterp/tools/djinterp/ and
# the djin records (c/modules/interp/djin.c), linked to the kit as
# DjinterpCli.cmake makes it - the argument parser, the kit, filesys and
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
#                                                            revised: 2026.11.27
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
# the units of the tables below the log, compiled with the log
include("${DJINTERP_ROOT}/build/cmake/djinterp/log_codes.cmake")


#-------------------------------------------------------------------------------
# the program
#-------------------------------------------------------------------------------
set(_djinterp_sources "${DJINTERP_ROOT}/src/djinterp")

add_executable(djinterp
    "${_djinterp_sources}/tools/djinterp/djinterp_main.c"
    "${_djinterp_sources}/tools/djinterp/djinterp_commands.c"
    "${_djinterp_sources}/c/modules/interp/djin.c"
    "${_djinterp_sources}/c/modules/interp/djin_desc.c"
    # every core djin, by path, until each is a library of its own: parse's,
    # desc's, the environment printer's and the log's, and what they write
    # through - the option core's describer, desc's machines, the streams
    "${_djinterp_sources}/c/modules/parse/settings.c"
    "${_djinterp_sources}/c/desc/desc_machines.c"
    "${_djinterp_sources}/c/desc/desc_options.c"
    "${_djinterp_sources}/c/desc/desc_units.c"
    "${_djinterp_sources}/env/env_printer.c"
    "${_djinterp_sources}/c/log/log.c"
    "${_djinterp_sources}/c/log/log_writers.c"
    ${DJINTERP_LOG_CODES_SOURCES}
    "${_djinterp_sources}/c/option/option_env_desc.c"
    "${_djinterp_sources}/c/util/ostream/ostream.c"
    "${_djinterp_sources}/c/util/ostream/ostream_console.c"
    "${_djinterp_sources}/c/util/ostream/ostream_file.c"
    "${_djinterp_sources}/c/text/ostream_format.c"
    "${_djinterp_sources}/c/text/ostream_template.c"
    "${_djinterp_sources}/c/sync/dmutex.c"
    # parse's djin configures the expression core, which it hooks, and the
    # core's diagnostics and storage
    "${_djinterp_sources}/c/modules/parse/diagnostic.c"
    "${_djinterp_sources}/c/modules/parse/storage.c"
    "${_djinterp_sources}/c/memory/mem_source.c"
    "${_djinterp_sources}/c/memory/mem_common.c"
    "${_djinterp_sources}/c/modules/parse/expr/fold.c"
    "${_djinterp_sources}/c/modules/parse/expr/parse.c"
    "${_djinterp_sources}/c/modules/parse/expr/render.c"
    "${_djinterp_sources}/c/modules/parse/expr/scan.c"
    "${_djinterp_sources}/c/modules/parse/expr/signature.c"
    "${_djinterp_sources}/c/modules/parse/expr/term.c")
find_package(Threads)
target_link_libraries(djinterp PRIVATE djinterp::cli)

if(Threads_FOUND)
    target_link_libraries(djinterp PRIVATE Threads::Threads)
endif()
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
        "STATUS;MATCHES;SETUP;NEEDS;OUTPUT_FILE" "COMMAND")

    d_cli_add_test(${_name}
                   COMMAND     djinterp ${_arg_COMMAND}
                   STATUS      ${_arg_STATUS}
                   MATCHES     "${_arg_MATCHES}"
                   OUTPUT_FILE "${_arg_OUTPUT_FILE}")

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
    # the djin describers, on the program's own sources but its main and
    # commands, run by DTest
    get_target_property(_program_sources djinterp SOURCES)
    list(FILTER _program_sources EXCLUDE REGEX "tools/djinterp/djinterp_")
    add_executable(djinterp_djin_desc_tests
        ${_program_sources}
        "${DJINTERP_ROOT}/tests/djinterp/c/modules/interp/djin_desc_tests.c"
        "${_djinterp_sources}/c/modules/test/test_suite.c")
    target_link_libraries(djinterp_djin_desc_tests PRIVATE djinterp::cli)
    if(Threads_FOUND)
        target_link_libraries(djinterp_djin_desc_tests PRIVATE Threads::Threads)
    endif()
    set_target_properties(djinterp_djin_desc_tests PROPERTIES
        C_STANDARD          99
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS        OFF)
    if(NOT MSVC)
        target_compile_options(djinterp_djin_desc_tests PRIVATE
                               -Wall -Wextra -pedantic)
        if(UNIX AND NOT APPLE)
            target_compile_definitions(djinterp_djin_desc_tests PRIVATE
                                       _XOPEN_SOURCE=700)
        endif()
    endif()
    add_test(NAME djinterp_djin_desc COMMAND djinterp_djin_desc_tests)

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
    # every core djin is registered, its settings read as the kit's are;
    # parse's six rows, in their order: the expression core's three and its
    # diagnostics' three; the first three each read from every source, and
    # a value each refuses said as the kit says one
    djinterp_program_test(djinterp_list_core plain
        COMMAND list STATUS 0
        MATCHES "parse +- +settings only\ndesc +- +settings only\nenv_printer +- +settings only\nlog +- +settings only\n")
    djinterp_program_test(djinterp_describe_parse plain
        COMMAND describe parse STATUS 0
        MATCHES "\nsettings\n  parse[.]expr_max_depth +int +1000 +D_CFG_PARSE_EXPR_MAX_DEPTH +the depth guard: 1 to 65535, or unbounded\n  parse[.]expr_max_errors +int +16 +D_CFG_PARSE_EXPR_MAX_ERRORS +errors before a parse stops: 1 to 65535\n  parse[.]expr_error_order +choice +leftmost +D_CFG_PARSE_EXPR_ERROR_ORDER +which error is the error: leftmost or structure_first\n  parse[.]diag_text +bool +true +D_CFG_PARSE_DIAG_TEXT +whether diagnostics carry their text; on only where the build has it [(]D_CFG_PARSE_DIAG_TEXT[)]\n  parse[.]diag_facts +bool +true +D_CFG_PARSE_DIAG_FACTS +whether diagnostics carry their facts; on only where the build has them [(]D_CFG_PARSE_DIAG_FACTS[)]\n  parse[.]log +bool +true +D_CFG_PARSE_LOG +whether parse's diagnostics are written to the log; on only where the build has parse's log unit [(]D_CFG_PARSE_LOG[)]\n$")
    djinterp_program_test(djinterp_get_parse plain
        COMMAND --parse.expr_error_order=structure_first get
                parse.expr_max_errors parse.expr_error_order
        STATUS 0
        MATCHES "^parse[.]expr_max_errors = 16\nparse[.]expr_error_order = structure_first\n$")
    djinterp_program_test(djinterp_parse_named plain
        COMMAND --parse.expr_max_depth=unbounded get --show-origin
                parse.expr_max_depth
        STATUS 0
        MATCHES "^parse[.]expr_max_depth = unbounded  [(]command line[)]\n")
    djinterp_program_test(djinterp_parse_range plain
        COMMAND --parse.expr_max_errors=0 list STATUS 2
        MATCHES "^djinterp: error: --parse[.]expr_max_errors: '0' is not a value 'parse[.]expr_max_errors' takes\n")
    djinterp_program_test(djinterp_parse_choice plain
        COMMAND --parse.expr_error_order=first list STATUS 2
        MATCHES "^djinterp: error: --parse[.]expr_error_order: 'first' is not a value 'parse[.]expr_error_order' takes\n")
    djinterp_program_test(djinterp_get_core plain
        COMMAND --parse.expr_max_depth=7 get --show-origin
                parse.expr_max_depth
        STATUS 0
        MATCHES "^parse[.]expr_max_depth = 7  [(]command line[)]\n")

    # describe as JSON, of the shape it names first; a format it does not
    # write, and a command a djin does not have, refused
    djinterp_program_test(djinterp_describe_json plain
        COMMAND describe cli --format=json STATUS 0
        MATCHES "^[{]\n  \"format\": \"djinterp[.]describe/1[.]0\",\n  \"name\": \"cli\",\n  \"settings\": [{]\n.*\"plugin_strict_version\": [{]\n +\"value\": true,\n +\"source\": \"default\",.*\n[}]\n$")
    djinterp_program_test(djinterp_describe_format plain
        COMMAND describe cli --format=yaml STATUS 2
        MATCHES "describe: 'yaml' is not a format: text or json")
    djinterp_program_test(djinterp_describe_command plain
        COMMAND describe parse run STATUS 2
        MATCHES "describe: 'parse' has no command 'run'")

    # the description, as JSON, for the man pages and the wiki: the kit's
    # members, a core djin's setting among them, then the program's own
    djinterp_program_test(djinterp_help_json plain
        COMMAND help --format=json STATUS 0
        MATCHES "^[{]\n  \"format\": \"djinterp[.]help/1[.]0\",\n  \"program\": \"djinterp\",\n.*\"name\": \"parse[.]expr_max_depth\",\n.*  \"djins\": [[]\n.*\"name\": \"cli\",\n      \"settings_only\": true\n.*  \"reserved\": [[]\n    \"list\",\n.*\n  []]\n[}]\n$")
    djinterp_program_test(djinterp_help_json_command plain
        COMMAND help set --format=json STATUS 0
        MATCHES "  \"command\": [{]\n    \"name\": \"set\",\n    \"path\": \"djinterp set\",\n")

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

    # a name whose first part names no djin, said so, by set and by the
    # command line, the latter through the argument parser's setting hook
    djinterp_program_test(djinterp_set_djin plain
        COMMAND set nosuch.depth=1 STATUS 2
        MATCHES "error: no djin is named 'nosuch'")
    djinterp_program_test(djinterp_setting_djin plain
        COMMAND --nosuch.depth=1 list STATUS 2
        MATCHES "^djinterp: error: --nosuch[.]depth: no djin is named 'nosuch'\n")
    djinterp_program_test(djinterp_setting_after plain
        COMMAND list --parse.expr_max_depth=7 STATUS 0
        MATCHES "parse +- +settings only")

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

    # the file's sections: another program's passed over, the program's
    # own read; every program's lines written before the first header, and
    # the program's section after them said; the file read without
    # sections, its headers said so, and still written in its sections.
    # The home starts empty at each run of the suite, not at configuring
    # alone.
    add_test(NAME djinterp_section_fresh
             COMMAND "${CMAKE_COMMAND}" -E remove_directory
                     "${CMAKE_CURRENT_BINARY_DIR}/homes/sections")
    set_tests_properties(djinterp_section_fresh PROPERTIES
        FIXTURES_SETUP djinterp_section_fresh)
    djinterp_program_test(djinterp_section_other sections
        COMMAND set --section=vparse cli.plugin_strict_version=false
        STATUS 0
        NEEDS djinterp_section_fresh SETUP djinterp_section_a)
    djinterp_program_test(djinterp_section_passed sections
        COMMAND get --show-origin cli.plugin_strict_version STATUS 0
        MATCHES "^cli[.]plugin_strict_version = true  [(]D_CFG_"
        NEEDS djinterp_section_a SETUP djinterp_section_b)
    djinterp_program_test(djinterp_section_own sections
        COMMAND set --section=djinterp cli.plugin_strict_version=false
        STATUS 0
        NEEDS djinterp_section_b SETUP djinterp_section_c)
    djinterp_program_test(djinterp_section_read sections
        COMMAND get --show-origin cli.plugin_strict_version STATUS 0
        MATCHES "^cli[.]plugin_strict_version = false  [(]file[)]"
        NEEDS djinterp_section_c SETUP djinterp_section_d)
    djinterp_program_test(djinterp_section_shared sections
        COMMAND set cli.plugin_strict_version=true STATUS 0
        MATCHES "settings:3: warning: this line sets 'cli[.]plugin_strict_version' too, and [[]vparse[]] reads it after the new one\n.*settings:6: warning: this line sets 'cli[.]plugin_strict_version' too, and [[]djinterp[]] reads it after the new one"
        NEEDS djinterp_section_d SETUP djinterp_section_e)
    djinterp_program_test(djinterp_section_flat sections
        COMMAND get cli.plugin_strict_version STATUS 0
        MATCHES "settings:2: warning: '[[]vparse[]]' is a section's header, and this run reads the file without sections"
        NEEDS djinterp_section_e SETUP djinterp_section_f)
    set_property(TEST djinterp_section_flat APPEND PROPERTY
        ENVIRONMENT "DJINTERP_CLI_SETTINGS_SECTIONS=false")
    # a run that reads the file as one part still writes in its sections:
    # [vparse]'s line replaced, and every program's lines left alone
    djinterp_program_test(djinterp_section_flat_set sections
        COMMAND set --section=vparse cli.plugin_strict_version=true STATUS 0
        NEEDS djinterp_section_f SETUP djinterp_section_g)
    set_property(TEST djinterp_section_flat_set APPEND PROPERTY
        ENVIRONMENT "DJINTERP_CLI_SETTINGS_SECTIONS=false")
    djinterp_program_test(djinterp_section_flat_kept sections
        COMMAND get --show-origin cli.plugin_strict_version STATUS 0
        MATCHES "^cli[.]plugin_strict_version = false  [(]file[)]\n"
        NEEDS djinterp_section_g)
    djinterp_program_test(djinterp_section_name plain
        COMMAND set --section=a/b cli.plugin_strict_version=true STATUS 2
        MATCHES "set: --section: 'a/b' is not a program's name")

    # an output that refuses what a command writes, said with its reason
    if(EXISTS "/dev/full")
        foreach(_command IN ITEMS list describe get json help)
            set(_arguments ${_command})

            if(_command STREQUAL "describe")
                set(_arguments describe cli)
            elseif(_command STREQUAL "json")
                set(_arguments describe cli --format=json)
            elseif(_command STREQUAL "help")
                set(_arguments help --format=json)
            endif()

            djinterp_program_test(djinterp_${_command}_full plain
                COMMAND ${_arguments} STATUS 2 OUTPUT_FILE "/dev/full"
                MATCHES "^djinterp: error: cannot write standard output: No space left on device\n$")
        endforeach()
    endif()
endif()
