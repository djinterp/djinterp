################################################################################
# djinterp [test]                                                  CMakeLists.txt
#
#   Test leaf for the djinterp/core/util compress_options vocabulary.
#
#     compress_options_tests   the compress_options.hpp option vocabulary
#                              (recipe R1: header-only - the vocabulary is pure
#                              data and pulls in no backend, so nothing beyond
#                              the test sources is compiled or linked)
#
#   NOTE: this file used to define compress_tests as well, from back when both
# components shared one directory at .../core/util/.  That suite now has its own
# leaf at ../compress/, which owns compress_tests_runner.cpp - a runner this
# directory does not contain.  Keeping the block here duplicated the target name
# (fatal once util/ gains an aggregate) and named a source that does not exist.
#
# path: /build/cmake/config/testing/djinterp/core/util/compress_options/
#       CMakeLists.txt
################################################################################

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/util")

# compress_options.hpp is pure data (plain enums and POD-ish structs); the suite
# verifies its defaults, sentinels, enum values, value semantics, and layout,
# and links no production source and no backend libraries.
file(GLOB _compress_options_sources CONFIGURE_DEPENDS
     "${_tests_dir}/compress_options_tests_*.cpp")
list(FILTER _compress_options_sources EXCLUDE REGEX "_main\\.cpp$")

#   INCLUDES carries the bare-include roots this suite reaches for:
#   - inc/djinterp/core        -> "djinterp.hpp"             (NS_ macros; must
#                                                             precede the header)
#   - inc/djinterp/core/util   -> "compress_options.hpp"     (the header tested)
#   - tests/djinterp/core/util -> "compress_options_tests.hpp"
djinterp_add_test_executable(compress_options_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/compress_options_tests_runner.cpp"
        ${_compress_options_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core"
        "${DJINTERP_INC_ROOT}/djinterp/core/util"
        "${_tests_dir}")
