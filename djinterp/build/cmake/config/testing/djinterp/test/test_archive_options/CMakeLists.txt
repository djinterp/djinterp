# ============================================================================
# djinterp [tests]                        test_archive_options - leaf recipe
#
#   Builds the unit-test executable for test_archive_options.hpp,
# comparison, pristine and diff for the archive
# option vocabulary.
# The runner lives here in the config tree; the section translation units are
# globbed from the tests tree.
#
# path:    /build/cmake/config/testing/djinterp/test/
#              test_archive_options/CMakeLists.txt
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/test")

# the glob keys on the component stem, so a sibling suite's translation units
# in the same flat tests directory are never pulled into this executable
file(GLOB _sources CONFIGURE_DEPENDS
     "${_tests_dir}/test_archive_options_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(test_archive_options_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_archive_options_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/test"   # bare header-under-test
        "${_tests_dir}")                       # bare suite-header

# D_TEST_REPORT_ENABLE_ARCHIVE is REQUIRED for the RUNNER, not for the section
# translation units.  The runner includes test_defaults.hpp to reach
# run_module, and test_defaults.hpp includes output/test_report_runner.hpp,
# which spells `byte_buffer` in pending_doc::bytes and write_bytes_to_file's
# parameter - a typedef reachable only through the archive include this macro
# guards.  Without it the runner does not compile at C++17 or higher.  This
# affects every DTest runner; see the BUILD PREREQUISITE note in the suite
# header.  Remove only once test_report_runner.hpp spells those two uses
# std::string, or includes compress.hpp unconditionally.
target_compile_definitions(test_archive_options_tests
    PRIVATE
        D_TEST_REPORT_ENABLE_ARCHIVE=1)
