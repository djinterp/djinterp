# =============================================================================
# testing/djinterp/core/meta/concepts/
#   LEAF (AGENTS-cmake R1) — GENERATED to match the established option/event
# leaf pattern.  Builds concepts_tests from the runner in this directory plus the
# section TUs globbed from the shared tests dir below.  If this suite's section
# TUs actually live in a per-suite subdirectory (not the shared djinterp/core/meta
# dir), adjust _tests_dir to match; the target/aggregation wiring is unaffected.
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/meta")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/concepts_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(concepts_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/concepts_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/meta"
        "${_tests_dir}")
