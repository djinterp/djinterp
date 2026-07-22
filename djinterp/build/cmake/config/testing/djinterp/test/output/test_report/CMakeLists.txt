# djinterp/test/output/test_report — test leaf (recipe R1). Builds test_report_tests and registers CTest.
#   runner (this dir) + section TUs (tests/djinterp/test/output).  Output dir is computed:
#   bin/<arch>-<Config>/testing/djinterp/test/output/test_report/.
set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/test/output")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/test_report_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_tests_(runner|main)\.cpp$")

djinterp_add_test_executable(test_report_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_report_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/test"
        "${DJINTERP_INC_ROOT}/djinterp/test/output"
        "${_tests_dir}")
