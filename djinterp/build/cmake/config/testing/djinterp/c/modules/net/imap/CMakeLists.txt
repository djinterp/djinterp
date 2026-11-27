# =============================================================================
# djinterp [test] -
# build/cmake/config/testing/djinterp/c/modules/net/imap/CMakeLists.txt
#
#   Test leaf for IMAP4rev1 (c/modules/net/imap), in DTest's C face, a CTest
# test with its runner in this directory:
#     - c_net_imap_tests: strings, values, responses, options and sessions,
#       over a scripted peer. It links net's codecs and the sink, and opens
#       no socket.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.19
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net       "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

# net's core, which every protocol links
file(GLOB _url CONFIGURE_DEPENDS "${_net}/net_url*.c")
set(_core "${_net}/net_common.c" "${_net}/net_memory.c" ${_url})

file(GLOB _tests CONFIGURE_DEPENDS "${_tests_dir}/imap/imap_tests*.c")
file(GLOB _codec CONFIGURE_DEPENDS "${_net}/codec/codec_*.c")

djinterp_add_test_executable(c_net_imap_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/imap_tests_runner.c"
        ${_tests}
        "${_tests_dir}/net_tests_assert.c"
        "${_net}/imap/imap.c"
        ${_codec}
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/sink_common.c"
        ${_core} "${_dtest}"
    INCLUDES  "${_tests_dir}/imap")
