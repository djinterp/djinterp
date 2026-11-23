# =============================================================================
# djinterp [test] --
# build/cmake/config/testing/djinterp/c/modules/net/codec/CMakeLists.txt
#
#   Test leaf for net's codecs (c/modules/net/codec), in DTest's C face, a
# CTest test with its runner in this directory:
#     - c_net_codec_tests: ASCII, base64, hex, the digests and lines.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.19
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/net")
set(_net       "${DJINTERP_SRC_ROOT}/djinterp/c/modules/net")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

# net's core, which every protocol links
file(GLOB _url CONFIGURE_DEPENDS "${_net}/net_url*.c")
set(_core "${_net}/net_common.c" "${_net}/net_memory.c" ${_url})

file(GLOB _tests CONFIGURE_DEPENDS "${_tests_dir}/codec/codec_tests*.c")
file(GLOB _codec CONFIGURE_DEPENDS "${_net}/codec/codec_*.c")

djinterp_add_test_executable(c_net_codec_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/codec_tests_runner.c"
        ${_tests}
        "${_tests_dir}/net_tests_assert.c"
        ${_codec}
        ${_core} "${_dtest}"
    INCLUDES  "${_tests_dir}/codec")
