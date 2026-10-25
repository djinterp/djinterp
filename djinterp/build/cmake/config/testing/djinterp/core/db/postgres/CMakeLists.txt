# =============================================================================
# djinterp/core/db :: PostgreSQL leaf (decision D4)
#
#   The PostgreSQL layer's C harness, its stub build's, and core/db's harness
# over it, each a CTest test. The C layer is C99, core/db C++17. The client
# library is looked for as CMake's PostgreSQL package (libpq); without it only
# the stub build's harness is made, which needs none. The harnesses that talk
# to a server read it from the environment (DJINTERP_TEST_POSTGRES_HOST, _PORT,
# _USER, _PASSWORD) and report a skip without one, unless
# DJINTERP_TEST_POSTGRES_REQUIRED is set.
# =============================================================================

set(_c_dir     "${DJINTERP_SRC_ROOT}/djinterp/c/db")
set(_core_dir  "${DJINTERP_SRC_ROOT}/djinterp/core/db")
set(_c_tests   "${DJINTERP_TESTS_ROOT}/djinterp/c/db/postgres")
set(_core_test "${DJINTERP_TESTS_ROOT}/djinterp/core/db/postgres")

file(GLOB _layer CONFIGURE_DEPENDS "${_c_dir}/postgres/postgres_*.c")
set(_common "${_c_dir}/db_common.c" "${_c_dir}/db_internal.c")

# the stub build: the layer without its client, every call D_DB_UNSUPPORTED
djinterp_add_test_executable(postgres_stubs_tests
    SOURCES "${_c_tests}/t_postgres_stubs.c" ${_layer} ${_common}
    DEFINES D_CFG_POSTGRES=0)

find_package(PostgreSQL QUIET)

if (NOT PostgreSQL_FOUND)
    message(STATUS "djinterp core/db: libpq not found; the stub build only")
    return()
endif()

set(_client PostgreSQL::PostgreSQL)

# the layer over its client: its C harness, and core/db's over it
djinterp_add_test_executable(postgres_c_tests
    SOURCES   "${_c_tests}/t_postgres.c" ${_layer} ${_common}
    LIBRARIES ${_client})
djinterp_add_test_executable(postgres_core_tests
    SOURCES   "${_core_test}/t_postgres_core.cpp"
        "${_core_dir}/database.cpp"
        "${_core_dir}/database_statement.cpp"
        "${_core_dir}/postgres/postgres.cpp"
        ${_layer} ${_common}
    LIBRARIES ${_client})
