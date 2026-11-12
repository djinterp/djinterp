# =============================================================================
# testing/djinterp/cpp/
#   The C++ layer's aggregate, and djinterp.hpp's suite.
#
#   Two things happen here:
#     - djinterp_header_tests : djinterp.hpp's suite (tests/djinterp/cpp/
#       djinterp_tests*), in DTest's C face: the runner in THIS directory, the
#       section translation units globbed from the tests tree, and DTest's C
#       face, which djinterp_c_runtime does not yet carry.
#     - all_djinterp_core : the subtree group target, which recurses into every
#       child directory and then depends on every test exe at or below here,
#       djinterp_header_tests included.
#
#   Composes upward: testing/djinterp/CMakeLists.txt (all_djinterp) recurses
# straight through this file, so the whole core layer is reachable from the
# top-level aggregate with no edit there.
#
#   NOTE — this file previously held the *option* aggregate (option_tests_all +
# all_djinterp_core_option); that content belongs one level down in
# core/option/CMakeLists.txt and has been moved there.  Defining option_tests_all
# here is what caused the duplicate-target configure error.
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp")

# the section TUs (the runner is named explicitly, never globbed; a stray
# entry point under tests/ would collide on main(), so exclude)
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/djinterp_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_(main|runner)\\.cpp$")

djinterp_add_test_executable(djinterp_header_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/djinterp_header_tests_runner.cpp"
        ${_sources}
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")

# Recurse into the core subsystems and publish the group target over them all.
djinterp_add_test_subtree(all_djinterp_core)
