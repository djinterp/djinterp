# ============================================================================
# djinterp [tests]  -  test leaf for test_report_runner_table.hpp
#
# Header-only component, so this is the R1 leaf recipe: glob the section TUs out
# of the tests dir, add the runner from THIS config dir, and register the
# executable.  djinterp_add_test_executable already puts inc/ on the target; the
# two extra INCLUDES below cover the suite header's two faces - inc/djinterp/
# test/output resolves the bare "test_report_runner_table.hpp" the section TUs include
# (normal face), and the tests dir itself resolves the bare
# "test_report_runner_table_tests.hpp" both faces include.
#
# NOTE (merge, do not clobber):  the config directory
#   build/cmake/config/testing/djinterp/test/output/
# is shared by every suite whose header lives under inc/djinterp/test/output/
# (test_render_pdf, test_render_pdf_table, this one, ...).  CMake allows only ONE
# CMakeLists.txt per directory, so if one already exists here, MERGE the marked
# block below into it (add this one djinterp_add_test_executable call and its two
# locals); do NOT overwrite the file.  Each suite's glob is scoped to its own
# "<component>_tests_*.cpp" prefix, so the calls coexist without overlap.
# ============================================================================

# ---- test_report_runner_table suite : BEGIN (merge this block) --------------------
set(_rrt_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/test/output")

file(GLOB _rrt_sources CONFIGURE_DEPENDS
     "${_rrt_tests_dir}/test_report_runner_table_tests_*.cpp")
list(FILTER _rrt_sources EXCLUDE REGEX "_runner\\.cpp$")

djinterp_add_test_executable(test_report_runner_table_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_report_runner_table_tests_runner.cpp"
        ${_rrt_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/test/output"
        "${_rrt_tests_dir}")
# ---- test_report_runner_table suite : END -----------------------------------------
