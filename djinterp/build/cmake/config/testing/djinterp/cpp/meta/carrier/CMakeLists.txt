# =============================================================================
# testing/djinterp/core/meta/carrier/
#   LEAF (AGENTS-cmake R1) — GENERATED to match the established option/event
# leaf pattern.  Builds carrier_tests from the runner in this directory plus the
# section TUs globbed from the shared tests dir below.  If this suite's section
# TUs actually live in a per-suite subdirectory (not the shared djinterp/core/meta
# dir), adjust _tests_dir to match; the target/aggregation wiring is unaffected.
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/meta")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/carrier_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(carrier_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/carrier_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/meta"
        "${_tests_dir}")
