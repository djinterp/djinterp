# =============================================================================
# djinterp/core/db :: MariaDB and MySQL leaf (decision D4)
#
#   core/db's harness over the MariaDB layer, a CTest test. The C layer is C99,
# core/db C++17. The client library is looked for as pkg-config's libmariadb,
# then mysqlclient; without it nothing is made here. The harness reads its
# server from the environment (DJINTERP_TEST_MARIADB_HOST, _PORT, _USER,
# _PASSWORD). The layer's own suites, and its stub build's, are
# c/db/mariadb's leaf, in DTest's C face (2026.11.10).
# =============================================================================

set(_c_dir     "${DJINTERP_SRC_ROOT}/djinterp/c/db")
set(_core_dir  "${DJINTERP_SRC_ROOT}/djinterp/cpp/db")
set(_core_test "${DJINTERP_TESTS_ROOT}/unintegrated/cpp/db/mariadb")

file(GLOB _layer CONFIGURE_DEPENDS "${_c_dir}/mariadb/mariadb_*.c")
set(_common "${_c_dir}/db_common.c" "${_c_dir}/db_internal.c")

find_package(PkgConfig QUIET)

if (PkgConfig_FOUND)
    pkg_check_modules(DJINTERP_MARIADB QUIET IMPORTED_TARGET libmariadb)

    if (NOT DJINTERP_MARIADB_FOUND)
        pkg_check_modules(DJINTERP_MARIADB QUIET IMPORTED_TARGET mysqlclient)
    endif()
endif()

if (NOT DJINTERP_MARIADB_FOUND)
    message(STATUS "djinterp core/db: no MariaDB or MySQL client; no "
                   "core/db harness")
    return()
endif()

set(_client PkgConfig::DJINTERP_MARIADB)

# core/db's harness over the layer and its client
djinterp_add_test_executable(mariadb_core_tests
    SOURCES   "${_core_test}/t_mariadb_core.cpp"
        "${_core_dir}/database.cpp"
        "${_core_dir}/database_statement.cpp"
        "${_core_dir}/mysql/mysql_common.cpp"
        "${_core_dir}/mysql/mysql.cpp"
        "${_core_dir}/mariadb/mariadb.cpp"
        ${_layer} ${_common}
    LIBRARIES ${_client})
