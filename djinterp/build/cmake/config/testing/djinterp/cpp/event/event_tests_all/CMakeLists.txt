# =============================================================================
# testing/djinterp/core/event/event_tests_all/
#   LEAF: builds event_tests_all - a single exe that runs EVERY event-subsystem
# test in one process and one report (one module per suite).  Unlike the
# per-component leaves, this globs the section TUs of ALL five suites
# (event_common / event_handler / event_table / event_registry /
# event_dispatcher) from tests/djinterp/cpp/event/ and links them under the
# union runner here.
#
#   The runner (event_tests_all_runner.cpp) forward-declares the tests_* entry
# points and includes NO suite test header - several suites define
# identically-named djinterp::testing fixtures (e.g. ev_int, summing), so
# pulling more than one header into a single TU would redefine them.  The
# entry-point names are unique across all suites, so they resolve at link time
# against the globbed section objects.  (This runner needs only inc/ - supplied
# by the helper - for djinterp/test/test_report_runner.hpp.)
#
#   Output lands at
# bin/<arch>-<Config>/testing/djinterp/core/event/event_tests_all/.
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/event")

# every event suite's section TUs (all five components share this flat dir); the
# union runner lives here in the config tree, not under tests/, so this glob
# never matches it (no duplicate main()).  A legacy *_main.cpp, if any, drops.
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/event_*_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(event_tests_all
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/event_tests_all_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/event"
        "${_tests_dir}")
