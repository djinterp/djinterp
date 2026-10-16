#*****************************************************************************
# djinterp [test]                                                CMakeLists.txt
#
#   Test wiring for the `pool_allocator` component (pool_allocator.hpp).
#
#   This is a delivery leaf: the runner and this file live together in
# .../pool/pool_allocator/, one level below the slug, so the pair is
# self-contained and owns every path it writes.  A sibling suite (pool)
# ships the same way beside it.  Collapsing the two into one multi-exe
# cluster at the slug level is a local decision - copy both
# djinterp_add_test_executable() calls into a shared CMakeLists and delete
# the leaves.
#
#   NOTE: no aggregate is shipped at .../core/memory/pool/.  CMake recursion
# only enters directories that carry a CMakeLists.txt, so a parent aggregate
# (`djinterp_add_test_subtree(<unique_name>)`) is required for this leaf to
# be reached at all.
#
#
# path:      /build/cmake/config/testing/djinterp/core/memory/pool/
#            pool_allocator/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
#*****************************************************************************

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/memory/pool")

#   The stem is the component name and nothing else, so this glob cannot
# match the sibling suite: "pool_tests_*.cpp" does not begin with
# "pool_allocator_tests_".
file(GLOB _sources CONFIGURE_DEPENDS
     "${_tests_dir}/pool_allocator_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(pool_allocator_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/pool_allocator_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/memory/pool"  # header under test
        "${_tests_dir}")                                  # suite header
