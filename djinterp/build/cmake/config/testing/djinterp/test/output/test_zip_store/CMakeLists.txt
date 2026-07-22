# ============================================================================
# djinterp [tests]  -  test leaf for test_zip_store.hpp
#
# Header-only component, so this is the R1 leaf recipe: glob the section TUs out
# of the tests dir, add the runner from THIS config dir, and register the
# executable.  djinterp_add_test_executable already puts inc/ on the target; the
# two extra INCLUDES below cover the suite header's two faces - inc/djinterp/
# test/output resolves the bare "test_zip_store.hpp" the section TUs include
# (normal face), and the tests dir itself resolves the bare
# "test_zip_store_tests.hpp" both faces include.
#
# NOTE (merge, do not clobber):  the config directory
#   build/cmake/config/testing/djinterp/test/output/
# is shared by every suite whose header lives under inc/djinterp/test/output/
# (test_render_pdf, test_render_pdf_table, test_report_runner[_table], this one,
# ...).  CMake allows only ONE CMakeLists.txt per directory, so if one already
# exists here, MERGE the marked block below into it (add this one
# djinterp_add_test_executable call and its two locals); do NOT overwrite the
# file.  Each suite's glob is scoped to its own "<component>_tests_*.cpp" prefix,
# so the calls coexist without overlap.
# ============================================================================

# ---- test_zip_store suite : BEGIN (merge this block) ------------------------
set(_zs_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/test/output")

file(GLOB _zs_sources CONFIGURE_DEPENDS
     "${_zs_tests_dir}/test_zip_store_tests_*.cpp")
list(FILTER _zs_sources EXCLUDE REGEX "_runner\\.cpp$")

djinterp_add_test_executable(test_zip_store_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_zip_store_tests_runner.cpp"
        ${_zs_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/test/output"
        "${_zs_tests_dir}")
# ---- test_zip_store suite : END ---------------------------------------------
