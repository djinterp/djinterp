# array family unit test suite -- leaf recipe (header-only module, no src/).
#
#   One executable for container/array/: array<>, its options, traits,
# concepts and iterators, and the four concurrent wrappers.  The section TUs
# follow the "<stem>_tests_<section>.cpp" convention, so a prefix-scoped glob
# picks them up; the runner beside this file owns main().
#
#   THREADS: the atomic, cow, rcu and threadsafe sections start threads, so
# the executable links the platform's thread library.

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/container/array")

file(GLOB _sources CONFIGURE_DEPENDS
    "${_tests_dir}/array_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

find_package(Threads REQUIRED)

djinterp_add_test_executable(array_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/array_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${_tests_dir}"            # suite header
    LIBRARIES
        Threads::Threads)
