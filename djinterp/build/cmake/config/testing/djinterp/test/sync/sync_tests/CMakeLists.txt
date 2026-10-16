#*****************************************************************************
# djinterp [tests]                                              CMakeLists.txt
#
#   Leaf recipe for the AGGREGATE sync_tests executable, which drives
# run_suite over all eight module specs in a single process.
#
#   DELIBERATE DEVIATION FROM THE PER-COMPONENT GLOB RULE:
#   Every other leaf in this tree globs only its own component's stem.  This
# one globs EVERY *_tests_*.cpp under the slug on purpose - the aggregate
# links the same section TUs as the per-module executables and adds only its
# own main().  Any future suite added under tests/djinterp/test/sync is
# therefore picked up automatically, which is the intent: the aggregate is
# meant to be the whole subframework.  Its spec list is NOT automatic, though
# - a new module must also be added to sync_tests.hpp's sync_all_specs().
#
#   The glob cannot collide with a competing main(): the per-module runners
# live in sibling directories of THIS config tree, not in the globbed tests
# directory.
#
# path:  /build/cmake/config/testing/djinterp/test/sync/sync_tests/CMakeLists.txt
#*****************************************************************************

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/test/sync")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/*_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(sync_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/sync_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/test/sync"   # bare header-under-test include
        "${_tests_dir}")                            # bare suite-header include
