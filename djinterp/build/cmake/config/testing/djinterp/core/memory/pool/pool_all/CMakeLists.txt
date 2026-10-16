#*****************************************************************************
# djinterp [test]                                                CMakeLists.txt
#
#   pool_all_tests: both pool-subsystem suites in one exe, one combined report.
#
#   Delivery leaf, sibling to pool/ and pool_allocator/ under the
# .../core/memory/pool/ aggregate.  Deliberate overlap: the same section TUs
# are compiled here and into the per-component exes, so either can be run
# alone.  The runner includes both suite headers - legal because each suite's
# check macro carries a distinct suffix.
#
#   The tests dir is tests/djinterp/core/memory/pool, NOT the flat
# tests/djinterp/core/memory this recipe used to name.  pool is the one
# component whose suite headers sit a level down, matching its headers in
# inc/djinterp/core/memory/pool.  Each glob keys on its component stem alone,
# and neither can match the other.
#
#
# path:      /build/cmake/config/testing/djinterp/core/memory/pool/pool_all/
#            CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
#*****************************************************************************

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/memory/pool")

file(GLOB _src0 CONFIGURE_DEPENDS "${_tests_dir}/pool_tests_*.cpp")
file(GLOB _src1 CONFIGURE_DEPENDS "${_tests_dir}/pool_allocator_tests_*.cpp")
list(FILTER _src0 EXCLUDE REGEX "_main\\.cpp$")
list(FILTER _src1 EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(pool_all_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/pool_all_tests_runner.cpp"
        ${_src0}
        ${_src1}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/memory/pool"  # headers under test
        "${_tests_dir}")                                  # suite headers
