# djinterp/test/output/test_output — test leaf (recipe R1). Builds test_output_tests and registers CTest.
#   runner (this dir) + section TUs (tests/djinterp/cpp/modules/test/output).  Output dir is computed:
#   bin/<arch>-<Config>/testing/djinterp/test/output/test_output/.
set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/test/output")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/test_output_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_tests_(runner|main)\.cpp$")

djinterp_add_test_executable(test_output_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_output_tests_runner.cpp"
        ${_sources}
        "${DJINTERP_SRC_ROOT}/djinterp/cpp/util/compress.cpp"   # internal::compress_buffer
        "${DJINTERP_SRC_ROOT}/djinterp/cpp/util/archive.cpp"    # internal::archive_create
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/modules/test"
        "${DJINTERP_INC_ROOT}/djinterp/cpp/modules/test/output"
        "${_tests_dir}")
