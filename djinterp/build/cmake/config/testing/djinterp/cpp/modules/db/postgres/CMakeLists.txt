# =============================================================================
# djinterp/core/db :: PostgreSQL leaf (decision D4)
#
#   core/db's harness over the PostgreSQL layer, a CTest test. The C layer is
# C99, core/db C++17. The client library is looked for as CMake's PostgreSQL
# package (libpq); without it nothing is made here. The harness reads its
# server from the environment (DJINTERP_TEST_POSTGRES_HOST, _PORT, _USER,
# _PASSWORD). The layer's own suites, and its stub build's, are
# c/modules/db/postgres's leaf, in DTest's C face (2026.11.10).
# =============================================================================

set(_c_dir     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/db")
set(_core_dir  "${DJINTERP_SRC_ROOT}/djinterp/cpp/modules/db")
set(_core_test "${DJINTERP_TESTS_ROOT}/unintegrated/cpp/db/postgres")

file(GLOB _layer CONFIGURE_DEPENDS "${_c_dir}/postgres/postgres_*.c")
set(_common "${_c_dir}/db_common.c" "${_c_dir}/db_internal.c")

find_package(PostgreSQL QUIET)

if (NOT PostgreSQL_FOUND)
    message(STATUS "djinterp core/db: libpq not found; no core/db harness")
    return()
endif()

set(_client PostgreSQL::PostgreSQL)

# core/db's harness over the layer and its client
djinterp_add_test_executable(postgres_core_tests
    SOURCES   "${_core_test}/t_postgres_core.cpp"
        "${_core_dir}/database.cpp"
        "${_core_dir}/database_statement.cpp"
        "${_core_dir}/postgres/postgres.cpp"
        ${_layer} ${_common}
    LIBRARIES ${_client})

# it needs a server: labelled needs-server, which a run without one leaves
# out (ctest -LE needs-server, as CI's build job does); CI's servers job
# runs it against one
if (BUILD_TESTING)
    set_property(TEST postgres_core_tests APPEND PROPERTY LABELS needs-server)
endif()
