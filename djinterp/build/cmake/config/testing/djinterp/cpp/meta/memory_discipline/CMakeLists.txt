# memory_discipline.hpp unit test suite -- leaf recipe (header-only module, no
# src/).
#
#   The tests directory .../meta/ holds other suites too; the glob is
# prefix-scoped (memory_discipline_tests_*) so this executable pulls only its
# own section TUs.

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/meta")

file(GLOB _sources CONFIGURE_DEPENDS
    "${_tests_dir}/memory_discipline_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(memory_discipline_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/memory_discipline_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${_tests_dir}")            # suite header
