# ============================================================================
# djinterp [test] : visitor test module wiring
#
#   Header-only leaf recipe for the visitor pattern suite.  The runner
# (visitor_tests_runner.cpp) lives beside this file; the per-section translation
# units live under the tests tree.  Globbing the sections keeps the list
# maintenance-free as sections are added or removed.
#
#   Slug: djinterp/patterns/visitor
#     header under test : ${DJINTERP_INC_ROOT}/djinterp/patterns/visitor/visitor.hpp
#     suite header      : ${DJINTERP_TESTS_ROOT}/djinterp/patterns/visitor/visitor_tests.hpp
#     sections          : ${DJINTERP_TESTS_ROOT}/djinterp/patterns/visitor/visitor_tests_*.cpp
#     runner            : ${CMAKE_CURRENT_SOURCE_DIR}/visitor_tests_runner.cpp
#
#   NOTE ON THE SLUG.  The header's banner gives its path as /inc/patterns/
# visitor.hpp, but it includes "../../djinterp.hpp" -- two levels up -- which that
# path cannot satisfy.  The slug below is the reading that makes the include
# resolve while keeping the banner's "patterns" grouping.  If the tree says
# otherwise, this is the one line to change.
#
#   The module floor is C++11: the classic visitor's visitor_base and
# visitable_base are C++98-clean, but concrete_visitor, the acyclic form and the
# static form all sit behind D_VISITOR_HAS_VARIADIC_TEMPLATES, and D_VISITABLE
# itself uses decltype.  The variant and concept sections gate themselves at C++17
# and C++20 and stand down below, so the suite needs no conditional wiring and
# reports the same test count at every level.
# ============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/patterns/visitor")

# every per-section translation unit for this component
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/visitor_tests_*.cpp")

# never sweep in a hand-rolled entry point
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

# The suite header includes the header under test as a bare "visitor.hpp" and the
# sections/runner include a bare "visitor_tests.hpp", so both the inc component
# directory and the tests directory must be on the include path.
djinterp_add_test_executable(visitor_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/visitor_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/patterns/visitor"
        "${_tests_dir}")
