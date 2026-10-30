# ============================================================================
# djinterp [tests]                        test_render_pdf_table - leaf recipe
#
#   Builds the unit-test executable for test_render_pdf_table.hpp, the default
# DTest PDF report renderer.  The runner lives here in the config tree; the
# section translation units are globbed from the tests tree.
#
# path:    /build/cmake/config/testing/djinterp/test/output/
#              test_render_pdf_table/CMakeLists.txt
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/test/output")

# the glob keys on the component stem, so a sibling suite's translation units
# in the same flat tests directory are never pulled into this executable
file(GLOB _sources CONFIGURE_DEPENDS
     "${_tests_dir}/test_render_pdf_table_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(test_render_pdf_table_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_render_pdf_table_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/modules/test/output"  # bare header-under-test
        "${_tests_dir}")                             # bare suite-header

# D_TEST_REPORT_ENABLE_ARCHIVE is REQUIRED for the RUNNER, not for the section
# translation units.  The runner includes test_defaults.hpp to reach
# run_module, and test_defaults.hpp includes output/test_report_runner.hpp,
# which spells `byte_buffer` in pending_doc::bytes and write_bytes_to_file's
# parameter - a typedef reachable only through the archive include this macro
# guards.  Without it the runner does not compile at C++17 or higher.  This
# affects every DTest runner, not just this suite; see the BUILD PREREQUISITE
# note in the suite header.
target_compile_definitions(test_render_pdf_table_tests
    PRIVATE
        D_TEST_REPORT_ENABLE_ARCHIVE=1)
