# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/c/modules/db/postgres/CMakeLists.txt
#
#   Test leaf for c/modules/db/postgres's suites, in DTest's C face
# (c/modules/test/test_suite.h), each a CTest test with its runner in this
# directory:
#     - c_postgres_stubs_tests: the layer built without libpq
#       (D_CFG_POSTGRES=0), every call D_DB_UNSUPPORTED; it links nothing
#       else.
#     - c_postgres_tests: the layer against a PostgreSQL server, made where
#       CMake's PostgreSQL package (libpq) is found. The server is read from
#       the environment (DJINTERP_TEST_POSTGRES_HOST, _PORT, _USER, _PASSWORD,
#       _DATABASE); without one to reach, its first test fails and says how to
#       name one. It carries the label needs-server, which CI's run on a push
#       leaves out (ctest --label-exclude needs-server).
#     - c_postgres_sizes_tests: the sizes postgres_connection.c computes for
#       its allocations, each given a count too large, made beside
#       c_postgres_tests. It includes the unit, which no other source here
#       then compiles, and needs no server.
# The C layer is C99. core/db's harnesses over it stay in
# cpp/modules/db/postgres.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.10
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/db/postgres")
set(_c_dir     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/db")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

file(GLOB _layer CONFIGURE_DEPENDS "${_c_dir}/postgres/postgres_*.c")
set(_common "${_c_dir}/db_common.c" "${_c_dir}/db_internal.c")

# the stub build: the layer without its client
djinterp_add_test_executable(c_postgres_stubs_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/postgres_stubs_tests_runner.c"
        "${_tests_dir}/postgres_stubs_tests.c"
        ${_layer} ${_common} "${_dtest}"
    INCLUDES  "${_tests_dir}"
    DEFINES   D_CFG_POSTGRES=0)

find_package(PostgreSQL QUIET)

if (NOT PostgreSQL_FOUND)
    message(STATUS "djinterp c/modules/db: libpq not found; the stub build only")
    return()
endif()

# the layer over its client
djinterp_add_test_executable(c_postgres_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/postgres_tests_runner.c"
        "${_tests_dir}/postgres_tests.c"
        ${_layer} ${_common} "${_dtest}"
    INCLUDES  "${_tests_dir}"
    LIBRARIES PostgreSQL::PostgreSQL)

# a server to test against: CI's run on a push leaves the suite out
if (TEST c_postgres_tests)
    set_property(TEST c_postgres_tests APPEND PROPERTY LABELS needs-server)
endif()

# the checked sizes, over the unit the suite includes
set(_rest ${_layer})
list(FILTER _rest EXCLUDE REGEX "/postgres_connection\\.c$")

djinterp_add_test_executable(c_postgres_sizes_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/postgres_sizes_tests_runner.c"
        "${_tests_dir}/postgres_sizes_tests.c"
        ${_rest} ${_common} "${_dtest}"
    INCLUDES  "${_tests_dir}"
    LIBRARIES PostgreSQL::PostgreSQL)
