# ============================================================================
# djinterp [test] : pattern test module wiring
#
#   Header-only leaf recipe for the pattern primitive suite. The runner
# (pattern_tests_runner.cpp) lives beside this file; the per-section
# translation units live under the tests tree. Globbing the sections keeps the
# list maintenance-free as sections are added or removed.
#
#   Slug: djinterp/core/paradigm/pattern
#     header under test : ${DJINTERP_INC_ROOT}/djinterp/core/paradigm/pattern/pattern.hpp
#     suite header      : ${DJINTERP_TESTS_ROOT}/djinterp/core/paradigm/pattern/pattern_tests.hpp
#     sections          : ${DJINTERP_TESTS_ROOT}/djinterp/core/paradigm/pattern/pattern_tests_*.cpp
#     runner            : ${CMAKE_CURRENT_SOURCE_DIR}/pattern_tests_runner.cpp
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/paradigm/pattern")

# every per-section translation unit for this component
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/pattern_tests_*.cpp")

# never sweep in a hand-rolled entry point
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

# The suite header includes the header under test as a bare "pattern.hpp" and
# the sections/runner include a bare "pattern_tests.hpp", so both the inc
# component directory and the tests directory must be on the include path.
djinterp_add_test_executable(pattern_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/pattern_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/paradigm/pattern"
        "${_tests_dir}")
