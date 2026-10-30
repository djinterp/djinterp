# ============================================================================
# djinterp — build wiring for the dtuple_wrap_partition.hpp test suite
#
#   Component: dtuple_wrap_partition  (header-only, no src/)
#   Slug     : djinterp/core/meta
#            (header at inc/djinterp/cpp/meta/dtuple_wrap_partition.hpp)
#
#   AGENTS-cmake.md R1 recipe (header-only module, single test leaf). Builds
# the `dtuple_wrap_partition_tests` executable from the runner in THIS config
# directory plus the section TUs under tests/. The helper adds only inc/
# automatically, so the leaf supplies both bare-include roots itself:
# inc/djinterp/cpp/meta (for `#include "dtuple_wrap_partition.hpp"`) and
# tests/djinterp/cpp/meta (for `#include "dtuple_wrap_partition_tests.hpp"`).
#
#   NOTE — shared directory: core/meta hosts several suites and a directory
# gets exactly ONE CMakeLists.txt. If a sibling suite (e.g. bsearch, dtuple,
# type_traits) is already wired here, do NOT add a second file — merge the
# djinterp_add_test_executable(dtuple_wrap_partition_tests ...) call below into
# the existing one (the R5 "single-folder cluster" form; sketch at the bottom).
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/meta")

# Section TUs (they DEFINE the tests_pwp_* functions). The runner lives in this
# config dir and is named explicitly below, so it is never globbed; a stray
# *_main.cpp under tests/ would collide on main(), so exclude it.
file(GLOB _wrap_partition_sources CONFIGURE_DEPENDS
    "${_tests_dir}/dtuple_wrap_partition_tests_*.cpp")
list(FILTER _wrap_partition_sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(dtuple_wrap_partition_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/dtuple_wrap_partition_tests_runner.cpp"
        ${_wrap_partition_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/meta"
        "${_tests_dir}")

# ----------------------------------------------------------------------------
# Optional group target over every suite in this directory. Safe for a lone
# leaf; drop it if a parent already provides the meta-level aggregate.
# Aggregate names are GLOBALLY UNIQUE.
# ----------------------------------------------------------------------------
# djinterp_add_test_subtree(all_djinterp_core_meta)

# ----------------------------------------------------------------------------
# R5 sketch — if several meta suites share this ONE directory, collapse them
# into a cluster instead of separate files:
#
#   set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/meta")
#   function(meta_suite _name)
#       file(GLOB _s CONFIGURE_DEPENDS "${_tests_dir}/${_name}_tests_*.cpp")
#       list(FILTER _s EXCLUDE REGEX "_main\\.cpp$")
#       djinterp_add_test_executable(${_name}_tests
#           SOURCES  "${CMAKE_CURRENT_SOURCE_DIR}/${_name}_tests_runner.cpp" ${_s}
#           INCLUDES "${DJINTERP_INC_ROOT}/djinterp/cpp/meta" "${_tests_dir}")
#   endfunction()
#   meta_suite(dtuple_wrap_partition)
#   meta_suite(bsearch)
#   # meta_suite(dtuple) ...
#   djinterp_add_test_subtree(all_djinterp_core_meta)   # optional group target
# ----------------------------------------------------------------------------
