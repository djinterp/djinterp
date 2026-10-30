# ============================================================================
# djinterp [testing]                    djinterp/core/paradigm - CMakeLists.txt
#
#   Test executables for the paradigm slug.  override.hpp is a header-only
# leaf with no src/ counterpart, so this follows the header-only recipe:
# glob the section TUs out of the tests tree, prepend the runner that lives
# beside this file in the config tree, and let djinterp_add_test_executable
# handle the include roots, the C++ standard, D_TESTING and CTest
# registration.
#
# path:      /build/cmake/config/testing/djinterp/core/paradigm/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.20
# ============================================================================

# ----------------------------------------------------------------------------
#  override_tests
# ----------------------------------------------------------------------------
#   Sections live in the mirrored tests tree; the runner carrying main() lives
# here.  The filter is defensive - the runner is not in the globbed directory,
# but the glob must never pick up a second main().

set(_override_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/paradigm")

file(GLOB _override_sources CONFIGURE_DEPENDS
    "${_override_tests_dir}/override_tests_*.cpp")

list(FILTER _override_sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(override_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/override_tests_runner.cpp"
        ${_override_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/paradigm"
        "${_override_tests_dir}")

unset(_override_sources)
unset(_override_tests_dir)
