# ============================================================================
# djinterp [test] : memento test module wiring
#
#   Header-only leaf recipe for the Memento pattern suite. The runner
# (memento_tests_runner.cpp) lives beside this file; the per-section
# translation units live under the tests tree. Globbing the sections keeps the
# list maintenance-free as sections are added or removed.
#
#   Slug: djinterp/paradigm/momento   (the module's own path spells the
#   directory "momento" while the header is memento.hpp; mirrored here)
#     header under test : ${DJINTERP_INC_ROOT}/djinterp/paradigm/momento/memento.hpp
#     suite header      : ${DJINTERP_TESTS_ROOT}/djinterp/paradigm/momento/memento_tests.hpp
#     sections          : ${DJINTERP_TESTS_ROOT}/djinterp/paradigm/momento/memento_tests_*.cpp
#     runner            : ${CMAKE_CURRENT_SOURCE_DIR}/memento_tests_runner.cpp
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/paradigm/momento")

# every per-section translation unit for this component
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/memento_tests_*.cpp")

# never sweep in a hand-rolled entry point
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

# The suite header includes the header under test as a bare "memento.hpp" and
# the sections/runner include a bare "memento_tests.hpp", so both the inc
# component directory and the tests directory must be on the include path.
djinterp_add_test_executable(memento_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/memento_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/paradigm/momento"
        "${_tests_dir}")
