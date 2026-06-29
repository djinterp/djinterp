# =============================================================================
# testing/djinterp/test/test_common/
#   LEAF: builds test_common_tests - the exe for inc/djinterp/test/test_common.hpp.
#
#   This directory holds only the runner (the main() entry point); the suite's
# section translation units live together under tests/djinterp/test/, and are
# pulled in by the glob below (the *_main.cpp hand-runner is excluded - the
# runner here is the entry point).  Two include dirs let the sources' bare
# #include "..." spellings resolve:
#     - inc/djinterp/test    -> the header under test (test_common.hpp)
#     - tests/djinterp/test  -> the test header        (test_common_tests.hpp)
# Output lands at bin/<arch>-<Config>/testing/djinterp/test/test_common/.
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/test")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/test_common_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(test_common_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_common_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/test"
        "${_tests_dir}")
