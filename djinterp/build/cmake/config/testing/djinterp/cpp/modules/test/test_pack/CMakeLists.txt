# djinterp/test/test_pack — test leaf (recipe R1). Builds test_pack_tests and registers CTest.
#   runner (this dir) + section TUs (tests/djinterp/test).  Output dir is computed:
#   bin/<arch>-<Config>/testing/djinterp/test/test_pack/.
set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/test")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/test_pack_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_tests_(runner|main)\\.cpp$")

djinterp_add_test_executable(test_pack_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_pack_tests_runner.cpp"
        ${_sources}
    DEFINES
        # the recording doubles in place of the compress and archive facades
        # (pack_facade/): the suite steers their outcomes through their hooks
        DTEST_PACK_USE_FACADE_DOUBLE=1
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/modules/test"
        "${DJINTERP_INC_ROOT}/djinterp/cpp/modules/test/output"
        "${_tests_dir}")
