# =============================================================================
# djinterp [test] -
# build/cmake/config/testing/djinterp/c/modules/db/log/CMakeLists.txt
#
#   Test leaf for db's codes for the log (c/modules/db/db_log.h), in DTest's C
# face, a CTest test with its runner in this directory:
#     - c_db_log_tests: both tables, their registration, and the vendor's
#       message. It links the log, as a program that logs db's failures
#       does, and none of the vendors' clients.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.19
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/db")
set(_c         "${DJINTERP_SRC_ROOT}/djinterp/c")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

find_package(Threads REQUIRED)

djinterp_add_test_executable(c_db_log_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/db_log_tests_runner.c"
        "${_tests_dir}/db_log_tests.c"
        "${_c}/modules/db/db_log.c"
        "${_c}/log/log.c"
        "${_c}/log/log_writers.c"
        ${DJINTERP_LOG_CODES_SOURCES}
        "${_c}/desc/desc_machines.c"
        "${_c}/desc/desc_units.c"
        "${_c}/desc/desc_options.c"
        "${_c}/option/option_env.c"
        "${_c}/option/option_env_registry.c"
        "${_c}/option/option_set_common.c"
        "${_c}/option/option_common.c"
        "${_c}/util/ostream/ostream_console.c"
        "${_c}/util/ostream/ostream_file.c"
        "${_c}/text/ostream_template.c"
        "${_c}/sync/dmutex.c"
        "${_dtest}"
    INCLUDES  "${_tests_dir}"
    LIBRARIES djinterp_c_filesys)
target_link_libraries(c_db_log_tests PRIVATE Threads::Threads)
