# =============================================================================
# testing/djinterp/cpp/meta/value_list/
#   LEAF (AGENTS-cmake R1) for value_list.hpp, a meta header: the runner in
# this directory plus the section TUs globbed from the shared meta tests dir.
# The glob is prefix-scoped (value_list_tests_*), so the other meta suites in
# that dir stay out. (Under functional/ until 2026.11.11, where the header
# never was -- decision L5.)
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/meta")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/value_list_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(value_list_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/value_list_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/meta"   # header under test
        "${_tests_dir}")
