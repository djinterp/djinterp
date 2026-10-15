#*******************************************************************************
# bitwisedom [tests]                                        check_bitfield.cmake
#
# One bitfield file, there and back: bitwisedom-gen reads it and writes it
# again as a bitfield, and the two must be the same bytes. A file the
# window wrote, or a script that lays its JSON out as the window does
# (tools/bitfield_json.py), passes; a key the reader lost, or one it
# changed, does not. Run by CTest with cmake -P; prints "<file> is the
# bitfield it writes back" when it is. A failure never says so: CTest passes
# a test by its output alone.
#   A checkout that turned the file's line ends into CRLFs has not changed
# the bitfield, so line ends are not compared.
#
#     GEN FILE WORK, and EMULATOR: what runs GEN where this machine cannot
#     -- in a cross build, CMAKE_CROSSCOMPILING_EMULATOR; empty otherwise
#
# path:      /tests/djinterp/tools/bitwisedom/check_bitfield.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
#                                                            revised: 2026.10.11
#*******************************************************************************
file(MAKE_DIRECTORY "${WORK}")

get_filename_component(_name "${FILE}" NAME)
set(_written "${WORK}/${_name}")

execute_process(
    COMMAND ${EMULATOR} "${GEN}" "${FILE}" --bitfield "${_written}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _output
    ERROR_VARIABLE  _errors)

if(NOT _result EQUAL 0)
    message(FATAL_ERROR "bitwisedom-gen failed (${_result}):\n${_errors}")
endif()

file(READ "${FILE}" _before)
file(READ "${_written}" _after)
string(REPLACE "\r\n" "\n" _before "${_before}")
string(REPLACE "\r\n" "\n" _after "${_after}")

if(NOT _before STREQUAL _after)
    string(LENGTH "${_before}" _read)
    string(LENGTH "${_after}" _wrote)
    message(FATAL_ERROR
            "${_name} differs from what is written back of it: ${_read} "
            "bytes read, ${_wrote} written. Compare it with ${_written}.")
endif()

message("${_name} is the bitfield it writes back")
