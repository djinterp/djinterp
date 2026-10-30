# =============================================================================
# testing/djinterp/core/event/event_handler/
#   LEAF: builds event_handler_tests - the exe for
# inc/djinterp/cpp/event/event_handler.hpp.  Header-only module (no src/), so
# this is AGENTS-cmake.md recipe R1: the runner here (the main() entry point)
# plus the section TUs, which live together under tests/djinterp/cpp/event/ and
# are pulled in by the event_handler_tests_* glob below (a legacy *_main.cpp, if
# any, is excluded - the runner in this dir is the entry point).
#
#   Two include dirs let the bare #include "..." spellings resolve:
#     - inc/djinterp/cpp/event    -> the headers under test
#     - tests/djinterp/cpp/event  -> the test header (event_handler_tests.hpp)
#   Output lands at bin/<arch>-<Config>/testing/djinterp/core/event/event_handler/.
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/event")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/event_handler_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(event_handler_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/event_handler_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/event"
        "${_tests_dir}")
