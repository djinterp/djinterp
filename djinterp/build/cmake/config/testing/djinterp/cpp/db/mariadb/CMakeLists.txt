# =============================================================================
# djinterp/core/db :: MariaDB and MySQL leaf (decision D4)
#
#   The MariaDB layer's C harness, its stub build's, and core/db's harness over
# it, each a CTest test. The C layer is C99, core/db C++17. The client library
# is looked for as pkg-config's libmariadb, then mysqlclient; without it only
# the stub build's harness is made, which needs none. The harnesses that talk
# to a server read it from the environment (DJINTERP_TEST_MARIADB_HOST, _PORT,
# _USER, _PASSWORD) and report a skip without one, unless
# DJINTERP_TEST_MARIADB_REQUIRED is set.
# =============================================================================

set(_c_dir     "${DJINTERP_SRC_ROOT}/djinterp/c/db")
set(_core_dir  "${DJINTERP_SRC_ROOT}/djinterp/cpp/db")
set(_c_tests   "${DJINTERP_TESTS_ROOT}/unintegrated/c/db/mariadb")
set(_core_test "${DJINTERP_TESTS_ROOT}/unintegrated/cpp/db/mariadb")

file(GLOB _layer CONFIGURE_DEPENDS "${_c_dir}/mariadb/mariadb_*.c")
set(_common "${_c_dir}/db_common.c" "${_c_dir}/db_internal.c")

# the stub build: the layer without its client, every call D_DB_UNSUPPORTED
djinterp_add_test_executable(mariadb_stubs_tests
    SOURCES "${_c_tests}/t_mariadb_stubs.c" ${_layer} ${_common}
    DEFINES D_CFG_MARIADB=0)

find_package(PkgConfig QUIET)

if (PkgConfig_FOUND)
    pkg_check_modules(DJINTERP_MARIADB QUIET IMPORTED_TARGET libmariadb)

    if (NOT DJINTERP_MARIADB_FOUND)
        pkg_check_modules(DJINTERP_MARIADB QUIET IMPORTED_TARGET mysqlclient)
    endif()
endif()

if (NOT DJINTERP_MARIADB_FOUND)
    message(STATUS "djinterp core/db: no MariaDB or MySQL client; the stub "
                   "build only")
    return()
endif()

set(_client PkgConfig::DJINTERP_MARIADB)

# the layer over its client: its C harness, and core/db's over it
djinterp_add_test_executable(mariadb_c_tests
    SOURCES   "${_c_tests}/t_mariadb.c" ${_layer} ${_common}
    LIBRARIES ${_client})
djinterp_add_test_executable(mariadb_core_tests
    SOURCES   "${_core_test}/t_mariadb_core.cpp"
        "${_core_dir}/database.cpp"
        "${_core_dir}/database_statement.cpp"
        "${_core_dir}/mysql/mysql_common.cpp"
        "${_core_dir}/mysql/mysql.cpp"
        "${_core_dir}/mariadb/mariadb.cpp"
        ${_layer} ${_common}
    LIBRARIES ${_client})
