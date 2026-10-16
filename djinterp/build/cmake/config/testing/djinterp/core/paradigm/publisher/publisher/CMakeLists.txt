# ============================================================================
# djinterp [test] : publish-subscribe test module wiring
#
#   Header-only leaf recipe for the publish-subscribe paradigm suite. The runner
# (publisher_tests_runner.cpp) lives beside this file; the per-section
# translation units live under the tests tree. Globbing the sections keeps the
# list maintenance-free as sections are added or removed.
#
#   Slug: djinterp/core/paradigm/pubsub   (from the module's own declared path,
#   /inc/djinterp/core/paradigm/pubsub/; the component file is carried through
#   as publisher.hpp to match the header as supplied)
#     header under test : ${DJINTERP_INC_ROOT}/djinterp/core/paradigm/pubsub/publisher.hpp
#     suite header      : ${DJINTERP_TESTS_ROOT}/djinterp/core/paradigm/pubsub/publisher_tests.hpp
#     sections          : ${DJINTERP_TESTS_ROOT}/djinterp/core/paradigm/pubsub/publisher_tests_*.cpp
#     runner            : ${CMAKE_CURRENT_SOURCE_DIR}/publisher_tests_runner.cpp
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/paradigm/pubsub")

# every per-section translation unit for this component
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/publisher_tests_*.cpp")

# never sweep in a hand-rolled entry point
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

# The suite header includes the header under test as a bare "publisher.hpp" and
# the sections/runner include a bare "publisher_tests.hpp", so both the inc
# component directory and the tests directory must be on the include path.
djinterp_add_test_executable(publisher_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/publisher_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/paradigm/pubsub"
        "${_tests_dir}")
