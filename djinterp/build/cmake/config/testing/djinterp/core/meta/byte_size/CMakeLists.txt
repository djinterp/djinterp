# ============================================================================
# djinterp — build wiring for the byte_size.hpp test suite
#
#   Component: byte_size  (header-only, no src/)
#   Slug     : djinterp/core/container
#            (byte_size.hpp lives at inc/djinterp/core/container/byte_size.hpp)
#
#   AGENTS-cmake.md R1 recipe (header-only module, single test leaf). Builds
# the `byte_size_tests` executable from the runner in THIS config directory
# plus the section TUs under tests/. The helper adds only inc/ automatically,
# so the leaf supplies both bare-include roots itself: inc/djinterp/core/container
# (for `#include "byte_size.hpp"`) and tests/djinterp/core/container (for
# `#include "byte_size_tests.hpp"`).
#
#   NOTE — shared directory: a directory gets exactly ONE CMakeLists.txt. If a
# sibling suite already lives in core/container, do NOT add a second file —
# merge the djinterp_add_test_executable(byte_size_tests ...) call below into
# the existing one (the R5 "single-folder cluster" form; sketch at the bottom).
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/container")

# Section TUs (they DEFINE the tests_byte_size_* functions). The runner lives
# in this config dir and is named explicitly below, so it is never globbed; a
# stray *_main.cpp under tests/ would collide on main(), so exclude it.
file(GLOB _byte_size_sources CONFIGURE_DEPENDS
    "${_tests_dir}/byte_size_tests_*.cpp")
list(FILTER _byte_size_sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(byte_size_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/byte_size_tests_runner.cpp"
        ${_byte_size_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/container"
        "${_tests_dir}")

# ----------------------------------------------------------------------------
# Optional group target over every suite in this directory. Safe for a lone
# leaf; drop it if a parent already provides the container-level aggregate.
# Aggregate names are GLOBALLY UNIQUE.
# ----------------------------------------------------------------------------
# djinterp_add_test_subtree(all_djinterp_core_container)

# ----------------------------------------------------------------------------
# R5 sketch — if several container suites share this ONE directory, collapse
# them into a cluster instead of separate files:
#
#   set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/container")
#   function(container_suite _name)
#       file(GLOB _s CONFIGURE_DEPENDS "${_tests_dir}/${_name}_tests_*.cpp")
#       list(FILTER _s EXCLUDE REGEX "_main\\.cpp$")
#       djinterp_add_test_executable(${_name}_tests
#           SOURCES  "${CMAKE_CURRENT_SOURCE_DIR}/${_name}_tests_runner.cpp" ${_s}
#           INCLUDES "${DJINTERP_INC_ROOT}/djinterp/core/container" "${_tests_dir}")
#   endfunction()
#   container_suite(byte_size)
#   # container_suite(<other>) ...
#   djinterp_add_test_subtree(all_djinterp_core_container)   # optional group target
# ----------------------------------------------------------------------------
