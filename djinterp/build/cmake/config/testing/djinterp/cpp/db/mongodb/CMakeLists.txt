# =============================================================================
# djinterp/core/db :: MongoDB leaf (B17)
#
#   core/db's harness over the MongoDB layer, a CTest test. The C layer is
# C99, core/db C++17. The driver is looked for as its own CMake package: the
# 2.x line's (mongoc, from 2.0) first, then the 1.x line's (mongoc-1.0, from
# 1.21); without either nothing is made here. Over the driver the layer
# guards its log handler with dmutex.c, over the platform's threads, which
# the build links. The harness reads its deployment from the environment
# (DJINTERP_TEST_MONGODB_URI, which carries the password, and
# DJINTERP_TEST_MONGODB_STANDALONE_URI). The layer's own suites, and its stub
# build's, are c/db/mongodb's leaf, in DTest's C face (2026.11.10).
# =============================================================================

set(_c_dir     "${DJINTERP_SRC_ROOT}/djinterp/c/db")
set(_core_dir  "${DJINTERP_SRC_ROOT}/djinterp/cpp/db")
set(_core_test "${DJINTERP_TESTS_ROOT}/unintegrated/cpp/db/mongodb")

file(GLOB _layer CONFIGURE_DEPENDS "${_c_dir}/mongodb/mongodb_*.c")
set(_common "${_c_dir}/db_common.c" "${_c_dir}/db_internal.c")

# the driver: 2.x's package, or 1.x's
find_package(mongoc 2.0 CONFIG QUIET)

if (TARGET mongoc::mongoc)
    set(_client mongoc::mongoc)
else()
    find_package(mongoc-1.0 1.21 CONFIG QUIET)

    if (TARGET mongo::mongoc_shared)
        set(_client mongo::mongoc_shared)
    elseif (TARGET mongo::mongoc_static)
        set(_client mongo::mongoc_static)
    endif()
endif()

if (NOT _client)
    message(STATUS "djinterp core/db: the MongoDB driver not found; "
                   "no core/db harness")
    return()
endif()

find_package(Threads REQUIRED)

set(_lock "${DJINTERP_SRC_ROOT}/djinterp/c/sync/dmutex.c")

# core/db's harness over the layer and its client
djinterp_add_test_executable(mongodb_core_tests
    SOURCES   "${_core_test}/t_mongodb_core.cpp"
        "${_core_dir}/database.cpp"
        "${_core_dir}/database_statement.cpp"
        "${_core_dir}/mongodb/mongodb.cpp"
        ${_layer} ${_common} ${_lock}
    LIBRARIES ${_client} Threads::Threads)
