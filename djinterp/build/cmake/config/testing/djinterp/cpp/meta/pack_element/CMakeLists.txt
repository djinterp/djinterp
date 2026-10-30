# ============================================================================
# djinterp [testing]                        djinterp/core/meta - CMakeLists.txt
#
#   Test executables for the meta slug.  pack_element.hpp is a header-only
# leaf with no src/ counterpart, so this follows the header-only recipe:
# glob the section TUs out of the tests tree, prepend the runner that lives
# beside this file in the config tree, and let djinterp_add_test_executable
# handle the include roots, the C++ standard, D_TESTING and CTest
# registration.
#
# path:      /build/cmake/config/testing/djinterp/core/meta/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.20
# ============================================================================

# ----------------------------------------------------------------------------
#  pack_element_tests
# ----------------------------------------------------------------------------
#   Sections live in the mirrored tests tree; the runner carrying main() lives
# here.  The filter is defensive - the runner is not in the globbed directory,
# but the glob must never pick up a second main().

set(_pack_element_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/meta")

file(GLOB _pack_element_sources CONFIGURE_DEPENDS
    "${_pack_element_tests_dir}/pack_element_tests_*.cpp")

list(FILTER _pack_element_sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(pack_element_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/pack_element_tests_runner.cpp"
        ${_pack_element_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/meta"
        "${_pack_element_tests_dir}")

unset(_pack_element_sources)
unset(_pack_element_tests_dir)
