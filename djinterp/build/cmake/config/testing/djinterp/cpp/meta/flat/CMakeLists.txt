# flat.hpp unit test suite -- leaf recipe (header-only tag module, no src/).
#
#   Delivery form (djinterp_testing.md S7.2): this suite ships in its OWN leaf
# directory .../container/structure/flat/ so the archive owns every path it
# writes.  Fold into a shared cluster by merging the call below into a parent
# CMakeLists and deleting this leaf.
#
#   NOTE (parent aggregate): recursion only descends into directories carrying a
# CMakeLists.txt; if .../container/structure/ has none, add a one-line
# djinterp_add_test_subtree(...) there.
#
#   NOTE (shared tests dir): flat and hierarchical share the tests directory
# .../container/structure/.  Every test in each suite carries a module-unique
# name (tests_tag_flat_* here, tests_tag_hierarchical_* there) and each suite's
# fixtures are module-namespaced (flat_depth_probe / hierarchical_depth_probe),
# so the two suites may be built separately (as here, via the prefix-scoped glob)
# OR aggregated into one executable -- see the sibling ../structure_tags leaf for
# a combined build.  The globs below are prefix-scoped so this target pulls only
# its own section TUs.

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/container/structure")

file(GLOB _sources CONFIGURE_DEPENDS
    "${_tests_dir}/flat_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(flat_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/flat_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/container/structure"  # header under test
        "${_tests_dir}")                                          # suite header
