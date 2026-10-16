# =============================================================================
# testing/djinterp/core/
#   Core-layer aggregate + the top-level djinterp header-smoke suite.
#
#   Two things happen here:
#     - djinterp_header_tests : a leaf exe built from djinterp_header_tests_runner.cpp
#       in THIS directory (a compile/link smoke test over the umbrella
#       <djinterp/core/djinterp.hpp> and the test-framework headers).  Its own
#       test header is pulled in by a repo-relative #include in the runner, so
#       only inc/ (added by the helper) and the shared core tests dir are needed.
#     - all_djinterp_core : the subtree group target, which recurses into every
#       core subsystem child directory (db, event, functional, meta, option,
#       paradigm, sync) and then depends on every test exe at or below here -
#       including djinterp_header_tests.
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

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core")

# Header-smoke section TUs, if any (the runner is named explicitly, never
# globbed; a stray *_main.cpp under tests/ would collide on main(), so exclude).
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/djinterp_header_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(djinterp_header_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/djinterp_header_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core"
        "${_tests_dir}")

# Recurse into the core subsystems and publish the group target over them all.
djinterp_add_test_subtree(all_djinterp_core)
