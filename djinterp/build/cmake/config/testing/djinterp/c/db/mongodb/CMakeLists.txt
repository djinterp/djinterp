# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/db/mongodb/CMakeLists.txt
#
#   Test leaf for c/db/mongodb's suites, in DTest's C face
# (c/modules/test/test_suite.h), each a CTest test with its runner in this
# directory:
#     - c_mongodb_stubs_tests: the layer built without the driver
#       (D_CFG_MONGODB=0), every call D_DB_UNSUPPORTED; it links nothing
#       else.
#     - c_mongodb_tests: the layer against a MongoDB deployment, made where
#       the driver's own CMake package is found: the 2.x line's (mongoc, from
#       2.0) first, then the 1.x line's (mongoc-1.0, from 1.21). Over the
#       driver the layer guards its log handler with dmutex.c, over the
#       platform's threads, which the build links. The deployment is read from
#       the environment (DJINTERP_TEST_MONGODB_URI, which carries the
#       password, and DJINTERP_TEST_MONGODB_STANDALONE_URI); without one to
#       reach, its first test fails and says how to name one.
# The C layer is C99. core/db's harnesses over it stay in cpp/db/mongodb.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.10
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/db/mongodb")
set(_c_dir     "${DJINTERP_SRC_ROOT}/djinterp/c/db")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

file(GLOB _layer CONFIGURE_DEPENDS "${_c_dir}/mongodb/mongodb_*.c")
set(_common "${_c_dir}/db_common.c" "${_c_dir}/db_internal.c")

# the stub build: the layer without its client
djinterp_add_test_executable(c_mongodb_stubs_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/mongodb_stubs_tests_runner.c"
        "${_tests_dir}/mongodb_stubs_tests.c"
        ${_layer} ${_common} "${_dtest}"
    INCLUDES  "${_tests_dir}"
    DEFINES   D_CFG_MONGODB=0)

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
    message(STATUS "djinterp c/db: the MongoDB driver not found; the stub "
                   "build only")
    return()
endif()

find_package(Threads REQUIRED)

# the layer over its client
djinterp_add_test_executable(c_mongodb_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/mongodb_tests_runner.c"
        "${_tests_dir}/mongodb_tests.c"
        ${_layer} ${_common} "${_dtest}"
        "${DJINTERP_SRC_ROOT}/djinterp/c/sync/dmutex.c"
    INCLUDES  "${_tests_dir}"
    LIBRARIES ${_client} Threads::Threads)
