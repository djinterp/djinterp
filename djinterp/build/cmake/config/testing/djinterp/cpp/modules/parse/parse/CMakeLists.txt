# djinterp/parse - parse.hpp test leaf (recipe R1, delivery form)
#
#   A single header-only suite.  The runner (main(), here in the config tree)
# links with the section translation units matching parse_tests_*.cpp (in
# tests/, where the flat djinterp::testing predicates are defined).  Linking
# the runner with those TUs produces the test executable; the suite header is
# included by both, in its two DTEST_SPEC_MODE faces.
#
#   INCLUDES adds exactly the two roots guide Section 6.2 calls for on top of
# the inc/ root the helper supplies on its own:
#     - inc/djinterp/parse : the bare "parse.hpp" include in the section TUs
#                            (normal face), plus the bare
#                            "../core/functional/result.hpp" parse.hpp itself
#                            resolves relative to that directory.
#     - tests/djinterp/parse : the bare "parse_tests.hpp" include in both the
#                              runner (spec face) and the section TUs.
#   The runner's own full-from-inc includes ("djinterp/core/djinterp.hpp",
# "djinterp/test/test_defaults.hpp") resolve through the inc/ root the helper
# already adds, so they are not listed here.
#
#   This directory is the component's OWN leaf (guide Section 7.2): a delivery
# ships the runner and this CMakeLists together one level below the slug so
# the archive overwrites nothing.  There is deliberately no parent aggregate
# in the zip -- see the BUILD NOTE at the foot of this file.
#
#   djinterp_add_test_executable links djinterp_c_runtime, defines D_TESTING=1,
# builds C++20/C17 (/permissive- /Zc:preprocessor under MSVC), sets the
# per-config output directory, and registers the exe with CTest.  Run it with
#     ctest -R parse_tests
# and it lands at bin/<arch>-<Config>/testing/djinterp/parse/parse_tests.
#
# path: /build/cmake/config/testing/djinterp/cpp/modules/parse/parse/CMakeLists.txt

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/parse")

# The section TUs, globbed from tests/.  The _main.cpp filter drops any stale
# tests/...\_main.cpp so a scratch main() can never collide with the runner's
# at link.  The glob keys on the parse_tests_ stem, which is disjoint from any
# sibling suite's stem in the same directory.
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/parse_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(parse_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/parse_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/modules/parse"      # bare header-under-test include
        "${_tests_dir}")                           # bare suite-header include

# BUILD NOTE (aggregate): this delivery ships no parent CMakeLists, because a
# zip overwrites and must not clobber existing wiring (guide Section 7.3).
# CMake recursion only descends into directories that already have a
# CMakeLists, so for this leaf to be reached, the parent
# config/testing/djinterp/parse/ needs a one-line aggregate added by hand:
#
#     djinterp_add_test_subtree(all_djinterp_parse)
#
# Add that (or merge parse_tests into an existing sibling cluster there per
# guide Section 6.4) so the suite is picked up by the build.
