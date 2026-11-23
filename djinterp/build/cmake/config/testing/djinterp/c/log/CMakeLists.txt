# =============================================================================
# djinterp [test]  -  build/cmake/config/testing/djinterp/c/log/CMakeLists.txt
#
#   Test leaf for c/log's suite, in DTest's C face
# (c/modules/test/test_suite.h), a CTest test with its runner in this
# directory:
#     - c_log_tests: the log's levels, codes, settings, records, writers and
#       times. They link the log's two units, desc's machines and units,
#       option_env's, the stream's console, file and template units with
#       the filesys units it calls (as the stream's own leaf links them),
#       and c/sync's mutex (the runtime has the stream itself), beside
#       DTest's C face.
#
#
# path:      /build/cmake/config/testing/djinterp/c/log/CMakeLists.txt
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
#                                                            revised: 2026.11.21
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/log")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")
set(_c         "${DJINTERP_SRC_ROOT}/djinterp/c")

find_package(Threads REQUIRED)

djinterp_add_test_executable(c_log_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/log_tests_runner.c"
        "${_tests_dir}/log_tests.c"
        "${_c}/log/log.c"
        "${_c}/log/log_writers.c"
        "${_c}/desc/desc_machines.c"
        "${_c}/desc/desc_units.c"
        "${_c}/desc/desc_options.c"
        "${_c}/option/option_env.c"
        "${_c}/option/option_env_registry.c"
        "${_c}/option/option_set_common.c"
        "${_c}/option/option_common.c"
        "${_c}/util/ostream/ostream_console.c"
        "${_c}/util/ostream/ostream_file.c"
        "${_c}/filesys/file_open.c"
        "${_c}/filesys/file_common.c"
        "${_c}/filesys/file_desc.c"
        "${_c}/filesys/file_io.c"
        "${_c}/filesys/file_path.c"
        "${_c}/filesys/file_seek.c"
        "${_c}/filesys/file_stat.c"
        "${_c}/text/ostream_template.c"
        "${_c}/sync/dmutex.c"
        "${_dtest}"
    INCLUDES  "${_tests_dir}")

target_link_libraries(c_log_tests PRIVATE Threads::Threads)
