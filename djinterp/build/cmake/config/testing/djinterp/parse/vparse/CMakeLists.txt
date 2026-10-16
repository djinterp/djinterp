# djinterp [vparse] — test leaf for djinterp/parse/vparse
#
# Recipe R2: the module has production sources under src/djinterp/parse/vparse
# consumed only by this test exe, so they are compiled straight into it.  The
# helper adds inc/ and defines D_TESTING=1; this leaf adds the module include
# dir (for the bare-form headers) and the tests dir (for the test header).

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/parse/vparse")
set(_src_dir   "${DJINTERP_SRC_ROOT}/djinterp/parse/vparse")

file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/vparse_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

file(GLOB _prod CONFIGURE_DEPENDS "${_src_dir}/*.cpp")

djinterp_add_test_executable(vparse_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/vparse_tests_runner.cpp"
        ${_sources}
        ${_prod}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/parse/vparse"
        "${_tests_dir}")
