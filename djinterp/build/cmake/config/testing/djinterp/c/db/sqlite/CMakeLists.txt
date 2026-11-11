# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/db/sqlite/CMakeLists.txt
#
#   Test leaf for c/db/sqlite's suites, in DTest's C face
# (c/modules/test/test_suite.h), each a CTest test with its runner in this
# directory:
#     - c_sqlite_stubs_tests: the layer built without SQLite
#       (D_CFG_SQLITE=0), every call D_DB_UNSUPPORTED; it links nothing else.
#     - c_sqlite_tests: the layer against SQLite, made where CMake's SQLite3
#       package is found. Its files go in the test's working directory, or the
#       one DJINTERP_TEST_SQLITE_DIRECTORY names, and are removed after. Its
#       extension checks load the library DJINTERP_TEST_SQLITE_EXTENSION
#       names, which scripts/ci/check_db_sqlite.sh builds; without one they
#       check what fails, and say so.
# The C layer is C99. core/db's harnesses over it stay in cpp/db/sqlite.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.10
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/db/sqlite")
set(_c_dir     "${DJINTERP_SRC_ROOT}/djinterp/c/db")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

file(GLOB _layer CONFIGURE_DEPENDS "${_c_dir}/sqlite/sqlite_*.c")
set(_common "${_c_dir}/db_common.c" "${_c_dir}/db_internal.c")

# the stub build: the layer without its client
djinterp_add_test_executable(c_sqlite_stubs_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/sqlite_stubs_tests_runner.c"
        "${_tests_dir}/sqlite_stubs_tests.c"
        ${_layer} ${_common} "${_dtest}"
    INCLUDES  "${_tests_dir}"
    DEFINES   D_CFG_SQLITE=0)

find_package(SQLite3 QUIET)

if (NOT SQLite3_FOUND)
    message(STATUS "djinterp c/db: SQLite3 not found; the stub build only")
    return()
endif()

# the layer over its client
djinterp_add_test_executable(c_sqlite_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/sqlite_tests_runner.c"
        "${_tests_dir}/sqlite_tests.c"
        ${_layer} ${_common} "${_dtest}"
    INCLUDES  "${_tests_dir}"
    LIBRARIES SQLite::SQLite3)
