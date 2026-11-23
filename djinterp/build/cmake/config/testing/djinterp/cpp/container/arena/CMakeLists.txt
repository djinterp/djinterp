# container/arena/ unit test suite -- leaf recipe (header-only module, no
# src/).
#
#   One executable for container/arena/ (the floor suite): arena.hpp, its traits
# and concepts, and tree/'s arena tree, traits and concepts.  The section TUs
# follow the "arena_tests_<section>.cpp" convention, so a prefix-scoped glob
# picks them up; the runner beside this file owns main().

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container/arena")

file(GLOB _sources CONFIGURE_DEPENDS
    "${_tests_dir}/arena_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(arena_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/arena_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${_tests_dir}")            # suite header
