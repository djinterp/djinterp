#*****************************************************************************
# djinterp [test]                                                CMakeLists.txt
#
#   Aggregate directory for the option layer (decomposed layout: one leaf
# subdirectory per suite).  This directory no longer defines the per-suite exes
# - each lives in its own child directory (option/, option_set/, ... optionator/)
# under AGENTS-cmake R1.  What remains here:
#
#     - option_tests_all : the combined-report aggregate exe, which links the
#       aggregate runner + EVERY suite's section TU (blanket glob of the shared
#       tests directory); and
#     - all_djinterp_core_option : the subtree group target, which recurses into
#       the per-suite child directories (building each <suite>_tests) and then
#       depends on every option exe at or below here - the ten per-suite exes
#       plus option_tests_all.
#
#   The section TUs stay in the shared tests/djinterp/core/option/ directory;
# each per-suite leaf globs its own sections from there.  Only the runners are
# per-directory: each <suite>_tests_runner.cpp sits in its suite's subdirectory
# beside that subdirectory's CMakeLists.txt, while option_tests_all_runner.cpp
# stays here beside this file.
#
# path:      /build/cmake/config/testing/djinterp/core/option/CMakeLists.txt
# author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.08
#*****************************************************************************

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/option")

# ---------------------------------------------------------------------------
# aggregate exe: one combined report over the whole option layer.  Links the
# aggregate runner + every section TU under the shared tests directory (the
# per-suite runners live in the child directories, so they are not swept in).
# The runner forward-declares the block-providers and defines
# D_TEST_REPORT_ENABLE_ARCHIVE itself, so no extra DEFINES here.
# ---------------------------------------------------------------------------
file(GLOB _all_sections CONFIGURE_DEPENDS "${_tests_dir}/*.cpp")
list(FILTER _all_sections EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(option_tests_all
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/option_tests_all_runner.cpp"
        ${_all_sections}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/option"
        "${_tests_dir}")

# ---------------------------------------------------------------------------
# group target: recurse into the per-suite child directories (each builds its
# <suite>_tests), then depend on every option exe at or below this directory.
# Parent aggregates (all_djinterp_core, ...) pick this up automatically.
# ---------------------------------------------------------------------------
djinterp_add_test_subtree(all_djinterp_core_option)
