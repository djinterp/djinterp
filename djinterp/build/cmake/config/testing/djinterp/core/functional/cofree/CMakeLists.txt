# djinterp [test]  cofree.hpp suite -- build wiring
#
#   Leaf recipe (R1): a header-only module under inc/djinterp/core/functional
# with no src/.  The executable is the runner in THIS config directory linked
# with the section translation units under tests/djinterp/core/functional.
#
#   INCLUDES gives the target the two roots the bare includes need on top of the
# inc/ root djinterp_add_test_executable already adds:
#     * inc/djinterp/core/functional -- resolves any bare header-under-test
#       include reached from the suite (the suite header itself uses the
#       full-from-inc form, so this is belt-and-braces / future-proofing);
#     * tests/djinterp/core/functional -- resolves the bare
#       #include "cofree_tests.hpp" from the runner and the section TUs.
#
#   Output path, D_TESTING=1, the C++20/C17 flags, djinterp_c_runtime, and the
# CTest registration (ctest -R cofree_tests) are all supplied by the helper;
# never set the output directory by hand.

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/functional")

# the section TUs (one per block); exclude any stale tests/..._main.cpp so two
# main()s never collide at link.
file(GLOB _cofree_sources CONFIGURE_DEPENDS
    "${_tests_dir}/cofree_tests_*.cpp")
list(FILTER _cofree_sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(cofree_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/cofree_tests_runner.cpp"
        ${_cofree_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/functional"
        "${_tests_dir}")
