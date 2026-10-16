# =============================================================================
# testing/djinterp/core/util/counter/counter/
#   LEAF (recipe R1, header-only) — builds counter_tests.
#
#   The runner (counter_tests_runner.cpp, this dir) owns main() and drives a
# report_builder; the section TUs globbed from the tests tree define the
# ::djinterp::testing::tests_counter_* functions it runs.  counter.hpp pulls in
# no backend, so nothing beyond the runner + section TUs is compiled or linked
# (report_builder's archive packaging is opt-in via D_TEST_REPORT_ENABLE_ARCHIVE
# and stays inert here — the console + single-PDF path has no archive
# dependency).
#
#   INCLUDES:
#     inc/djinterp/core/util/counter   -> bare  #include "counter.hpp"
#     tests/djinterp/core/util/counter -> bare  #include "counter_tests.hpp"
#   The helper adds inc/ for <djinterp/core/djinterp.hpp> and the djinterp/test/*
# headers, links djinterp_c_runtime, sets the per-config output dir, and
# registers the exe with CTest (run it with `ctest -R counter_tests`).  Any
# ancestor djinterp_add_test_subtree picks this leaf up automatically.
#
#   NOTE: the runner lives one level below the util/counter aggregate (the leaf
# dir shares the component's name, matching the paradigm/<name>/<name>/ and
# memory/pool/<name>/ layout); the section TUs live one level up, directly under
# tests/djinterp/core/util/counter, hence _tests_dir drops the trailing repeat.
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/util/counter")

# section TUs (one per surface).  Exclude any stray *_main.cpp so it can never
# collide with the runner's main() at link time.
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/counter_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(counter_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/counter_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/util/counter"
        "${_tests_dir}")
