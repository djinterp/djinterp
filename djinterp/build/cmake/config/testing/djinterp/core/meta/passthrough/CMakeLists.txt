# passthrough.hpp unit test suite -- leaf recipe (header-only module, no src/).
#
#   Delivery form (djinterp_testing.md S7.2): this suite ships in its OWN leaf
# directory .../meta/passthrough/ so the archive owns every path it writes and
# cannot overwrite a sibling's wiring.  To fold it into a shared cluster later,
# merge this djinterp_add_test_executable(...) call into the CMakeLists that
# already sits in .../meta/ (S6.4) and delete this leaf.
#
#   NOTE (parent aggregate): recursion only descends into directories that
# carry a CMakeLists.txt.  If .../meta/ (or a level above it) has no aggregate,
# add a one-line djinterp_add_test_subtree(<unique_name>) there so the build
# reaches this leaf.

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/meta")

file(GLOB _sources CONFIGURE_DEPENDS
    "${_tests_dir}/passthrough_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(passthrough_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/passthrough_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/meta"   # bare header-under-test include
        "${_tests_dir}")                            # bare suite-header include
