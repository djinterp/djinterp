# =============================================================================
# djinterp/core/db :: database.hpp unit-test leaf
#
#   database.hpp's tests, built with the module's production sources (recipe
# R2, approach A). Since chunk 4.2 (db_audit 9.6) database.hpp is no longer
# header-only: its out-of-line definitions -- the exceptions, the
# configurations' defaults, the utilities -- are in
# src/djinterp/cpp/db/database.cpp, which reads values through the C
# layers' common code, c/db/db_common.c, and its private helpers,
# c/db/db_internal.c. The three are named explicitly rather than globbed:
# core/db's other sources are the vendors', and need their client
# libraries, which these tests do not. The executable links the runner
# (which owns main()) against every `database_tests_*.cpp` section
# translation unit and the three sources.
#
#   Naming (per the test-authoring guide):
#     - exe / CTest target : database_tests   (ctest -R database_tests)
#     - runner (main)       : database_tests_runner.cpp  (this config dir)
#     - section TUs         : tests/unintegrated/core/db/database_tests_*.cpp
#
#   Include dirs for a test leaf = inc/djinterp/cpp/db (so the bare
# `#include "database.hpp"` resolves) + tests/unintegrated/cpp/db (so the bare
# `#include "database_tests.hpp"` resolves). Only inc/ is added automatically;
# the rest is declared here.
#
#   This directory is a pure leaf. It automatically becomes a member of every
# parent aggregate (e.g. all_djinterp_core, all_djinterp, all_tests). To expose
# a group target scoped to just this directory, a sibling/parent dir may add a
# one-line `djinterp_add_test_subtree(all_djinterp_core_db)`.
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/unintegrated/cpp/db")
set(_src_dir   "${DJINTERP_SRC_ROOT}/djinterp")

# all section TUs for this component; the runner is added explicitly below.
# EXCLUDE any legacy `_main.cpp` so it can never collide with the runner's
# main() at link time.
file(GLOB _sources CONFIGURE_DEPENDS "${_tests_dir}/database_tests_*.cpp")
list(FILTER _sources EXCLUDE REGEX "_main\\.cpp$")

# production sources compiled into this exe (recipe R2, approach A)
set(_prod
    "${_src_dir}/cpp/db/database.cpp"
    "${_src_dir}/c/db/db_common.c"
    "${_src_dir}/c/db/db_internal.c")

djinterp_add_test_executable(database_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/database_tests_runner.cpp"
        ${_sources}
        ${_prod}
    INCLUDES
        "${DJINTERP_INC_ROOT}/djinterp/cpp/db"
        "${_tests_dir}")
