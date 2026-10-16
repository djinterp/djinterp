# ============================================================================
# djinterp [tests]                           test_report_runner - leaf recipe
#
#   Builds the unit-test executable for test_report_runner.hpp, the runner
# front end for the report subsystem.  The runner lives here in the config
# tree; the section translation units are globbed from the tests tree.
#
# path:    /build/cmake/config/testing/djinterp/test/output/
#              test_report_runner/CMakeLists.txt
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/test/output")

# the glob keys on the component stem, so a sibling suite's translation units
# in the same flat tests directory are never pulled into this executable
file(GLOB _sources CONFIGURE_DEPENDS
     "${_tests_dir}/test_report_runner_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(test_report_runner_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_report_runner_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/test/output"  # bare header-under-test
        "${_tests_dir}")                             # bare suite-header

# D_TEST_REPORT_ENABLE_ARCHIVE is REQUIRED, not optional - see the BUILD
# PREREQUISITE note in test_report_runner_tests.hpp.  The module under test
# spells pending_doc::bytes and write_bytes_to_file's parameter `byte_buffer`,
# a typedef reached only through the archive include this macro guards, so the
# header does not compile at C++17 or higher without it.  Defining it for the
# whole target also compiles in the packaging arm the archive section covers
# and keeps report_builder's member set identical across every translation
# unit that links together.  Remove only once the module spells those two uses
# std::string (or includes compress.hpp unconditionally), and drop the archive
# section with it.
target_compile_definitions(test_report_runner_tests
    PRIVATE
        D_TEST_REPORT_ENABLE_ARCHIVE=1)
