# =============================================================================
# djinterp/core/db :: database.hpp unit-test leaf
#
#   Header-only module (recipe R1): no production `src/` is compiled in, since
# database.hpp is header-only. The executable links the runner (which owns
# main()) against every `database_tests_*.cpp` section translation unit.
#
#   Naming (per the test-authoring guide):
#     - exe / CTest target : database_tests   (ctest -R database_tests)
#     - runner (main)       : database_tests_runner.cpp  (this config dir)
#     - section TUs         : tests/djinterp/core/db/database_tests_*.cpp
#
#   Include dirs for a test leaf = inc/djinterp/core/db (so the bare
# `#include "database.hpp"` resolves) + tests/djinterp/core/db (so the bare
# `#include "database_tests.hpp"` resolves). Only inc/ is added automatically;
# the rest is declared here.
#
#   This directory is a pure leaf. It automatically becomes a member of every
# parent aggregate (e.g. all_djinterp_core, all_djinterp, all_tests). To expose
# a group target scoped to just this directory, a sibling/parent dir may add a
# one-line `djinterp_add_test_subtree(all_djinterp_core_db)`.
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/core/db")

# all section TUs for this component; the runner is added explicitly below.
# EXCLUDE any legacy `_main.cpp` so it can never collide with the runner's
# main() at link time.
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/database_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

djinterp_add_test_executable(database_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/database_tests_runner.cpp"
        ${_sources}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/core/db"
        "${_tests_dir}")
