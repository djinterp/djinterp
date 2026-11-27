# =============================================================================
# djinterp [test]  -  build/cmake/config/testing/djinterp/cpp/memory/memory/CMakeLists.txt
#
#   Test leaf for cpp/memory (memory.hpp and every face it includes), in
# DTest's C face: the runner in this directory, the section translation units
# globbed from the tests tree, the four C kernels the faces wrap, and DTest's
# C face, which djinterp_c_runtime does not yet carry.
#   The kernels are compiled here, though the runtime has them, for the
# checks a test build turns on in them: a mark from another arena refused, a
# slot released twice caught. Those knobs move no field; the layout knobs are
# the whole build's, set on the runtime (ruled 2026.11.26), so these copies
# and the runtime lay memory's types out alike. The tally's checks run where
# DJINTERP_MEM_TALLY is on, as in CI.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.11
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/memory")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/memory_tests_*.cpp")
list(FILTER _sections EXCLUDE REGEX "_(main|runner)\\.cpp$")

djinterp_add_test_executable(memory_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/memory_tests_runner.cpp"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/mem_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/mem_source.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/arena.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/pool.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
