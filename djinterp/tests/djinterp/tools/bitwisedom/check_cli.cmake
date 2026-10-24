#*******************************************************************************
# bitwisedom [tests]                                             check_cli.cmake
#
# One transcript of the command line, run and compared: each command it holds
# is run in a work directory of its own, with what it reads on standard
# input, and what it writes on standard output and on standard error, and
# the status it exits with, must be what the transcript says -- every line,
# in order, and nothing more. Run by CTest with cmake -P; prints
# "transcript passed" when every command did. A failure never says so: CTest
# passes a test by its output alone.
#   A transcript is lines of text, each starting with what it is:
#
#     # TEXT        a comment; so is a line with nothing on it
#     $ COMMAND     a command: bitwisedom or bitwisedom-gen and its arguments,
#                   split as a POSIX shell splits them, quotes and all
#     < TEXT        a line of the command's standard input
#     > TEXT        a line of its standard output; `>` alone, an empty one
#     ! TEXT        a line of its standard error; `!` alone, an empty one
#     ? STATUS      the status it exits with; 0 when the command says none
#     + NAME        a file made in the work directory, before the command
#                   after it, of the `| TEXT` lines that follow
#
# The lines after a command, up to the next command or file, are its. The
# terminal is said to be 80 columns wide, so help is wrapped as the
# transcript has it, whatever runs the test.
#
#     BITWISEDOM BITWISEDOM_GEN TRANSCRIPT WORK, and EMULATOR: what runs the
#     programs where this machine cannot -- in a cross build,
#     CMAKE_CROSSCOMPILING_EMULATOR; empty otherwise
#
# path:      /tests/djinterp/tools/bitwisedom/check_cli.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
#                                                            revised: 2026.10.22
#*******************************************************************************
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")

set(_input "${WORK}.stdin")
set(_failures "")
set(_commands 0)

#[[
cli_run
  The command held -- its arguments, its input and what is expected of it --
run and compared; each difference added to the failures.
]]
macro(cli_run)
    separate_arguments(_args UNIX_COMMAND "${_command}")
    list(GET _args 0 _program)
    list(REMOVE_AT _args 0)

    if(_program STREQUAL "bitwisedom")
        set(_executable "${BITWISEDOM}")
    elseif(_program STREQUAL "bitwisedom-gen")
        set(_executable "${BITWISEDOM_GEN}")
    else()
        message(FATAL_ERROR "${TRANSCRIPT}: '${_program}' is not a program "
                            "the transcripts run")
    endif()

    file(WRITE "${_input}" "${_stdin}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env COLUMNS=80
                ${EMULATOR} "${_executable}" ${_args}
        WORKING_DIRECTORY "${WORK}"
        INPUT_FILE        "${_input}"
        RESULT_VARIABLE   _status
        OUTPUT_VARIABLE   _out
        ERROR_VARIABLE    _err)
    math(EXPR _commands "${_commands} + 1")

    if( (NOT _out STREQUAL _expected_out) OR
        (NOT _err STREQUAL _expected_err) OR
        (NOT _status STREQUAL _expected_status) )
        string(APPEND _failures
               "\n$ ${_command}\n"
               "-- status: expected ${_expected_status}, was ${_status}\n"
               "-- standard output, expected:\n${_expected_out}"
               "-- standard output, written:\n${_out}"
               "-- standard error, expected:\n${_expected_err}"
               "-- standard error, written:\n${_err}")
    endif()

    set(_command "")
endmacro()

#[[
cli_file
  The file held, written in the work directory.
]]
macro(cli_file)
    file(WRITE "${WORK}/${_file}" "${_file_text}")
    set(_file "")
endmacro()

file(READ "${TRANSCRIPT}" _text)
string(REPLACE "\r\n" "\n" _text "${_text}")

set(_command "")
set(_file "")

# one line at a time, never as a list, so that a `;` is text
while(NOT _text STREQUAL "")
    string(FIND "${_text}" "\n" _end)

    if(_end LESS 0)
        set(_line "${_text}")
        set(_text "")
    else()
        string(SUBSTRING "${_text}" 0 ${_end} _line)
        math(EXPR _next "${_end} + 1")
        string(SUBSTRING "${_text}" ${_next} -1 _text)
    endif()

    string(SUBSTRING "${_line}" 0 1 _kind)
    string(LENGTH "${_line}" _length)
    set(_rest "")

    if(_length GREATER 2)
        string(SUBSTRING "${_line}" 2 -1 _rest)
    endif()

    # a file's line, or the end of the file
    if(NOT _file STREQUAL "")
        if(_kind STREQUAL "|")
            string(APPEND _file_text "${_rest}\n")

            continue()
        endif()

        cli_file()
    endif()

    if( (_kind STREQUAL "$") OR (_kind STREQUAL "+") )
        if(NOT _command STREQUAL "")
            cli_run()
        endif()
    endif()

    if(_kind STREQUAL "$")
        set(_command "${_rest}")
        set(_stdin "")
        set(_expected_out "")
        set(_expected_err "")
        set(_expected_status 0)
    elseif(_kind STREQUAL "+")
        set(_file "${_rest}")
        set(_file_text "")
    elseif(_kind STREQUAL "<")
        string(APPEND _stdin "${_rest}\n")
    elseif(_kind STREQUAL ">")
        string(APPEND _expected_out "${_rest}\n")
    elseif(_kind STREQUAL "!")
        string(APPEND _expected_err "${_rest}\n")
    elseif(_kind STREQUAL "?")
        set(_expected_status "${_rest}")
    elseif( (NOT _kind STREQUAL "#") AND (NOT _kind STREQUAL "") )
        message(FATAL_ERROR "${TRANSCRIPT}: a line that is nothing a "
                            "transcript holds: ${_line}")
    endif()
endwhile()

if(NOT _file STREQUAL "")
    cli_file()
endif()

if(NOT _command STREQUAL "")
    cli_run()
endif()

if(NOT _failures STREQUAL "")
    message(FATAL_ERROR "${TRANSCRIPT}: commands differ:${_failures}")
endif()

message("transcript passed: ${_commands} commands, ${TRANSCRIPT}")
