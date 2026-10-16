# structure_tags -- AGGREGATE test executable for the two structural-kind tag
# suites (flat + hierarchical) in a single binary.
#
#   This leaf demonstrates that the two tag suites can now be aggregated.  It was
# once impossible because both suites defined identically-named tests_tag_* bodies
# (and a shared depth_probe fixture); those names are now module-unique
# (tests_tag_flat_* / tests_tag_hierarchical_*, flat_depth_probe /
# hierarchical_depth_probe), so linking both suites' section TUs into one
# executable produces no duplicate symbols.
#
#   It globs BOTH prefixes from the shared tests directory and links them with the
# combined runner, which runs both module specs in one process.  The per-suite
# leaves (../flat, ../hierarchical) remain valid for building each suite on its
# own; this aggregate is an additional target, not a replacement.
#
#   NOTE (parent aggregate): recursion only descends into directories carrying a
# CMakeLists.txt; if .../container/structure/ has none, add a one-line
# djinterp_add_test_subtree(...) there so the build reaches this leaf.

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/container/structure")

file(GLOB _sources CONFIGURE_DEPENDS
    "${_tests_dir}/flat_tests_*.cpp"
    "${_tests_dir}/hierarchical_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(structure_tags_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/structure_tags_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/container/structure"  # headers under test
        "${_tests_dir}")                                          # suite headers
