#*****************************************************************************
# djinterp [test]                                                CMakeLists.txt
#
#   Wires the sort subsystem's test executables.  This directory is a MULTI-EXE
# CLUSTER (section 6.4): twelve suites share the one slug
# djinterp/core/util/sort, so they share this single CMakeLists rather than
# each carrying its own.
#
#   Every suite follows the leaf recipe of section 6.1 -- runner listed
# explicitly from THIS directory (it lives in the config tree), section TUs
# globbed from the tests tree -- and takes the same two INCLUDES:
#
#     inc/djinterp/core/util/sort   so the section TUs' bare
#                                   #include "bubble_sort.hpp" resolves
#     tests/djinterp/core/util/sort so the bare
#                                   #include "bubble_sort_tests.hpp" resolves
#                                   in both faces
#
# The helper already adds the inc/ root, which is what the runners' spec face
# needs for <djinterp/core/djinterp.hpp> and "djinterp/test/test_defaults.hpp".
#
#
# path:      /build/cmake/config/testing/djinterp/core/util/sort/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                        created: 2026.07.22
#*****************************************************************************

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/util/sort")
set(_inc_dir   "${DJINTERP_INC_ROOT}/djinterp/core/util/sort")

# djinterp_add_sort_suite
#   macro: one leaf recipe, twelve times.  <_name> is the suite stem, e.g.
# bubble_sort_tests.  Each suite's runner lives in its OWN leaf folder named
# after the stem without the _tests suffix -- bubble_sort_tests -> bubble_sort/
# bubble_sort_tests_runner.cpp -- matching the per-component layout the rest of
# the tree uses; the section TUs are still <_name>_*.cpp under the (flat) tests
# tree.  The runners include their suite header bare (INCLUDES carries the tests
# dir), so sitting a directory deeper does not affect include resolution.
macro(djinterp_add_sort_suite _name)
    string(REGEX REPLACE "_tests$" "" _dir "${_name}")   # bubble_sort_tests -> bubble_sort

    file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/${_name}_*.cpp")
    list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

    djinterp_add_test_executable(${_name}
        SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}/${_name}_runner.cpp"
            ${_sources}
        INCLUDES
            "${_inc_dir}"
            "${_tests_dir}")

    unset(_sources)
    unset(_dir)
endmacro()

#   the shared vocabulary and the dispatch facility
djinterp_add_sort_suite(sort_common_tests)
djinterp_add_sort_suite(sort_dispatch_tests)

#   the six algorithms
djinterp_add_sort_suite(bubble_sort_tests)
djinterp_add_sort_suite(selection_sort_tests)
djinterp_add_sort_suite(insertion_sort_tests)
djinterp_add_sort_suite(merge_sort_tests)
djinterp_add_sort_suite(quick_sort_tests)
djinterp_add_sort_suite(heap_sort_tests)

#   the two integrations built on top of them
djinterp_add_sort_suite(sort_monoid_tests)
djinterp_add_sort_suite(sort_view_tests)

#   the facade, and the cross-cutting conformance suite
djinterp_add_sort_suite(sort_tests)
djinterp_add_sort_suite(sort_conformance_tests)

#   group target: builds every suite at or below this directory, so
# `cmake --build . --target sort_subsystem_tests` covers the whole subsystem
# and `ctest -R _tests` runs them.
djinterp_add_test_subtree(sort_subsystem_tests)