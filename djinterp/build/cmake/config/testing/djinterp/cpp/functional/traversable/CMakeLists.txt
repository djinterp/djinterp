# djinterp [test]  traversable.hpp suite -- build wiring
#
#   Leaf recipe: the executable is the runner in THIS config directory linked
# with the suite's section translation units.
#
#   PATH NOTE (read this if the build cannot find the sources / the header).
# This suite does NOT use the usual tests/djinterp/<slug> layout.  Its header,
# traversable_tests.hpp, reaches the code under test with a RELATIVE include
#     #include "../../core/functional/traversable.hpp"
# and its banner records its path as
#     inc/djinterp/test/functional/traversable_tests.hpp
# Both pin the suite header (and, we assume, the co-located section TUs) under
#     inc/djinterp/test/functional
# so _tests_dir points there rather than under tests/.  If in your tree the
# traversable_tests_*.cpp section files actually live somewhere else (e.g.
# tests/djinterp/test/functional or tests/djinterp/cpp/functional), change the
# single set(_tests_dir ...) line below to match; nothing else needs to move.
#
#   INCLUDES adds inc/djinterp/test/functional so the bare
# #include "traversable_tests.hpp" resolves from the runner (which sits in this
# config dir, not beside the header).  The suite header reaches
# traversable.hpp relatively (no -I needed) and the runner reaches
# test_defaults.hpp full-from-inc (via the inc/ root the helper already adds),
# so no other include roots are required.
#
#   Output path, D_TESTING=1, the C++20/C17 flags, djinterp_c_runtime, and the
# CTest registration (ctest -R traversable_tests) are all supplied by the
# helper; never set the output directory by hand.

set(_tests_dir "${DJINTERP_INC_ROOT}/djinterp/test/functional")

# the section TUs (one per block); exclude any stale ..._main.cpp so two
# main()s never collide at link.
file(GLOB _traversable_sources CONFIGURE_DEPENDS
    "${_tests_dir}/traversable_tests_*.cpp")
list(FILTER _traversable_sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(traversable_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/traversable_tests_runner.cpp"
        ${_traversable_sources}
    INCLUDES
        "${_tests_dir}")
