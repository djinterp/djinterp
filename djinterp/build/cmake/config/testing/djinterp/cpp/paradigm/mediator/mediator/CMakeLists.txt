# ============================================================================
# djinterp [test] : mediator test module wiring
#
#   Header-only leaf recipe for the mediator pattern suite.  The runner
# (mediator_tests_runner.cpp) lives beside this file; the per-section
# translation units live under the tests tree.  Globbing the sections keeps
# the list maintenance-free as sections are added or removed.
#
#   Slug: djinterp/patterns/mediator
#     header under test : ${DJINTERP_INC_ROOT}/djinterp/patterns/mediator/mediator.hpp
#     suite header      : ${DJINTERP_TESTS_ROOT}/djinterp/patterns/mediator/mediator_tests.hpp
#     sections          : ${DJINTERP_TESTS_ROOT}/djinterp/patterns/mediator/mediator_tests_*.cpp
#     runner            : ${CMAKE_CURRENT_SOURCE_DIR}/mediator_tests_runner.cpp
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/patterns/mediator")

# every per-section translation unit for this component
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/mediator_tests_*.cpp")

# never sweep in a hand-rolled entry point
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

# The suite header includes the header under test as a bare "mediator.hpp" and
# the sections/runner include a bare "mediator_tests.hpp", so both the inc
# component directory and the tests directory must be on the include path.
djinterp_add_test_executable(mediator_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/mediator_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/patterns/mediator"
        "${_tests_dir}")
