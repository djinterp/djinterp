# =============================================================================
# djinterp/core/db :: SQLite leaf (decision D4)
#
#   The SQLite layer's C harness, its stub build's, and core/db's harness over
# it, each a CTest test. The C layer is C99, core/db C++17. The client library
# is looked for as CMake's SQLite3 package; without it only the stub build's
# harness is made, which needs none. The harnesses that talk to a server read
# it from the environment (a file of its own) and report a skip without one,
# unless DJINTERP_TEST_SQLITE_REQUIRED is set.
# =============================================================================

set(_c_dir     "${DJINTERP_SRC_ROOT}/djinterp/c/db")
set(_core_dir  "${DJINTERP_SRC_ROOT}/djinterp/cpp/db")
set(_c_tests   "${DJINTERP_TESTS_ROOT}/unintegrated/c/db/sqlite")
set(_core_test "${DJINTERP_TESTS_ROOT}/unintegrated/cpp/db/sqlite")

file(GLOB _layer CONFIGURE_DEPENDS "${_c_dir}/sqlite/sqlite_*.c")
set(_common "${_c_dir}/db_common.c" "${_c_dir}/db_internal.c")

# the stub build: the layer without its client, every call D_DB_UNSUPPORTED
djinterp_add_test_executable(sqlite_stubs_tests
    SOURCES "${_c_tests}/t_sqlite_stubs.c" ${_layer} ${_common}
    DEFINES D_CFG_SQLITE=0)

find_package(SQLite3 QUIET)

if (NOT SQLite3_FOUND)
    message(STATUS "djinterp core/db: SQLite3 not found; the stub build only")
    return()
endif()

set(_client SQLite::SQLite3)

# the layer over its client: its C harness, and core/db's over it
djinterp_add_test_executable(sqlite_c_tests
    SOURCES   "${_c_tests}/t_sqlite.c" ${_layer} ${_common}
    LIBRARIES ${_client})
djinterp_add_test_executable(sqlite_core_tests
    SOURCES   "${_core_test}/t_sqlite_core.cpp"
        "${_core_dir}/database.cpp"
        "${_core_dir}/database_statement.cpp"
        "${_core_dir}/sqlite/sqlite.cpp"
        ${_layer} ${_common}
    LIBRARIES ${_client})
