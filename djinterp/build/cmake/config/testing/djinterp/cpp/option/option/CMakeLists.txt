#*****************************************************************************
# djinterp [test]                                                CMakeLists.txt
#
#   Leaf build for the option.hpp unit suite (AGENTS-cmake R1).  'option' is a
# string prefix of every sibling suite and its sections mix the _tests-suffix
# and _tests-infix spellings, so its section TUs are named explicitly rather
# than globbed (a glob wide enough to catch option's suffix-spelled sections
# would also pull in option_override's, compiling them into two targets).
# Sections live in the shared tests/djinterp/cpp/option/.
#
#   Each named section is guarded by EXISTS so a not-yet-present TU does not
# hard-fail configuration of the whole tree the way a bare missing source in
# add_executable() does; option_tests still builds from the runner plus
# whatever sections are on disk.  When the missing section TUs are added under
# tests/djinterp/cpp/option/, re-run CMake and they are linked in.  (NOTE:
# until every section this suite needs is present, option_tests will fail at
# LINK - the runner calls into the section entry points - so this guard unblocks
# the rest of the build, it does not make option_tests complete on its own.)
#
# path: /build/cmake/config/testing/djinterp/cpp/option/option/CMakeLists.txt
# author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.08
#*****************************************************************************

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/option")

set(_option_sections
    "${_tests_dir}/option_sentinels_tests.cpp"
    "${_tests_dir}/option_core_tests.cpp"
    "${_tests_dir}/option_is_option_tests.cpp"
    "${_tests_dir}/option_tests_concepts.cpp")

set(_sources "")
foreach(_s IN LISTS _option_sections)
    if(EXISTS "${_s}")
        list(APPEND _sources "${_s}")
    else()
        message(STATUS "option_tests: section not present yet, skipping ${_s}")
    endif()
endforeach()

djinterp_add_test_executable(option_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/option_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/option"
        "${_tests_dir}")
