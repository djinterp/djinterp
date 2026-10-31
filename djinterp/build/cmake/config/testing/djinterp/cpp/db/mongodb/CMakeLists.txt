# =============================================================================
# djinterp/core/db :: MongoDB leaf (B17)
#
#   The MongoDB layer's C harness, its stub build's, and core/db's harness
# over it, each a CTest test. The C layer is C99, core/db C++17. The driver
# is looked for as its own CMake package: the 2.x line's (mongoc, from 2.0)
# first, then the 1.x line's (mongoc-1.0, from 1.21); without either only
# the stub build's harness is made, which needs none. Over the driver the
# layer guards its log handler with dmutex.c, over the platform's threads,
# which those builds link. The harnesses that talk to a deployment read it
# from the environment (DJINTERP_TEST_MONGODB_URI, which carries the
# password, and DJINTERP_TEST_MONGODB_STANDALONE_URI) and report a skip
# without one, unless DJINTERP_TEST_MONGODB_REQUIRED is set.
# =============================================================================

set(_c_dir     "${DJINTERP_SRC_ROOT}/djinterp/c/db")
set(_core_dir  "${DJINTERP_SRC_ROOT}/djinterp/cpp/db")
set(_c_tests   "${DJINTERP_TESTS_ROOT}/unintegrated/c/db/mongodb")
set(_core_test "${DJINTERP_TESTS_ROOT}/unintegrated/cpp/db/mongodb")

file(GLOB _layer CONFIGURE_DEPENDS "${_c_dir}/mongodb/mongodb_*.c")
set(_common "${_c_dir}/db_common.c" "${_c_dir}/db_internal.c")

# the stub build: the layer without its client, every call D_DB_UNSUPPORTED
djinterp_add_test_executable(mongodb_stubs_tests
    SOURCES "${_c_tests}/t_mongodb_stubs.c" ${_layer} ${_common}
    DEFINES D_CFG_MONGODB=0)

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
                   "the stub build only")
    return()
endif()

find_package(Threads REQUIRED)

set(_lock "${DJINTERP_SRC_ROOT}/djinterp/c/sync/dmutex.c")

# the layer over its client: its C harness, and core/db's over it
djinterp_add_test_executable(mongodb_c_tests
    SOURCES   "${_c_tests}/t_mongodb.c" ${_layer} ${_common} ${_lock}
    LIBRARIES ${_client} Threads::Threads)
djinterp_add_test_executable(mongodb_core_tests
    SOURCES   "${_core_test}/t_mongodb_core.cpp"
        "${_core_dir}/database.cpp"
        "${_core_dir}/database_statement.cpp"
        "${_core_dir}/mongodb/mongodb.cpp"
        ${_layer} ${_common} ${_lock}
    LIBRARIES ${_client} Threads::Threads)
