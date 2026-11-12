# =============================================================================
# testing/djinterp/core/util/timer/timer/
#   LEAF (recipe R1, header-only) — builds timer_tests.
#
#   The runner (timer_tests_runner.cpp, this dir) owns main() and drives a
# report_builder; the section TUs globbed from the tests tree define the
# ::djinterp::testing::tests_timer_* functions it runs.  timer.hpp pulls in no
# backend; the parity tests compare it with the C timer, so that timer's
# sources (c/util/timer/timer.c, with counter.c and dtime.c, which it reads)
# are compiled beside the runner and the section TUs, and nothing else
# (report_builder's archive packaging is opt-in via D_TEST_REPORT_ENABLE_ARCHIVE
# and stays inert here).
#
#   INCLUDES:
#     inc/djinterp/cpp/util/timer   -> bare  #include "timer.hpp"
#     tests/djinterp/cpp/util/timer -> bare  #include "timer_tests.hpp"
#   The helper adds inc/ for <djinterp/core/djinterp.hpp> and the djinterp/test/*
# headers, links djinterp_c_runtime, sets the per-config output dir, and
# registers the exe with CTest (run it with `ctest -R timer_tests`).  Any
# ancestor djinterp_add_test_subtree picks this leaf up automatically.
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/util/timer")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/timer_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(timer_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/timer_tests_runner.cpp"
        ${_sources}
        # the C timer, for the parity tests, and what it reads: counter.h's
        # arithmetic and dtime.h's clock
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/timer/timer.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/counter/counter.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/dtime.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/util/timer"
        "${_tests_dir}")
