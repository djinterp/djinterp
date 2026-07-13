# ============================================================================
# djinterp/core/sync - test leaf: atomic.hpp
#
#   Builds the `atomic_tests` executable for inc/djinterp/core/sync/atomic.hpp.
# atomic.hpp is header-only (no src/djinterp/core/sync/*.cpp), so this follows
# Recipe R1 from AGENTS-cmake.md.
#
#   Layout (all four roots share the slug `djinterp/core/sync`):
#     header under test : inc/djinterp/core/sync/atomic.hpp
#     test header       : tests/djinterp/core/sync/atomic_tests.hpp
#     section TUs       : tests/djinterp/core/sync/atomic_tests_*.cpp
#     runner (main)     : <this dir>/atomic_tests_runner.cpp
#     built exe         : bin/<arch>-<Config>/testing/djinterp/core/sync/atomic_tests
#
#   INCLUDES gives the leaf both bare-include roots the test sources need:
#     - inc/djinterp/core/sync  -> the test header's  #include "atomic.hpp"
#     - tests/djinterp/core/sync -> the runner's/sections' #include "atomic_tests.hpp"
#   (inc/ itself is added by the helper, which is how the runner resolves
#    #include <djinterp/test/output/test_report_runner.hpp>.)
#
#   The helper supplies D_TESTING=1, links djinterp_c_runtime, sets the per-
#   config output dir, and registers CTest - do not set those by hand.
#
#   Aggregate (optional): to expose a group target over the sync test leaves,
#   drop a one-line `djinterp_add_test_subtree(all_djinterp_core_sync)` at a
#   chosen level (this dir may do both per AGENTS-cmake.md section 5).  It is
#   left out here to avoid claiming a global aggregate name the wider tree may
#   already define.
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/sync")

# section translation units (one per like-group section of atomic.hpp)
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/atomic_tests_*.cpp")

# defensively drop any legacy *_main.cpp so it can never collide with the
# runner's main() at link time
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(atomic_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/atomic_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/sync"
        "${_tests_dir}")
