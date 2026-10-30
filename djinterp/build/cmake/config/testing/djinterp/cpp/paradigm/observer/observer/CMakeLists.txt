# ============================================================================
# djinterp [test] : observer test module wiring
#
#   Header-only leaf recipe for the observer pattern suite. The runner
# (observer_tests_runner.cpp) lives beside this file; the per-section
# translation units live under the tests tree. Globbing the sections keeps the
# list maintenance-free as sections are added or removed.
#
#   Slug: djinterp/paradigm/pattern   (the module header sits directly in
#   paradigm/pattern/ as observer.hpp)
#     header under test : ${DJINTERP_INC_ROOT}/djinterp/paradigm/pattern/observer.hpp
#     suite header      : ${DJINTERP_TESTS_ROOT}/djinterp/paradigm/pattern/observer_tests.hpp
#     sections          : ${DJINTERP_TESTS_ROOT}/djinterp/paradigm/pattern/observer_tests_*.cpp
#     runner            : ${CMAKE_CURRENT_SOURCE_DIR}/observer_tests_runner.cpp
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/paradigm/pattern")

# every per-section translation unit for this component
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/observer_tests_*.cpp")

# never sweep in a hand-rolled entry point
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

# The suite header includes the header under test as a bare "observer.hpp" and
# the sections/runner include a bare "observer_tests.hpp", so both the inc
# component directory and the tests directory must be on the include path.
djinterp_add_test_executable(observer_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/observer_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/paradigm/pattern"
        "${_tests_dir}")
