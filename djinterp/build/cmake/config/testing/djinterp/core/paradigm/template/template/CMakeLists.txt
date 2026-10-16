# ============================================================================
# djinterp [test] : template test module wiring
#
#   Header-only leaf recipe for the template-source-sink algebra suite.  The
# runner (template_tests_runner.cpp) lives beside this file; the per-section
# translation units live under the tests tree.  Globbing the sections keeps the
# list maintenance-free as sections are added or removed.
#
#   Slug: djinterp/core/paradigm/template   (from the module's own declared
#   path, /inc/djinterp/core/paradigm/template/template.hpp)
#     header under test : ${DJINTERP_INC_ROOT}/djinterp/core/paradigm/template/template.hpp
#     suite header      : ${DJINTERP_TESTS_ROOT}/djinterp/core/paradigm/template/template_tests.hpp
#     sections          : ${DJINTERP_TESTS_ROOT}/djinterp/core/paradigm/template/template_tests_*.cpp
#     runner            : ${CMAKE_CURRENT_SOURCE_DIR}/template_tests_runner.cpp
#
#   The module requires C++14 (higher-order return-type deduction) and
# self-suppresses on C++11, so no dialect floor is asserted here beyond whatever
# the toolchain configuration already sets.  The concept section stands down on
# a dialect without concepts, so the suite needs no conditional wiring.
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/paradigm/template")

# every per-section translation unit for this component
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/template_tests_*.cpp")

# never sweep in a hand-rolled entry point
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

# The suite header includes the header under test as a bare "template.hpp" and
# the sections/runner include a bare "template_tests.hpp", so both the inc
# component directory and the tests directory must be on the include path.
djinterp_add_test_executable(template_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/template_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/paradigm/template"
        "${_tests_dir}")
