# djinterp -- test_traits test leaf
#
#   Recipe R1: a header-only module with no production sources.  The header under
# test is inc/djinterp/test/test_traits.hpp, so the slug is `djinterp/test` and
# this file lives at
#
#     build/cmake/config/testing/djinterp/test/CMakeLists.txt
#
# with the runner (main) beside it.  The section TUs live under tests/ and are
# globbed; the exe lands in
#
#     bin/<arch>-<Config>/testing/djinterp/test/
#
# because that is where THIS CMakeLists sits relative to the config root -- the
# output directory is computed, never written by hand.
#
#   NOTE ON MERGING.  `djinterp/test` is a populated slug: test_kind, test_pack
# and the rest of the test framework live in the same inc/ directory, so a
# CMakeLists probably already exists here with leaves of its own.  If so, do NOT
# replace it -- lift the `test_suite()` helper below into it (or reuse the one it
# already has) and add the single call.  That is recipe R5, the single-folder
# cluster, and the helper is written to make the merge a one-liner.

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/test")
set(_inc_dir   "${DJINTERP_INC_ROOT}/djinterp/test")

# test_suite
#   function: defines one header-only test leaf named <component>_tests.  Globs
# tests/<component>_tests_*.cpp for the sections, pairs them with the runner of
# the same stem from this directory, and adds the two include roots a test leaf
# always needs -- inc/<slug>, so the bare `#include "<component>.hpp"` the
# framework's headers use resolves, and tests/<slug> for the test header.
function(test_suite _component)
    file(GLOB _sections CONFIGURE_DEPENDS
        "${_tests_dir}/${_component}_tests_*.cpp")
    list(FILTER _sections EXCLUDE REGEX "_main\\.cpp$")

    djinterp_add_test_executable(${_component}_tests
        SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/${_component}_tests_runner.cpp"
            ${_sections}
        INCLUDES
            "${_inc_dir}"
            "${_tests_dir}")
endfunction()

test_suite(test_traits)

#   test_traits_tests quarantines four demonstrations whose failure mode is the
# BUILD rather than the report: count_holds refusing to short-circuit,
# is_nothrow_probe handed a probe that is not one, a D_TEST_STATIC that fires,
# and a completeness-requiring trait run over the list that contains the
# incomplete fixture.  They are off by default, so the suite always builds.  To
# reproduce all four -- the build failing IS the finding -- configure with:
#
#     -DDJINTERP_TT_HAZARD_TESTS=ON
#
if(DJINTERP_TT_HAZARD_TESTS)
    target_compile_definitions(test_traits_tests
        PRIVATE D_TT_HAZARD_TESTS=1)
endif()

#   RUNNING IT ON ITS OWN.  djinterp_add_test_executable registers the exe with
# CTest under the target's own name, so from the build directory:
#
#     cmake --build . --target test_traits_tests
#     ctest -R test_traits_tests --output-on-failure
#
# or run the binary directly, which is what writes the PDF:
#
#     ./bin/<arch>-<Config>/testing/djinterp/test/test_traits_tests
