# djinterp/test/test_event — test leaf (recipe R1). Builds test_event_tests and registers CTest.
#   runner (this dir) + section TUs (tests/djinterp/test).  Output dir is computed:
#   bin/<arch>-<Config>/testing/djinterp/test/test_event/.
#   It links DTest's C library, djinterp_c_test, for C's alphabet
#   (test_event.c), which the parity section (test_event_tests_parity.cpp)
#   holds to C++'s.
set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/test")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/test_event_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_tests_(runner|main)\.cpp$")

djinterp_add_test_executable(test_event_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_event_tests_runner.cpp"
        ${_sources}
    LIBRARIES
        djinterp_c_test
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/modules/test"
        "${DJINTERP_INC_ROOT}/djinterp/cpp/modules/test/output"
        "${_tests_dir}")
