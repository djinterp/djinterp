# container/table/ unit test suite -- leaf recipe (header-only module, no
# src/).
#
#   One executable for container/table/ (the floor suite): the fifteen table
# headers.  The section TUs follow the "table_tests_<section>.cpp"
# convention, so a prefix-scoped glob picks them up and never the retired
# test_table_*.cpp programs; the runner beside this file owns main().

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container/table")

file(GLOB _sources CONFIGURE_DEPENDS
    "${_tests_dir}/table_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(table_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/table_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${_tests_dir}")            # suite header
