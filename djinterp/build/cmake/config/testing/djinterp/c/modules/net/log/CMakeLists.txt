# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/c/modules/net/log/CMakeLists.txt
#
#   Test leaf for net's codes for the log (c/modules/net/net_log.h), in
# DTest's C face, a CTest test with its runner in this directory:
#     - c_net_log_tests: the table and its registration. It links the log,
#       as a program that logs net's failures does, and no socket backend.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.19
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_c         "${DJINTERP_SRC_ROOT}/djinterp/c")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

find_package(Threads REQUIRED)

djinterp_add_test_executable(c_net_log_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/net_log_tests_runner.c"
        "${_tests_dir}/net_log_tests.c"
        "${_c}/modules/net/net_log.c"
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
target_link_libraries(c_net_log_tests PRIVATE Threads::Threads)
