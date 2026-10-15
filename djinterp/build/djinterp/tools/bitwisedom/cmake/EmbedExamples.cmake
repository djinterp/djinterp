#*******************************************************************************
# bitwisedom [cmake]                                         EmbedExamples.cmake
#
# The large examples, built in: each res/djinterp/tools/bitwisedom/examples/large/<name>.bitfield.json
# becomes an array of its bytes, LARGE_<NAME>, in large_examples.inc, which
# src/examples.cpp includes and reads as it would any bitfield. The examples
# stay JSON in the tree -- to read, to diff, and to open as files -- and the
# window and bitwisedom-gen have them without a file beside them.
#   The .inc is written as CMake configures, into the build tree, and only
# when what it holds changes, so a configure that changes nothing compiles
# nothing again. A change to a JSON file configures again at the next build.
# A string literal would be shorter, but MSVC holds no literal past 64 KB;
# an array of bytes has no such limit.
#
#     BITWISEDOM_GENERATED_DIR   where large_examples.inc is written; the
#                                core adds it to its include path
#
# path:      /build/djinterp/tools/bitwisedom/cmake/EmbedExamples.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
#                                                            revised: 2026.10.11
#*******************************************************************************
include_guard(GLOBAL)

set(BITWISEDOM_GENERATED_DIR "${CMAKE_BINARY_DIR}/bitwisedom_generated")

# bitwisedom_embed_examples(<directory>)
#   Writes large_examples.inc from the directory's *.bitfield.json, sorted
# by name, and asks CMake to configure again when any of them changes.
function(bitwisedom_embed_examples _directory)
    file(GLOB _files LIST_DIRECTORIES false
         "${_directory}/*.bitfield.json")
    list(SORT _files)

    string(CONCAT _text
        "// large_examples.inc: written by cmake/EmbedExamples.cmake from\n"
        "// examples/large (under res/djinterp/tools/bitwisedom); edit JSON.\n"
        "\n")

    foreach(_file ${_files})
        get_filename_component(_name "${_file}" NAME)
        string(REGEX REPLACE "[.]bitfield[.]json$" "" _stem "${_name}")
        string(MAKE_C_IDENTIFIER "LARGE_${_stem}" _array)
        string(TOUPPER "${_array}" _array)

        # the bytes, sixteen to a line (CMake's expressions count no
        # repeats, so the sixteen are spelt out)
        file(READ "${_file}" _hex HEX)
        string(LENGTH "${_hex}" _digits)
        math(EXPR _bytes "${_digits} / 2")
        string(REPEAT "0x..," 16 _line)
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _hex "${_hex}")
        string(REGEX REPLACE "(${_line})" "\\1\n    " _hex "${_hex}")
        string(REGEX REPLACE ",(\n    )?$" "" _hex "${_hex}")

        string(APPEND _text
            "// ${_name}: ${_bytes} bytes\n"
            "const unsigned char ${_array}[] = {\n"
            "    ${_hex}\n"
            "};\n\n")

        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                     "${_file}")
    endforeach()

    # a new or removed file configures again too
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                 "${_directory}")

    file(MAKE_DIRECTORY "${BITWISEDOM_GENERATED_DIR}")
    file(WRITE "${BITWISEDOM_GENERATED_DIR}/large_examples.inc.new"
         "${_text}")
    file(COPY_FILE "${BITWISEDOM_GENERATED_DIR}/large_examples.inc.new"
                   "${BITWISEDOM_GENERATED_DIR}/large_examples.inc"
                   ONLY_IF_DIFFERENT)
    file(REMOVE "${BITWISEDOM_GENERATED_DIR}/large_examples.inc.new")
endfunction()
