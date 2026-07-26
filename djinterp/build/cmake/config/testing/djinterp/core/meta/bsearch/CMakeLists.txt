# bsearch.hpp unit test suite -- leaf recipe (header-only module, no src/).
#
#   Delivery form (djinterp_testing.md S7.2): this suite ships in its OWN leaf
# directory .../meta/bsearch/ so the archive owns every path it writes and
# cannot overwrite a sibling's wiring.  To fold it into a shared cluster later,
# merge this djinterp_add_test_executable(...) call into the CMakeLists that
# already sits in .../meta/ (S6.4) and delete this leaf.
#
#   NOTE (parent aggregate): recursion only descends into directories that
# carry a CMakeLists.txt.  If .../meta/ (or a level above it) has no aggregate,
# add a one-line djinterp_add_test_subtree(<unique_name>) there so the build
# reaches this leaf.
#
#   NOTE (sibling headers): bsearch.hpp includes pack_element.hpp and
# lookup_sentinels.hpp from the same inc/djinterp/core/meta directory; both are
# reached through the inc/ root the helper already adds and the meta include
# below.  They are part of the meta core, not of this suite.

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/meta")

file(GLOB _sources CONFIGURE_DEPENDS
    "${_tests_dir}/bsearch_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(bsearch_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/bsearch_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/meta"   # bare header-under-test include
        "${_tests_dir}")                            # bare suite-header include
