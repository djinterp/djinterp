# kv_pair.hpp unit test suite -- leaf recipe (header-only module, no src/).
#
#   Delivery form (djinterp_testing.md S7.2): ships in its OWN leaf directory
# .../meta/kv_pair/.  Fold into a shared cluster by merging the call below into
# a parent CMakeLists and deleting this leaf.
#
#   NOTE (parent aggregate): recursion only descends into directories carrying a
# CMakeLists.txt; if .../meta/ has none, add a one-line
# djinterp_add_test_subtree(...) there.
#
#   NOTE (shared tests dir): the tests directory .../meta/ also holds other
# suites; the glob is prefix-scoped (kv_pair_tests_*) so this executable pulls
# only its own section TUs.

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/meta")

file(GLOB _sources CONFIGURE_DEPENDS
    "${_tests_dir}/kv_pair_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(kv_pair_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/kv_pair_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/meta"   # header under test
        "${_tests_dir}")                            # suite header
