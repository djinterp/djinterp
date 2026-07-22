# djinterp/test/test_context — test leaf (recipe R1). Builds test_context_tests and registers CTest.
#   runner (this dir) + section TUs (tests/djinterp/test).  Output dir is computed:
#   bin/<arch>-<Config>/testing/djinterp/test/test_context/.
set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/test")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/test_context_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_tests_(runner|main)\.cpp$")

djinterp_add_test_executable(test_context_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_context_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/test"
        "${DJINTERP_INC_ROOT}/djinterp/test/output"
        "${_tests_dir}")
