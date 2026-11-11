# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/db/mariadb/CMakeLists.txt
#
#   Test leaf for c/db/mariadb's suites, in DTest's C face
# (c/modules/test/test_suite.h), each a CTest test with its runner in this
# directory:
#     - c_mariadb_stubs_tests: the layer built without a client
#       (D_CFG_MARIADB=0), every call D_DB_UNSUPPORTED; it links nothing else.
#     - c_mariadb_tests: the layer against a MariaDB or MySQL server, made
#       where pkg-config finds libmariadb, or else mysqlclient. The server is
#       read from the environment (DJINTERP_TEST_MARIADB_HOST, _PORT, _USER,
#       _PASSWORD, _SOCKET); without one to reach, its first test fails and
#       says how to name one.
#     - c_mariadb_sizes_tests: the size mariadb_query.c computes for a
#       query's attribute names, given a count too large, made beside
#       c_mariadb_tests; skipped over a client without query attributes. It
#       includes the unit, which no other source here then compiles, and
#       needs no server.
# The C layer is C99. core/db's harnesses over it stay in cpp/db/mariadb.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.10
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/db/mariadb")
set(_c_dir     "${DJINTERP_SRC_ROOT}/djinterp/c/db")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

file(GLOB _layer CONFIGURE_DEPENDS "${_c_dir}/mariadb/mariadb_*.c")
set(_common "${_c_dir}/db_common.c" "${_c_dir}/db_internal.c")

# the stub build: the layer without its client
djinterp_add_test_executable(c_mariadb_stubs_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/mariadb_stubs_tests_runner.c"
        "${_tests_dir}/mariadb_stubs_tests.c"
        ${_layer} ${_common} "${_dtest}"
    INCLUDES  "${_tests_dir}"
    DEFINES   D_CFG_MARIADB=0)

find_package(PkgConfig QUIET)

if (PkgConfig_FOUND)
    pkg_check_modules(DJINTERP_MARIADB QUIET IMPORTED_TARGET libmariadb)

    if (NOT DJINTERP_MARIADB_FOUND)
        pkg_check_modules(DJINTERP_MARIADB QUIET IMPORTED_TARGET mysqlclient)
    endif()
endif()

if (NOT DJINTERP_MARIADB_FOUND)
    message(STATUS "djinterp c/db: no MariaDB or MySQL client; the stub "
                   "build only")
    return()
endif()

# the layer over its client
djinterp_add_test_executable(c_mariadb_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/mariadb_tests_runner.c"
        "${_tests_dir}/mariadb_tests.c"
        ${_layer} ${_common} "${_dtest}"
    INCLUDES  "${_tests_dir}"
    LIBRARIES PkgConfig::DJINTERP_MARIADB)

# the checked sizes, over the unit the suite includes
set(_rest ${_layer})
list(FILTER _rest EXCLUDE REGEX "/mariadb_query\\.c$")

djinterp_add_test_executable(c_mariadb_sizes_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/mariadb_sizes_tests_runner.c"
        "${_tests_dir}/mariadb_sizes_tests.c"
        ${_rest} ${_common} "${_dtest}"
    INCLUDES  "${_tests_dir}"
    LIBRARIES PkgConfig::DJINTERP_MARIADB)
