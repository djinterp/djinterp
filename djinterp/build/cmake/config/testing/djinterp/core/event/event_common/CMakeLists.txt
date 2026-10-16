# =============================================================================
# testing/djinterp/core/event/event_common/
#   LEAF: builds event_common_tests - the exe for
# inc/djinterp/core/event/event_common.hpp.  Header-only module (no src/), so this
# is AGENTS-cmake.md recipe R1: the runner here (the main() entry point) plus
# the section TUs, which live together under tests/djinterp/core/event/ and are
# pulled in by the event_common_tests_* glob below (a legacy *_main.cpp, if any, is
# excluded - the runner in this dir is the entry point).
#
#   Two include dirs let the bare #include "..." spellings resolve:
#     - inc/djinterp/core/event    -> the headers under test
#     - tests/djinterp/core/event  -> the test header (event_common_tests.hpp)
#   Output lands at bin/<arch>-<Config>/testing/djinterp/core/event/event_common/.
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/event")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/event_common_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(event_common_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/event_common_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/event"
        "${_tests_dir}")
