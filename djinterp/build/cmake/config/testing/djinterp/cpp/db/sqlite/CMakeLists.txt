# =============================================================================
# djinterp/core/db :: SQLite leaf (decision D4)
#
#   core/db's harness over the SQLite layer, a CTest test. The C layer is C99,
# core/db C++17. The client library is looked for as CMake's SQLite3 package;
# without it nothing is made here. The layer's own suites, and its stub
# build's, are c/db/sqlite's leaf, in DTest's C face (2026.11.10).
# =============================================================================

set(_c_dir     "${DJINTERP_SRC_ROOT}/djinterp/c/db")
set(_core_dir  "${DJINTERP_SRC_ROOT}/djinterp/cpp/db")
set(_core_test "${DJINTERP_TESTS_ROOT}/unintegrated/cpp/db/sqlite")

file(GLOB _layer CONFIGURE_DEPENDS "${_c_dir}/sqlite/sqlite_*.c")
set(_common "${_c_dir}/db_common.c" "${_c_dir}/db_internal.c")

find_package(SQLite3 QUIET)

if (NOT SQLite3_FOUND)
    message(STATUS "djinterp core/db: SQLite3 not found; no core/db harness")
    return()
endif()

set(_client SQLite::SQLite3)

# core/db's harness over the layer and its client
djinterp_add_test_executable(sqlite_core_tests
    SOURCES   "${_core_test}/t_sqlite_core.cpp"
        "${_core_dir}/database.cpp"
        "${_core_dir}/database_statement.cpp"
        "${_core_dir}/sqlite/sqlite.cpp"
        ${_layer} ${_common}
    LIBRARIES ${_client})
